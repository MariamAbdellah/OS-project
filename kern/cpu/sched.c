#include "sched.h"

#include <inc/assert.h>

#include <kern/proc/user_environment.h>
#include <kern/trap/trap.h>
#include <kern/mem/kheap.h>
#include <kern/mem/memory_manager.h>
#include <kern/tests/utilities.h>
#include <kern/cmd/command_prompt.h>


uint32 isSchedMethodRR(){if(scheduler_method == SCH_RR) return 1; return 0;}
uint32 isSchedMethodMLFQ(){if(scheduler_method == SCH_MLFQ) return 1; return 0;}
uint32 isSchedMethodBSD(){if(scheduler_method == SCH_BSD) return 1; return 0;}

//===================================================================================//
//============================ SCHEDULER FUNCTIONS ==================================//
//===================================================================================//

//===================================
// [1] Default Scheduler Initializer:
//===================================
void sched_init()
{
	old_pf_counter = 0;

	sched_init_RR(INIT_QUANTUM_IN_MS);

	init_queue(&env_new_queue);
	init_queue(&env_exit_queue);
	scheduler_status = SCH_STOPPED;
}

//=========================
// [2] Main FOS Scheduler:
//=========================
void
fos_scheduler(void)
{
	//	cprintf("inside scheduler\n");

	chk1();
	scheduler_status = SCH_STARTED;

	//This variable should be set to the next environment to be run (if any)
	struct Env* next_env = NULL;

	if (scheduler_method == SCH_RR)
	{
		// Implement simple round-robin scheduling.
		// Pick next environment from the ready queue,
		// and switch to such environment if found.
		// It's OK to choose the previously running env if no other env
		// is runnable.

		//If the curenv is still exist, then insert it again in the ready queue
		if (curenv != NULL)
		{
			enqueue(&(env_ready_queues[0]), curenv);
		}

		//Pick the next environment from the ready queue
		next_env = dequeue(&(env_ready_queues[0]));

		//Reset the quantum
		//2017: Reset the value of CNT0 for the next clock interval
		kclock_set_quantum(quantums[0]);
		//uint16 cnt0 = kclock_read_cnt0_latch() ;
		//cprintf("CLOCK INTERRUPT AFTER RESET: Counter0 Value = %d\n", cnt0 );

	}
	else if (scheduler_method == SCH_MLFQ)
	{
		next_env = fos_scheduler_MLFQ();
	}
	else if (scheduler_method == SCH_BSD)
	{
		next_env = fos_scheduler_BSD();
	}
	//temporarily set the curenv by the next env JUST for checking the scheduler
	//Then: reset it again
	struct Env* old_curenv = curenv;
	curenv = next_env ;
	chk2(next_env) ;
	curenv = old_curenv;

	//sched_print_all();

	if(next_env != NULL)
	{
		//		cprintf("\nScheduler select program '%s' [%d]... counter = %d\n", next_env->prog_name, next_env->env_id, kclock_read_cnt0());
		//		cprintf("Q0 = %d, Q1 = %d, Q2 = %d, Q3 = %d\n", queue_size(&(env_ready_queues[0])), queue_size(&(env_ready_queues[1])), queue_size(&(env_ready_queues[2])), queue_size(&(env_ready_queues[3])));
		env_run(next_env);
	}
	else
	{
		/*2015*///No more envs... curenv doesn't exist any more! return back to command prompt
		curenv = NULL;
		//lcr3(K_PHYSICAL_ADDRESS(ptr_page_directory));
		lcr3(phys_page_directory);

		//cprintf("SP = %x\n", read_esp());

		scheduler_status = SCH_STOPPED;
		//cprintf("[sched] no envs - nothing more to do!\n");
		while (1)
			run_command_prompt(NULL);

	}
}

//=============================
// [3] Initialize RR Scheduler:
//=============================
void sched_init_RR(uint8 quantum)
{

	// Create 1 ready queue for the RR
	num_of_ready_queues = 1;
#if USE_KHEAP
	sched_delete_ready_queues();
	env_ready_queues = kmalloc(sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8)) ;
#endif
	quantums[0] = quantum;
	kclock_set_quantum(quantums[0]);
	init_queue(&(env_ready_queues[0]));

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_RR;
	//=========================================
	//=========================================
}

//===============================
// [4] Initialize MLFQ Scheduler:
//===============================
void sched_init_MLFQ(uint8 numOfLevels, uint8 *quantumOfEachLevel)
{
#if USE_KHEAP
	//=========================================
	//DON'T CHANGE THESE LINES=================
	sched_delete_ready_queues();
	//=========================================
	//=========================================

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_MLFQ;
	//=========================================
	//=========================================
#endif
}

//===============================
// [5] Initialize BSD Scheduler:
//===============================
void sched_init_BSD(uint8 numOfLevels, uint8 quantum)
{
#if USE_KHEAP
	//TODO: [PROJECT'23.MS3 - #4] [2] BSD SCHEDULER - sched_init_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");
	kclock_set_quantum(quantum);
	//env_ready_queues[numOfLevels];
	num_of_ready_queues = numOfLevels;
	//?
	env_ready_queues = kmalloc(num_of_ready_queues * sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8));
	for(uint8 i = 0; i < numOfLevels; i++){
		//?
		//env_ready_queues[i] = (struct Env_Queue*)kmalloc(sizeof(env_ready_queues[i]));
		//quantums = kmalloc(num_of_ready_queues * sizeof(uint8));
		init_queue(&(env_ready_queues[i]));

		quantums[i] = quantum;
	}
	load_avg = fix_int(0);

	//=========================================
	//DON'T CHANGE THESE LINES=================
	scheduler_status = SCH_STOPPED;
	scheduler_method = SCH_BSD;
	//=========================================
	//=========================================
#endif
}


//=========================
// [6] MLFQ Scheduler:
//=========================
struct Env* fos_scheduler_MLFQ()
{
	panic("not implemented");
	return NULL;
}

//=========================
// [7] BSD Scheduler:
//=========================
struct Env* fos_scheduler_BSD()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - fos_scheduler_BSD
	//Your code is here
	//Comment the following line
	//panic("Not implemented yet");

	int t = timer_ticks();
	int nice = (curenv->nice * 2);
	fixed_point_t recent_cpu = curenv->recent_cpu;
	recent_cpu = fix_unscale(recent_cpu, 4);

	int rounded_recent = fix_trunc(recent_cpu);
	int priority = PRI_MAX - rounded_recent - nice;

	enqueue(&env_ready_queues[priority], curenv);

	struct Env* env_to_run = NULL;

	for(uint8 i = PRI_MAX; i >= 0; i--)
	{
		if(queue_size(&(env_ready_queues[i])) != 0)
		{
			env_to_run = dequeue(&(env_ready_queues[i]));
			//env_to_run->recent_cpu[0] = 0; //0? sec or tick?
			//env_to_run->nice = 0;
			break;
		}
	}

	return env_to_run;
}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================

 void clock_interrupt_handler()
{
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	{

		/*

		int t = timer_ticks();

		if(t == 0)
		{       //what about curent running env?
			struct Env* env;
			int q_size;
			for(int i = 0; i <= num_of_ready_queues; i++)
			{
				q_size = queue_size(&env_ready_queues[i]);
				//initialize nice & recent_cpu and calc priority
				for(int j = 0; j < q_size; j++)
				{
					env = dequeue(&env_ready_queues[i]);
					env->nice = 0;
					env->recent_cpu[0] = fix_int(0);

					int priority = PRI_MAX;
					enqueue(&env_ready_queues[priority], env);
				}
			}
		}

		fixed_point_t seconds = fix_int(ticks * quantums[1]);
		seconds = fix_unscale(seconds, 1000);//fix_unscale(secs, 1000);
		//int t = fix_trunc(seconds);
		fixed_point_t seconds_whole = fix_int(fix_trunc(seconds));

		fixed_point_t con = fix_sub(seconds, seconds_whole);
		fixed_point_t zero = fix_int(0);

		if(con == 0)
		{
			int ready_processes = 0;
			for (int i = PRI_MIN; i <= PRI_MAX; i++)
			{
				ready_processes += queue_size(&(env_ready_queues[i]));
			}

			ready_processes ++;      //curenv

			int t = fix_trunc(seconds_whole);
			fixed_point_t x = fix_int(59);
			fixed_point_t y = fix_int(60);
			fixed_point_t z = fix_int(1);
			fixed_point_t div1 = fix_div(x,y);
			fixed_point_t div2 = fix_div(z, y);
			int tprev = t - 1;
			if (tprev == 0)
			{
				//load_avg[tprev] = 0;
				load_avg[tprev] = fix_scale(div2, ready_processes);
				//load_avg[t] = fix_scale(div2, ready_processes);
			}

			load_avg[t] = fix_add(fix_mul(div1, load_avg[tprev]), fix_scale(div2, ready_processes));
			for (int i = PRI_MIN; i <= PRI_MAX; i++){
				int current_queue_size = queue_size(&(env_ready_queues[i]));
				for (int y = 0; y < current_queue_size; y++){
					struct Env* env = dequeue(&(env_ready_queues[i]));
					fixed_point_t load_avg_2 = fix_scale(load_avg[t], 2);
					fixed_point_t f1 = fix_int(1);
					fixed_point_t load_avg_2_1 = fix_add(load_avg_2, f1);
					//fixed_point_t load_avg_2_1 = load_avg_2 + 1;
					fixed_point_t a = fix_div(load_avg_2, load_avg_2_1);
					fixed_point_t a_recent_cpu = fix_mul(a, env->recent_cpu[t - 1]);
					fixed_point_t fnice = fix_int(env->nice);
					env->recent_cpu[t] = fix_add(a_recent_cpu, fnice);
					//fixed_point_t recentC = fix_add(a_recent_cpu, fnice);
					//env->recent_cpu[t] = recentC
					//->recent_cpu[t] = a_recent_cpu + env->nice;
				}

			}
		}
		*/

		/*
		if(curenv != NULL)//increment recent cpu for running env every tick
		{
			//struct Env * runningenv = fos_scheduler_BSD();
//
//			fixed_point_t load_avg_2 = fix_scale(load_avg[t], 2);
//			fixed_point_t float1 = fix_int(1);
//			fixed_point_t load_avg_2_1 = fix_add(load_avg_2, float1);
//			//fixed_point_t load_avg_2_1 = load_avg_2 + 1;
//			fixed_point_t a = fix_div(load_avg_2, load_avg_2_1);
//			fixed_point_t a_recent_cpu = fix_mul(a, curenv->recent_cpu[t - 1]);
//			fixed_point_t nice1 = fix_int(curenv->nice);

			//int inc = fix_int(1);
			fixed_point_t prev_recent_cpu = curenv->recent_cpu[t];
			//curenv->recent_cpu[t] = fix_add(prev_recent_cpu, inc);
		}


		struct Env * env = NULL;
		if(t % 4 == 0)
		{
			int q_size;
			for(int i = num_of_ready_queues; i >= 0; i--)
			{
				q_size = queue_size(&env_ready_queues[i]);
				for(int j = 0; j < q_size; j++)
				{
					env = dequeue(&(env_ready_queues[i]));
					fixed_point_t cpu_time = fix_unscale(env->recent_cpu[t], 4);
					//fixed_point_t nice = fix_scale(env->nice, 2);
					int nice = env->nice * 2;

					int recent_cpu = fix_trunc(cpu_time);
					int priority = PRI_MAX - recent_cpu - nice;

					int prev_priority = env->priority;

					remove_from_queue(&env_ready_queues[prev_priority], env);
					enqueue(&(env_ready_queues[priority]), env);

//					if(priority != prev_priority)//infinity loop
//					{
//						remove_from_queue(&env_ready_queues[prev_priority], env);
//						enqueue(&(env_ready_queues[priority]), env);
//					}

					if(priority > PRI_MAX)
					{
						env->priority = PRI_MAX;
					}
					else if(priority < PRI_MIN)
					{
						env->priority = PRI_MIN;

					}
					else
					{
						env->priority = priority;
					}
				}


			}

		}



	}


	*/

		int t = timer_ticks();

				int miliseconds = t * quantums[0];

				if(miliseconds % 1000 == 0)
				{
					int ready_processes = 0;
					for (int i = PRI_MIN; i <= PRI_MAX; i++)
					{
						ready_processes += queue_size(&(env_ready_queues[i]));
					}

					if(curenv != NULL)
					{
						ready_processes ++;      //curenv
					}

					fixed_point_t x = fix_int(59);
					fixed_point_t y = fix_int(60);
					fixed_point_t z = fix_int(1);
					fixed_point_t div1 = fix_div(x,y);
					fixed_point_t div2 = fix_div(z, y);
		//			if (tprev == 0)
		//			{
		//				//load_avg[tprev] = 0;
		//				load_avg[tprev] = fix_scale(div2, ready_processes);
		//				//load_avg[t] = fix_scale(div2, ready_processes);
		//			}
					fixed_point_t prev_load_avg = load_avg;
					load_avg = fix_add(fix_mul(div1, prev_load_avg), fix_scale(div2, ready_processes));
					for (int i = PRI_MIN; i <= PRI_MAX; i++){
						int current_queue_size = queue_size(&(env_ready_queues[i]));
						for (int y = 0; y < current_queue_size; y++){
							struct Env* env = dequeue(&(env_ready_queues[i]));

							fixed_point_t load_avg_2 = fix_scale(load_avg, 2);
							fixed_point_t f1 = fix_int(1);
							fixed_point_t load_avg_2_1 = fix_add(load_avg_2, f1);
							fixed_point_t a = fix_div(load_avg_2, load_avg_2_1);

							fixed_point_t a_recent_cpu = fix_mul(a, env->recent_cpu);
							fixed_point_t fnice = fix_int(env->nice);
							env->recent_cpu = fix_add(a_recent_cpu, fnice);
						}

					}
				}

				if(curenv != NULL)//calc recent cpu for running env every tick
				{
					//struct Env * runningenv = fos_scheduler_BSD();

		/*			fixed_point_t load_avg_2 = fix_scale(load_avg, 2);
					fixed_point_t float1 = fix_int(1);
					fixed_point_t load_avg_2_1 = fix_add(load_avg_2, float1);
					fixed_point_t a = fix_div(load_avg_2, load_avg_2_1);


					fixed_point_t a_recent_cpu = fix_mul(a, curenv->recent_cpu);
					fixed_point_t nice1 = fix_int(curenv->nice);
					curenv->recent_cpu = fix_add(a_recent_cpu, nice1);*/

					fixed_point_t inc = fix_int(1);
					fixed_point_t prev_recent_cpu = curenv->recent_cpu;
					curenv->recent_cpu = fix_add(prev_recent_cpu, inc);
				}


				struct Env * env = NULL;
				if(t % 4 == 0)
				{
					int q_size;
					for(int i = 0; i <= num_of_ready_queues; i++)
					{
						q_size = queue_size(&env_ready_queues[i]);
						for(int j = 0; j < q_size; j++)
						{
							env = dequeue(&(env_ready_queues[i]));

							int prev_priority = env->priority;

							int priority = PRI_MAX - fix_trunc(fix_unscale(env->recent_cpu, 4)) - (env->nice * 2);


							remove_from_queue(&env_ready_queues[prev_priority], env);
							enqueue(&(env_ready_queues[priority]), env);

		//					if(priority != prev_priority)//infinity loop
		//					{
		//						remove_from_queue(&env_ready_queues[prev_priority], env);
		//						enqueue(&(env_ready_queues[priority]), env);
		//					}

							if(priority > PRI_MAX)
							{
								env->priority = PRI_MAX;
							}
							else if(priority < PRI_MIN)
							{
								env->priority = PRI_MIN;

							}
							else
							{
								env->priority = priority;
							}
						}


					}

				}

	}
	/********DON'T CHANGE THIS LINE***********/
	ticks++ ;
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_TIME_APPROX))
	{
		update_WS_time_stamps();
	}
	//cprintf("Clock Handler\n") ;
	fos_scheduler();
	/*****************************************/
}


//===================================================================
// [9] Update LRU Timestamp of WS Elements
//	  (Automatically Called Every Quantum in case of LRU Time Approx)
//===================================================================
void update_WS_time_stamps()
{
	struct Env *curr_env_ptr = curenv;

	if(curr_env_ptr != NULL)
	{
		struct WorkingSetElement* wse ;
		{
			int i ;
#if USE_KHEAP
			LIST_FOREACH(wse, &(curr_env_ptr->page_WS_list))
			{
#else
			for (i = 0 ; i < (curr_env_ptr->page_WS_max_size); i++)
			{
				wse = &(curr_env_ptr->ptr_pageWorkingSet[i]);
				if( wse->empty == 1)
					continue;
#endif
				//update the time if the page was referenced
				uint32 page_va = wse->virtual_address ;
				uint32 perm = pt_get_page_permissions(curr_env_ptr->env_page_directory, page_va) ;
				uint32 oldTimeStamp = wse->time_stamp;

				if (perm & PERM_USED)
				{
					wse->time_stamp = (oldTimeStamp>>2) | 0x80000000;
					pt_set_page_permissions(curr_env_ptr->env_page_directory, page_va, 0 , PERM_USED) ;
				}
				else
				{
					wse->time_stamp = (oldTimeStamp>>2);
				}
			}
		}

		{
			int t ;
			for (t = 0 ; t < __TWS_MAX_SIZE; t++)
			{
				if( curr_env_ptr->__ptr_tws[t].empty != 1)
				{
					//update the time if the page was referenced
					uint32 table_va = curr_env_ptr->__ptr_tws[t].virtual_address;
					uint32 oldTimeStamp = curr_env_ptr->__ptr_tws[t].time_stamp;

					if (pd_is_table_used(curr_env_ptr->env_page_directory, table_va))
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2) | 0x80000000;
						pd_set_table_unused(curr_env_ptr->env_page_directory, table_va);
					}
					else
					{
						curr_env_ptr->__ptr_tws[t].time_stamp = (oldTimeStamp>>2);
					}
				}
			}
		}
	}
}


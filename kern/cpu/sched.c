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

	//cprintf("sched_init_BSD called with numoflevels = %d, and quantum = %d \n", numOfLevels, quantum);
	kclock_set_quantum(quantum);
	//cprintf("kclock quantum is set \n");
	//env_ready_queues[numOfLevels];
	num_of_ready_queues = numOfLevels;
	//?
	//cprintf("set the number of ready queues \n");
	env_ready_queues = kmalloc(num_of_ready_queues * sizeof(struct Env_Queue));
	quantums = kmalloc(num_of_ready_queues * sizeof(uint8));
	//cprintf("allocated size for env_ready_quesues and quantums \n");

	for(uint8 i = 0; i < numOfLevels; i++){
		//cprintf("looping on number of levels -> level i = %d \n", i);
		//?
		//env_ready_queues[i] = (struct Env_Queue*)kmalloc(sizeof(env_ready_queues[i]));
		//quantums = kmalloc(num_of_ready_queues * sizeof(uint8));
		init_queue(&(env_ready_queues[i]));
		quantums[i] = quantum;
		//cprintf("initialized ready queue %d and quantum %d \n", i, i);
	}
	load_avg = fix_int(0);
	//cprintf("set load_avg = 0 \n");

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

	//cprintf("**********************called fos_scheduler_BSD**************** \n");
	if(curenv != NULL)//&&stat
	{
		//cprintf("checked existing curenv \n");
		int nice = (curenv->nice * 2);
		fixed_point_t recent_cpu = curenv->recent_cpu;
		recent_cpu = fix_unscale(recent_cpu, 4);

		int rounded_recent = fix_trunc(recent_cpu);
		int priority = PRI_MAX - rounded_recent - nice;
		//curenv->env_status = ENV_READY;

		if(priority > PRI_MAX)
		{
			//cprintf("priority higher than max -> setting it to max \n");
			curenv->priority = PRI_MAX;
		}
		else if(priority < PRI_MIN)
		{
			//cprintf("priority lower than min -> setting it to min \n");
			curenv->priority = PRI_MIN;

		}
		else
		{
			//cprintf("priority set \n");
			curenv->priority = priority;
		}

		//cprintf("curenve id = %d \t curenve priority = %d\n", curenv->env_id, curenv->priority);

		//cprintf("trying to add curenv to queue \n");
		enqueue(&(env_ready_queues[curenv->priority]), curenv);
		//cprintf("calculated curenv priority and enqueued in ready queue of priority level \n");
	}

	struct Env* env_to_run = NULL;

	//cprintf("searching for env to run \n");
	for(int i = PRI_MAX; i >= PRI_MIN; i--)
	{
		if(queue_size(&(env_ready_queues[i])) != 0)
		{
			//cprintf("found env at level i = %d \n", i);
			env_to_run = dequeue(&(env_ready_queues[i]));
			//cprintf("returned env = %x\n", env_to_run);
			//cprintf("set quantums[%d] = quantums[0] = %d \n", i, quantums[0]);
			//quantums[i] = quantums[0];

			kclock_set_quantum(quantums[i]);
//			if(env_to_run != NULL)
//			{
				break;
//
//			}
		}
	}

	if (env_to_run == NULL)
	{
		load_avg = fix_int(0);
		//cprintf("found no ready env to run -> load_avg = 0 \n");
	}

	//cprintf("return env \n");
	return env_to_run;
}

//========================================
// [8] Clock Interrupt Handler
//	  (Automatically Called Every Quantum)
//========================================

void clock_interrupt_handler()
{
	//cprintf("**clock_interrupt_handler is called ** \n");
	//TODO: [PROJECT'23.MS3 - #5] [2] BSD SCHEDULER - Your code is here
	{
		//cprintf("checking if sch_bsd \n");
		if(scheduler_method == SCH_BSD)
		{
			int t = timer_ticks();

			int miliseconds = t * quantums[0];

			//cprintf("calculate miliseconds = t * quantums[0] = %d \n", miliseconds);
			if(miliseconds % 1000 == 0)
			{

				//cprintf("entered condition that miliseconds are perfect seconds by passing % 1000 == 0 \n");
				int ready_processes = 0;

				//cprintf("calculating numbers of ready processes \n");
				for (int i = PRI_MIN; i <= PRI_MAX; i++)
				{
					ready_processes += queue_size(&(env_ready_queues[i]));
				}

				//cprintf("check if curenv exist and add it on ready_processes numbers \n");
				if (curenv != NULL){
					ready_processes ++;      //curenv
				}

				fixed_point_t x = fix_int(59);
				fixed_point_t y = fix_int(60);
				fixed_point_t z = fix_int(1);
				fixed_point_t div1 = fix_div(x,y);
				fixed_point_t div2 = fix_div(z, y);

				fixed_point_t prev_load_avg = load_avg;
				load_avg = fix_add(fix_mul(div1, prev_load_avg), fix_scale(div2, ready_processes));

				//cprintf("calculated load_avg \n");

				//cprintf("calculating recent_cpu for all ready queues \n");
				for (int i = PRI_MIN; i <= PRI_MAX; i++)
				{

					struct Env * env = NULL;
					LIST_FOREACH(env, &(env_ready_queues[i]))
					{
						fixed_point_t load_avg_2 = fix_scale(load_avg, 2);
						fixed_point_t f1 = fix_int(1);
						fixed_point_t load_avg_2_1 = fix_add(load_avg_2, f1);
						fixed_point_t a = fix_div(load_avg_2, load_avg_2_1);

						fixed_point_t a_recent_cpu = fix_mul(a, env->recent_cpu);
						fixed_point_t fnice = fix_int(env->nice);
						env->recent_cpu = fix_add(a_recent_cpu, fnice);
					}

					/*
					int current_queue_size = queue_size(&(env_ready_queues[i]));
					for (int y = 0; y < current_queue_size; y++)
					{

						cprintf("deq for calc recent cpu\n");
						struct Env* env = dequeue(&(env_ready_queues[i]));

						fixed_point_t load_avg_2 = fix_scale(load_avg, 2);
						fixed_point_t f1 = fix_int(1);
						fixed_point_t load_avg_2_1 = fix_add(load_avg_2, f1);
						fixed_point_t a = fix_div(load_avg_2, load_avg_2_1);

						fixed_point_t a_recent_cpu = fix_mul(a, env->recent_cpu);
						fixed_point_t fnice = fix_int(env->nice);
						env->recent_cpu = fix_add(a_recent_cpu, fnice);

						enqueue(&(env_ready_queues[i]), env);

						//cprintf("calculated recent_cpu for item y = %d in queue i = %d \n", y, i);
					}*/
					//cprintf ("------------- \n");

				}
			}

			if(curenv != NULL)//calc recent cpu for running env every tick
			{

				//cprintf("curenv exists: incrementing recent_cpu \n");
				fixed_point_t inc = fix_int(1);
				fixed_point_t prev_recent_cpu = curenv->recent_cpu;
				curenv->recent_cpu = fix_add(prev_recent_cpu, inc);
			}



			if(t % 4 == 0)
			{
				//cprintf("4 ticks passed ---- \n");
				int q_size;
				//cprintf("calculating priority for all env \n");
				struct Env * env = NULL;
				for(uint8 i = 0; i < num_of_ready_queues; i++)
				{

					LIST_FOREACH(env, &(env_ready_queues[i]))
					{
						int prev_priority = env->priority;

						int priority = PRI_MAX - fix_trunc(fix_unscale(env->recent_cpu, 4)) - (env->nice * 2);

						if(priority > PRI_MAX)
						{
							//cprintf("priority higher than max -> setting it to max \n");
							env->priority = PRI_MAX;
						}
						else if(priority < PRI_MIN)
						{
							//cprintf("priority lower than min -> setting it to min \n");
							env->priority = PRI_MIN;

						}
						else
						{
							//cprintf("priority set \n");
							env->priority = priority;
						}

						if(env->priority != prev_priority)
						{
							remove_from_queue(&(env_ready_queues[i]), env);
							enqueue(&(env_ready_queues[env->priority]), env);
						}
					}

					/*
					q_size = queue_size(&env_ready_queues[i]);
					for(int j = 0; j < q_size; j++)
					{
						cprintf("deq for calc priority\n");
						env = dequeue(&(env_ready_queues[i]));

						int prev_priority = env->priority;

						int priority = PRI_MAX - fix_trunc(fix_unscale(env->recent_cpu, 4)) - (env->nice * 2);

						if(priority > PRI_MAX)
						{
							//cprintf("priority higher than max -> setting it to max \n");
							env->priority = PRI_MAX;
						}
						else if(priority < PRI_MIN)
						{
							//cprintf("priority lower than min -> setting it to min \n");
							env->priority = PRI_MIN;

						}
						else
						{
							//cprintf("priority set \n");
							env->priority = priority;
						}

						enqueue(&(env_ready_queues[env->priority]), env);
						//cprintf("calculated priority for env order j = %d at queue i = %d \n", j, i);
					}
					//cprintf("--------------------- \n");

					*/
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


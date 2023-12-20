/*
 * fault_handler.c
 *
 *  Created on: Oct 12, 2022
 *      Author: HP
 */

#include "trap.h"
#include <kern/proc/user_environment.h>
#include "../cpu/sched.h"
#include "../disk/pagefile_manager.h"
#include "../mem/memory_manager.h"
#include "kern/mem/kheap.h"

//2014 Test Free(): Set it to bypass the PAGE FAULT on an instruction with this length and continue executing the next one
// 0 means don't bypass the PAGE FAULT
uint8 bypassInstrLength = 0;

//===============================
// REPLACEMENT STRATEGIES
//===============================
//2020
void setPageReplacmentAlgorithmLRU(int LRU_TYPE)
{
	assert(LRU_TYPE == PG_REP_LRU_TIME_APPROX || LRU_TYPE == PG_REP_LRU_LISTS_APPROX);
	_PageRepAlgoType = LRU_TYPE ;
}
void setPageReplacmentAlgorithmCLOCK(){_PageRepAlgoType = PG_REP_CLOCK;}
void setPageReplacmentAlgorithmFIFO(){_PageRepAlgoType = PG_REP_FIFO;}
void setPageReplacmentAlgorithmModifiedCLOCK(){_PageRepAlgoType = PG_REP_MODIFIEDCLOCK;}
/*2018*/ void setPageReplacmentAlgorithmDynamicLocal(){_PageRepAlgoType = PG_REP_DYNAMIC_LOCAL;}
/*2021*/ void setPageReplacmentAlgorithmNchanceCLOCK(int PageWSMaxSweeps){_PageRepAlgoType = PG_REP_NchanceCLOCK;  page_WS_max_sweeps = PageWSMaxSweeps;}

//2020
uint32 isPageReplacmentAlgorithmLRU(int LRU_TYPE){return _PageRepAlgoType == LRU_TYPE ? 1 : 0;}
uint32 isPageReplacmentAlgorithmCLOCK(){if(_PageRepAlgoType == PG_REP_CLOCK) return 1; return 0;}
uint32 isPageReplacmentAlgorithmFIFO(){if(_PageRepAlgoType == PG_REP_FIFO) return 1; return 0;}
uint32 isPageReplacmentAlgorithmModifiedCLOCK(){if(_PageRepAlgoType == PG_REP_MODIFIEDCLOCK) return 1; return 0;}
/*2018*/ uint32 isPageReplacmentAlgorithmDynamicLocal(){if(_PageRepAlgoType == PG_REP_DYNAMIC_LOCAL) return 1; return 0;}
/*2021*/ uint32 isPageReplacmentAlgorithmNchanceCLOCK(){if(_PageRepAlgoType == PG_REP_NchanceCLOCK) return 1; return 0;}

//===============================
// PAGE BUFFERING
//===============================
void enableModifiedBuffer(uint32 enableIt){_EnableModifiedBuffer = enableIt;}
uint8 isModifiedBufferEnabled(){  return _EnableModifiedBuffer ; }

void enableBuffering(uint32 enableIt){_EnableBuffering = enableIt;}
uint8 isBufferingEnabled(){  return _EnableBuffering ; }

void setModifiedBufferLength(uint32 length) { _ModifiedBufferLength = length;}
uint32 getModifiedBufferLength() { return _ModifiedBufferLength;}

//===============================
// FAULT HANDLERS
//===============================

//Handle the table fault
void table_fault_handler(struct Env * curenv, uint32 fault_va)
{
	//panic("table_fault_handler() is not implemented yet...!!");
	//Check if it's a stack page
	uint32* ptr_table;
#if USE_KHEAP
	{
		ptr_table = create_page_table(curenv->env_page_directory, (uint32)fault_va);
	}
#else
	{
		__static_cpt(curenv->env_page_directory, (uint32)fault_va, &ptr_table);
	}
#endif
}

void readpagefromPF(struct Env * curenv, uint32 fault_va)
{
	struct FrameInfo *fi_ptr = NULL;

	int val =allocate_frame(&fi_ptr);
	/*
	if(fi_ptr != NULL)
	{

	}
	*/




	map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
	fi_ptr->va = fault_va;

	val = pf_read_env_page(curenv,(void *)fault_va);

	if(val == E_PAGE_NOT_EXIST_IN_PF)
	{
		if(!((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)))
		{

			sched_kill_env(curenv->env_id);

		}
	}



	//int val = pf_read_env_page(curenv,(void *)fault_va);
/*
	if(val == E_PAGE_NOT_EXIST_IN_PF)
	{
		if(fault_va < USTACKTOP && fault_va >= USTACKBOTTOM)
		{

			val =allocate_frame(&fi_ptr);
			map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
			fi_ptr->va = fault_va;
			//val=pf_update_env_page(curenv,fault_va,fi_ptr);
		}
		else if (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)
		{

			val =allocate_frame(&fi_ptr);
			map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
			fi_ptr->va = fault_va;
			//val=pf_update_env_page(curenv,fault_va,fi_ptr);
		}
		else
		{
			sched_kill_env(curenv->env_id);
		}
	}
	else
	{
		val =allocate_frame(&fi_ptr);
		map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
		fi_ptr->va = fault_va;
		//val=pf_update_env_page(curenv,fault_va,fi_ptr);
	}
	*/
}

//Handle the page fault

void page_fault_handler(struct Env * curenv, uint32 fault_va)
{
#if USE_KHEAP
	struct WorkingSetElement *victimWSElement = NULL;
	uint32 wsSize = LIST_SIZE(&(curenv->page_WS_list));
#else
	int iWS =curenv->page_last_WS_index;
	uint32 wsSize = env_page_ws_get_size(curenv);
#endif




	//cprintf("REPLACEMENT=========================WS Size = %d\n", wsSize );
	//refer to the project presentation and documentation for details
	if(isPageReplacmentAlgorithmFIFO())
	{
		//TODO: [PROJECT'23.MS3 - #1] [1] PAGE FAULT HANDLER - FIFO Replacement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() FIFO Replacement is not implemented yet...!!");
		if(wsSize < (curenv->page_WS_max_size))
		{
			//cprintf("PLACEMENT=========================WS Size = %d\n", wsSize );
			//TODO: [PROJECT'23.MS2 - #15] [3] PAGE FAULT HANDLER - Placement
			// Write your code here, remove the panic and write your code
			//panic("page_fault_handler().PLACEMENT is not implemented yet...!!");
			//refer to the project presentation and documentation for details

			//readpagefromPF(curenv, fault_va);

			struct FrameInfo *fi_ptr = NULL;
			int val = allocate_frame(&fi_ptr);
			map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE | PERM_AVAILABLE);
			fi_ptr->va = fault_va;

			val = pf_read_env_page(curenv,(void *)fault_va);
			if(val == E_PAGE_NOT_EXIST_IN_PF)
			{
				//cprintf("Entered condition E_PAGE_NOT_EXIST_IN_PF");

				if(!((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM ) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)))
				{
					sched_kill_env(curenv->env_id);
				}
			}

			struct WorkingSetElement* new_element = env_page_ws_list_create_element(curenv, fault_va);
			struct WorkingSetElement* last_element = LIST_LAST(&(curenv->page_WS_list));
			LIST_INSERT_AFTER(&(curenv->page_WS_list), last_element, new_element);
			curenv->page_last_WS_index = (curenv->page_last_WS_index) + 1;
			if (LIST_SIZE(&(curenv->page_WS_list)) ==  (curenv->page_WS_max_size))
			{
				curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			}
			else// if(LIST_SIZE(&(curenv->page_WS_list)) < curenv->page_WS_max_size)
			{
				curenv->page_last_WS_element = NULL;
			}

		}
		else
		{
			struct FrameInfo *fi_ptr = NULL;
			int val = allocate_frame(&fi_ptr);
			map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE | PERM_AVAILABLE);
			fi_ptr->va = fault_va;

			val = pf_read_env_page(curenv,(void *)fault_va);
			if(val == E_PAGE_NOT_EXIST_IN_PF)
			{
				//cprintf("Entered condition E_PAGE_NOT_EXIST_IN_PF");

				if(!((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM ) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)))
				{
					sched_kill_env(curenv->env_id);
				}
			}

			uint32 *ptr_page_table;
			struct WorkingSetElement* First_element = LIST_FIRST(&(curenv->page_WS_list));
			struct WorkingSetElement *New_element = env_page_ws_list_create_element(curenv, fault_va);

			uint32 pagePerm = pt_get_page_permissions(curenv->env_page_directory, First_element->virtual_address);
			if(pagePerm & PERM_MODIFIED)
			{
				uint32* ptr_page_table = NULL;
				struct FrameInfo* frame_ptr = get_frame_info(curenv->env_page_directory, First_element->virtual_address, &ptr_page_table);
				int ret = pf_update_env_page(curenv, First_element->virtual_address, frame_ptr);
				cprintf("updated pf\n");
			}
			LIST_REMOVE(&(curenv->page_WS_list),First_element);
			struct FrameInfo *ptr_frame_info = get_frame_info(curenv->env_page_directory,First_element->virtual_address, &ptr_page_table);
			if (ptr_frame_info != 0)
			{
				unmap_frame(curenv->env_page_directory,First_element->virtual_address);
			}

			kfree(First_element);
			LIST_INSERT_TAIL(&(curenv->page_WS_list), New_element);

			// Update page indices and elements
			//curenv->page_last_WS_index = ((curenv->page_last_WS_index) + 1) % curenv->page_WS_max_size;
			curenv->page_last_WS_element = LIST_FIRST(&(curenv->page_WS_list));
			//curenv->page_last_WS_element=New_element;

			cprintf("ennnddd fifiooooo func \n");


		}


	}
	if(isPageReplacmentAlgorithmLRU(PG_REP_LRU_LISTS_APPROX))
	{
		//TODO: [PROJECT'23.MS3 - #2] [1] PAGE FAULT HANDLER - LRU Replacement
		// Write your code here, remove the panic and write your code
		//panic("page_fault_handler() LRU Replacement is not implemented yet...!!");

		int activeSize = LIST_SIZE(&(curenv->ActiveList));
		int secondSize = LIST_SIZE(&(curenv->SecondList));

		//map and alloc?



		if((activeSize + secondSize) < curenv->page_WS_max_size)//(LIST_SIZE(&(curenv->ActiveList)) + LIST_SIZE(&(curenv->SecondList)))
			//(activeSize < curenv->ActiveListSize && secondSize < curenv->SecondListSize)
		{

			cprintf("placement fault_va = %x\n", fault_va);

			//check if va already exists in any of the two lists

			/*
			 * invalidate(va)
			 * add (if first list is full -> second)
			 *  */
			//env_page_ws_invalidate(curenv, fault_va);

			//insert element in curenv->page_WS_list && update pf if modified(in replacement)
			if(activeSize < curenv->ActiveListSize)
			{
				//readpagefromPF(curenv, fault_va);

				struct FrameInfo *fi_ptr = NULL;
				int val = allocate_frame(&fi_ptr);
				map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE | PERM_AVAILABLE);
				fi_ptr->va = fault_va;

				val = pf_read_env_page(curenv,(void *)fault_va);
				if(val == E_PAGE_NOT_EXIST_IN_PF)
				{
					//cprintf("Entered condition E_PAGE_NOT_EXIST_IN_PF");

					if(!((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM ) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)))
					{
						sched_kill_env(curenv->env_id);
					}
				}


				pt_set_page_permissions(curenv->env_page_directory, fault_va, PERM_PRESENT, 0);
				//set present = 1?
				struct WorkingSetElement* newElement = env_page_ws_list_create_element(curenv, fault_va);
				LIST_INSERT_HEAD(&(curenv->ActiveList), newElement);
			}
			else
			{
				//env_page_ws_invalidate(curenv, fault_va);

				int foundinSL = 0;
				struct WorkingSetElement* temp;
				LIST_FOREACH(temp, &(curenv->SecondList))
				{

					if(ROUNDDOWN(temp->virtual_address, PAGE_SIZE) == ROUNDDOWN(fault_va, PAGE_SIZE))
					{
						LIST_REMOVE(&(curenv->SecondList), temp);

						struct WorkingSetElement* scElement = LIST_LAST(&(curenv->ActiveList));
						LIST_REMOVE(&(curenv->ActiveList), scElement);

						LIST_INSERT_HEAD(&(curenv->ActiveList), temp);

						LIST_INSERT_HEAD(&(curenv->SecondList), scElement);

						pt_set_page_permissions(curenv->env_page_directory,  temp->virtual_address, PERM_PRESENT, 0);
						pt_set_page_permissions(curenv->env_page_directory,  scElement->virtual_address, 0, PERM_PRESENT);

						foundinSL = 1;
						break;
					}
				}


				if(!foundinSL)
			//else
				{
					//struct WorkingSetElement* lastElement = LIST_LAST(&(curenv->SecondList));
					//LIST_REMOVE(&(curenv->SecondList), lastElement);

					//readpagefromPF(curenv, fault_va);

					struct FrameInfo *fi_ptr = NULL;
					allocate_frame(&fi_ptr);
					map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE | PERM_AVAILABLE);
					fi_ptr->va = fault_va;

					int val = pf_read_env_page(curenv,(void *)fault_va);
					if(val == E_PAGE_NOT_EXIST_IN_PF)
					{
						//cprintf("Entered condition E_PAGE_NOT_EXIST_IN_PF");

						if(!((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM ) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)))
						{
							sched_kill_env(curenv->env_id);
						}
					}


					struct WorkingSetElement* scElement = LIST_LAST(&(curenv->ActiveList));
					LIST_REMOVE(&(curenv->ActiveList), scElement);

					pt_set_page_permissions(curenv->env_page_directory, scElement->virtual_address, 0, PERM_PRESENT);
					LIST_INSERT_HEAD(&(curenv->SecondList), scElement);

					struct WorkingSetElement* newElement = env_page_ws_list_create_element(curenv, fault_va);
					pt_set_page_permissions(curenv->env_page_directory, newElement->virtual_address, PERM_PRESENT, 0);
					LIST_INSERT_HEAD(&(curenv->ActiveList), newElement);

				}

			}

			//LIST_INSERT_HEAD(&(curenv->page_WS_list), newElement);

		}
		/*replacement*/
		else
		{

			cprintf("****************entered replacement with fault_va = %x****************\n", fault_va);

			int foundSL = 0;
			int count = 0;
			struct WorkingSetElement* temp;

			LIST_FOREACH(temp, &(curenv->SecondList))
			{
				//cprintf("%d \n", count);
				//count++;
				//cprintf("loop: temp = %x\n", temp);


				if(ROUNDDOWN(temp->virtual_address, PAGE_SIZE) == ROUNDDOWN(fault_va, PAGE_SIZE))
				{
					//cprintf("found va in second list\n");

					LIST_REMOVE(&(curenv->SecondList), temp);

					struct WorkingSetElement* scElement = LIST_LAST(&(curenv->ActiveList));

					LIST_REMOVE(&(curenv->ActiveList), scElement);
					LIST_INSERT_HEAD(&(curenv->SecondList), scElement);

					LIST_INSERT_HEAD(&(curenv->ActiveList), temp);

					pt_set_page_permissions(curenv->env_page_directory,  temp->virtual_address, PERM_PRESENT, 0);
					pt_set_page_permissions(curenv->env_page_directory,  scElement->virtual_address, 0, PERM_PRESENT);

					foundSL = 1;
					break;
				}
			}
			if(!foundSL)
			{

				struct FrameInfo *fi_ptr = NULL;
				int val = allocate_frame(&fi_ptr);
				map_frame(curenv->env_page_directory, fi_ptr,fault_va, PERM_PRESENT | PERM_USER | PERM_WRITEABLE);
				fi_ptr->va = fault_va;

				val = pf_read_env_page(curenv,(void *)fault_va);
				if(val == E_PAGE_NOT_EXIST_IN_PF)
				{
					//cprintf("Entered condition E_PAGE_NOT_EXIST_IN_PF");

					if(!((fault_va < USTACKTOP && fault_va >= USTACKBOTTOM ) || (fault_va >= USER_HEAP_START && fault_va < USER_HEAP_MAX)))
					{
						sched_kill_env(curenv->env_id);
					}
				}


				struct WorkingSetElement* lastElement = LIST_LAST(&(curenv->SecondList));

				//cprintf("getting perms\n");

				uint32 pagePerm = pt_get_page_permissions(curenv->env_page_directory, lastElement->virtual_address);
				uint32* ptr_page_table = NULL;

				struct FrameInfo* frame_ptr = get_frame_info(curenv->env_page_directory, lastElement->virtual_address, &ptr_page_table);
				if(pagePerm & PERM_MODIFIED)
				{
					//cprintf("modified page\n");
					int ret = pf_update_env_page(curenv, lastElement->virtual_address, frame_ptr);
					//cprintf("updated pf\n");
				}


				LIST_REMOVE(&(curenv->SecondList), lastElement);
				//cprintf("removed from second list\n");

				if(frame_ptr != NULL)
				{
					unmap_frame(curenv->env_page_directory, lastElement->virtual_address);
					//cprintf("unmapped frame\n");
				}

				kfree(lastElement);

				struct WorkingSetElement* scElement = LIST_LAST(&(curenv->ActiveList));
				LIST_REMOVE(&(curenv->ActiveList), scElement);

				pt_set_page_permissions(curenv->env_page_directory, scElement->virtual_address, 0, PERM_PRESENT);
				LIST_INSERT_HEAD(&(curenv->SecondList), scElement);

				struct WorkingSetElement* newElement = env_page_ws_list_create_element(curenv, fault_va);
				pt_set_page_permissions(curenv->env_page_directory, newElement->virtual_address, PERM_PRESENT, 0);
				LIST_INSERT_HEAD(&(curenv->ActiveList), newElement);
			}
			//cprintf("******************DONE REPLACEMENT***************\n");

		}

		//TODO: [PROJECT'23.MS3 - BONUS] [1] PAGE FAULT HANDLER - O(1) implementation of LRU replacement
	}

}

void __page_fault_handler_with_buffering(struct Env * curenv, uint32 fault_va)
{
	panic("this function is not required...!!");
}




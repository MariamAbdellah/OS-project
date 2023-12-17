#include <inc/lib.h>

struct SizeInfoBlock_LIST USizeBlockList;
int initialized = 0;
//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//


int FirstTimeFlag = 1;
void InitializeUHeap()
{
	if(FirstTimeFlag)
	{
#if UHP_USE_BUDDY
		initialize_buddy();
		cprintf("BUDDY SYSTEM IS INITIALIZED\n");
#endif
		FirstTimeFlag = 0;
	}
}

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//

//=============================================
// [1] CHANGE THE BREAK LIMIT OF THE USER HEAP:
//=============================================
/*2023*/
void* sbrk(int increment)
{
	return (void*) sys_sbrk(increment);
}

//=================================
// [2] ALLOCATE SPACE IN USER HEAP:
//=================================

void initializeUserList()
{

	uint32 daLimit = sys_get_hard_limit();

	//cprintf("*****************************  ******* user hard Limit = %x \n", daLimit);
	//cprintf("********trying to create the first item in U-list******** \n");
	struct BlockSizeInfo *UfirstBlock;

	//cprintf("\n ****trying to find first addr with alloc_block_FF **** \n");
	//void * firstaddr = ;
	UfirstBlock = (struct BlockSizeInfo *)alloc_block_FF(sizeOfBlockInfo());
	cprintf("**888888888888888888888 user first block address = %x** \n", UfirstBlock);
	uint32 PageSegementSize = USER_HEAP_MAX - (daLimit + (uint32)PAGE_SIZE);
	UfirstBlock->va = daLimit + PAGE_SIZE;
	UfirstBlock->size = PageSegementSize;
	UfirstBlock->is_free= 1;
	//cprintf("passed \n");
	cprintf("UfirstBlock = %x, va = %x, size = %d, is_free = %d \n",
			UfirstBlock, UfirstBlock->va, UfirstBlock->size, UfirstBlock->is_free);

	LIST_INIT(&USizeBlockList);
	LIST_INSERT_HEAD(&USizeBlockList,UfirstBlock);

	initialized = 1;


}

void* malloc(uint32 size)
{

	cprintf("malloc is called with size = %d \n", size);
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	//TODO: [PROJECT'23.MS2 - #09] [2] USER HEAP - malloc() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("malloc() is not implemented yet...!!");



	//uint32 listsize = LIST_SIZE(&USizeBlockList);
	//cprintf("user size block list at malloc = %d \n", listsize);

	//cprintf("USER SIZE BLOCK LIST: [displaying items using list_foreach] \n");
	/*struct BlockSizeInfo *temptr;
	LIST_FOREACH(temptr,&USizeBlockList){
		cprintf("entered List_foreach in printing test \n");
		cprintf("temptr = %x, temptr->va = %x, temptr->size = %d, temptr->is_free = %d \n",
				temptr, temptr->va, temptr->size, temptr->is_free);
	}*/

	int is_allocated = 0;
	if(sys_isUHeapPlacementStrategyFIRSTFIT() == 1){
		if (size <= DYN_ALLOC_MAX_BLOCK_SIZE){
			//cprintf("entered block allocation \n");
			return alloc_block_FF(size);
			is_allocated = 1;
		}
		else if (size > DYN_ALLOC_MAX_BLOCK_SIZE) {

			if(!initialized)
			{
				initializeUserList();
			}

			//cprintf("entered paging allocation \n");
			size = ROUNDUP(size, PAGE_SIZE);
			unsigned int numofpages = size / PAGE_SIZE;
			struct BlockSizeInfo *temptr;
			//cprintf("trying to call list foreach \n");
			LIST_FOREACH(temptr,&USizeBlockList){
				//cprintf("entered List_foreach \n");
				//cprintf("temptr in malloc = %x", temptr);
				//cprintf("temptr->size, temptr->va \n", temptr);
				if(temptr->is_free == 1){
					//cprintf("found one free \n");
					if(temptr->size >= size){
						//cprintf("found free with enough size \n");
						if (temptr->size > size){
							//cprintf("creating new block with excess space \n");
							struct BlockSizeInfo *newBlock = (struct BlockSizeInfo*)alloc_block_FF(sizeOfBlockInfo());
							uint32 newaddress = temptr->va + size;
							newBlock->va = newaddress;
							newBlock->size = temptr->size - size;
							newBlock->is_free= 1;

							LIST_INSERT_AFTER(&USizeBlockList,temptr, newBlock);
						}
						temptr->size = size;
						temptr->is_free = 0;
						is_allocated = 1;

						sys_allocate_user_mem(temptr->va, temptr->size);
						uint32 * res = (uint32 *)temptr->va;
						return res;
					}
				}
			}
		}
	}
	//	else if(sys_isUHeapPlacementStrategyBESTFIT() == 1){
	//
	//	}
	if (is_allocated != 1){
		//cprintf("entered is allocated == 0 \n");
		return NULL;
	}
	//cprintf("didn't enter any case \n");
	return NULL;
	//Use sys_isUHeapPlacementStrategyFIRSTFIT() and	sys_isUHeapPlacementStrategyBESTFIT()
	//to check the current strategy

}

//=================================
// [3] FREE SPACE FROM USER HEAP:
//=================================

void free(void* virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #11] [2] USER HEAP - free() [User Side]
	// Write your code here, remove the panic and write your code
	//panic("free() is not implemented yet...!!");
	//cprintf("free is called with virtual_address = %x \n", virtual_address);

	uint32 hardlimit = sys_get_hard_limit();
	if ((uint32)virtual_address < hardlimit && (uint32)virtual_address >= USER_HEAP_START){
		//cprintf("entered block condition \n");
		return free_block(virtual_address);
	}
	else if ((uint32)virtual_address < USER_HEAP_MAX && (uint32)virtual_address >= (hardlimit + PAGE_SIZE)){
		//cprintf("entered pages condition \n");
		struct BlockSizeInfo *temptr;
		LIST_FOREACH(temptr,&USizeBlockList){
			if(temptr->va == (uint32)virtual_address){
				temptr->is_free=1;
				sys_free_user_mem(temptr->va, temptr->size);

				struct BlockSizeInfo* prev_ptr = LIST_PREV((struct BlockSizeInfo*) temptr);
				struct BlockSizeInfo* nxt_ptr = LIST_NEXT((struct BlockSizeInfo*) temptr);

				struct BlockSizeInfo* first = LIST_FIRST(&USizeBlockList);
				struct BlockSizeInfo* last = LIST_LAST(&USizeBlockList);

				if(temptr != last && nxt_ptr != 0){
					if(nxt_ptr->is_free==1){
						//cprintf("merge with next \n");
						struct BlockSizeInfo *oldnxt = nxt_ptr;
						uint32 old_nxt_size=nxt_ptr->size;
						temptr->size=temptr->size + old_nxt_size;
						oldnxt->is_free=0;
						oldnxt->size=0;
						oldnxt->va = 0;
						LIST_REMOVE(&USizeBlockList, oldnxt);
						free_block((void*)oldnxt);
					}
				}
				if(temptr != first && prev_ptr != 0){
					if(prev_ptr->is_free ==1){
						//cprintf("merge with previous \n");
						struct BlockSizeInfo *oldtemp = temptr;
						uint32 old_temptr_size = (temptr->size);
						temptr = prev_ptr;
						temptr->va = prev_ptr->va;
						temptr->size=(prev_ptr->size)+old_temptr_size;
						oldtemp->is_free = 0;
						oldtemp->size=0;
						oldtemp->va = 0;
						LIST_REMOVE(&USizeBlockList, oldtemp);
						free_block((void*)oldtemp);
					}
				}
			}
		}
	}
	else {
		panic("invalid address");
	}

}

//=================================
// [4] ALLOCATE SHARED VARIABLE:
//=================================
void* smalloc(char *sharedVarName, uint32 size, uint8 isWritable)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	if (size == 0) return NULL ;
	//==============================================================
	panic("smalloc() is not implemented yet...!!");
	return NULL;
}

//========================================
// [5] SHARE ON ALLOCATED SHARED VARIABLE:
//========================================
void* sget(int32 ownerEnvID, char *sharedVarName)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================
	// Write your code here, remove the panic and write your code
	panic("sget() is not implemented yet...!!");
	return NULL;
}


//==================================================================================//
//============================== BONUS FUNCTIONS ===================================//
//==================================================================================//

//=================================
// REALLOC USER SPACE:
//=================================
//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to malloc().
//	A call with new_size = zero is equivalent to free().

//  Hint: you may need to use the sys_move_user_mem(...)
//		which switches to the kernel mode, calls move_user_mem(...)
//		in "kern/mem/chunk_operations.c", then switch back to the user mode here
//	the move_user_mem() function is empty, make sure to implement it.
void *realloc(void *virtual_address, uint32 new_size)
{
	//==============================================================
	//DON'T CHANGE THIS CODE========================================
	InitializeUHeap();
	//==============================================================

	// Write your code here, remove the panic and write your code
	panic("realloc() is not implemented yet...!!");
	return NULL;

}


//=================================
// FREE SHARED VARIABLE:
//=================================
//	This function frees the shared variable at the given virtual_address
//	To do this, we need to switch to the kernel, free the pages AND "EMPTY" PAGE TABLES
//	from main memory then switch back to the user again.
//
//	use sys_freeSharedObject(...); which switches to the kernel mode,
//	calls freeSharedObject(...) in "shared_memory_manager.c", then switch back to the user mode here
//	the freeSharedObject() function is empty, make sure to implement it.

void sfree(void* virtual_address)
{
	// Write your code here, remove the panic and write your code
	panic("sfree() is not implemented yet...!!");
}


//==================================================================================//
//========================== MODIFICATION FUNCTIONS ================================//
//==================================================================================//

void expand(uint32 newSize)
{
	panic("Not Implemented");

}
void shrink(uint32 newSize)
{
	panic("Not Implemented");

}
void freeHeap(void* virtual_address)
{
	panic("Not Implemented");

}

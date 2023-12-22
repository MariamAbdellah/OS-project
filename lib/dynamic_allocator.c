/*
 * dynamic_allocator.c
 *
 *  Created on: Sep 21, 2023
 *      Author: HP
 */
#include <inc/assert.h>
#include <inc/string.h>
#include "../inc/dynamic_allocator.h"

struct MemBlock_LIST BlockList;

void printBlist()
{
	cprintf("entered List_foreach in printing test \n");
	struct BlockMetaData *temptr;
	LIST_FOREACH(temptr,&BlockList){

		cprintf("temptr = %x, temptr->size = %x, temptr->is_free = %x \n",
				temptr/*, temptr->va*/, temptr->size, temptr->is_free);
	}
}

//==================================================================================//
//============================== GIVEN FUNCTIONS ===================================//
//==================================================================================//

//=====================================================
// 1) GET BLOCK SIZE (including size of its meta data):
//=====================================================
uint32 get_block_size(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->size ;
}

//===========================
// 2) GET BLOCK STATUS:
//===========================
int8 is_free_block(void* va)
{
	struct BlockMetaData *curBlkMetaData = ((struct BlockMetaData *)va - 1) ;
	return curBlkMetaData->is_free ;
}

//===========================================
// 3) ALLOCATE BLOCK BASED ON GIVEN STRATEGY:
//===========================================
void *alloc_block(uint32 size, int ALLOC_STRATEGY)
{
	void *va = NULL;
	switch (ALLOC_STRATEGY)
	{
	case DA_FF:
		va = alloc_block_FF(size);
		break;
	case DA_NF:
		va = alloc_block_NF(size);
		break;
	case DA_BF:
		va = alloc_block_BF(size);
		break;
	case DA_WF:
		va = alloc_block_WF(size);
		break;
	default:
		cprintf("Invalid allocation strategy\n");
		break;
	}
	return va;
}

//===========================
// 4) PRINT BLOCKS LIST:
//===========================

void print_blocks_list(struct MemBlock_LIST list)
{
	cprintf("=========================================\n");
	struct BlockMetaData* blk ;
	cprintf("\nDynAlloc Blocks List:\n");
	LIST_FOREACH(blk, &list)
	{
		cprintf("(size: %d, isFree: %d)\n", blk->size, blk->is_free) ;
	}
	cprintf("=========================================\n");

}
//
////********************************************************************************//
////********************************************************************************//

//==================================================================================//
//============================ REQUIRED FUNCTIONS ==================================//
//==================================================================================//
bool is_initialized = 0;
//==================================
// [1] INITIALIZE DYNAMIC ALLOCATOR:
//==================================
void initialize_dynamic_allocator(uint32 daStart, uint32 initSizeOfAllocatedSpace)
{
	//cprintf("dynamic allocator initialization is called \n");


   //=========================================
   //DON'T CHANGE THESE LINES=================
   if (initSizeOfAllocatedSpace == 0)
   {
	   return ;
   }
   is_initialized = 1;
   //=========================================
   //=========================================

   //TODO: [PROJECT'23.MS1 - #5] [3] DYNAMIC ALLOCATOR - initialize_dynamic_allocator()

    struct BlockMetaData *firstBlock = (struct BlockMetaData *)daStart;



    //cprintf("firstBlock->is_free = %d\nfirstBlock->size = %d\n", firstBlock->is_free, initSizeOfAllocatedSpace);

    //cprintf("dastart = %x \n", daStart);
    //cprintf("initSizeOfAllocatedSpace = %d \n", initSizeOfAllocatedSpace);


    firstBlock->is_free = 1;
    firstBlock->size = initSizeOfAllocatedSpace;

    //cprintf("dastart = %x \n", daStart);
    //cprintf("initSizeOfAllocatedSpace = %d \n", initSizeOfAllocatedSpace);

    //cprintf("firstblock address = %x \n", firstBlock);
    //cprintf("firstblock size = %d \n", firstBlock->size);

    LIST_INIT(&BlockList);
    LIST_INSERT_HEAD(&BlockList,firstBlock);

    //panic("initialize_dynamic_allocator is not implemented yet");

}

//=========================================
// [4] ALLOCATE BLOCK BY FIRST FIT:
//=========================================

void *alloc_block_FF(uint32 size)
{
    //TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()
	//cprintf("entered alloc_block_FF \n");

    uint32 required_size=size+sizeOfMetaData();

    uint8 allocated=0;

    struct BlockMetaData *temptr=(struct BlockMetaData *)KERNEL_HEAP_START;


    //void* breakable=sbrk(required_size);
    //cprintf("sbrk called first time \n");

    if(size==0){
        return NULL;
    }

    if (!is_initialized)
    {

    //cprintf("entered not initialized \n");
    uint32 required_size = size + sizeOfMetaData();
    uint32 da_start = (uint32)sbrk(required_size);
    //get new break since it's page aligned! thus, the size can be more than the required one
    uint32 da_break = (uint32)sbrk(0);
    //cprintf("start address of alloctaed = %x\n", da_start);
    //cprintf("segment break (data end) = %x\n", da_break);
    //cprintf("sbrk allocated size = %d\n", required_size);
    initialize_dynamic_allocator(da_start, da_break - da_start);
    }

    LIST_FOREACH(temptr,&BlockList){
    	//cprintf("entered loop \n");
    	//cprintf ("temptr = %x \n", temptr);
        if(temptr->is_free==1){
            if(temptr->size == required_size){
            	//cprintf("entered = condition in ff \n");
                temptr->is_free=0;
                temptr->size=required_size;
                allocated=1;
                uint32 test = (uint32)temptr +sizeOfMetaData();
                struct BlockMetaData *res;
                res= (struct BlockMetaData *)test;
                //cprintf("result of block_FF = %x \n", res);
               return res;
            }

            else if(temptr->size> required_size){
            	//cprintf("entered > condition in ff \n");
            	uint32 remainingsize = (temptr->size) - required_size;

            	//cprintf("tmptr size = %x\trequired size = %x\tremaining size = %x\n", temptr->size, required_size, remainingsize);

            	if(remainingsize >= sizeOfMetaData()){
                    struct BlockMetaData *newblockk;
                    uint32 temp = (uint32)temptr + required_size;
                    newblockk= (struct BlockMetaData *)temp;

                    newblockk->is_free = 1;
                    newblockk->size = (temptr->size)-required_size;

                    //TODO:insert to the list

                    LIST_INSERT_AFTER(&BlockList,temptr, newblockk);

                    temptr->size=required_size;
            	}

                temptr->is_free=0;

                allocated=1;

                uint32 test = (uint32)temptr +sizeOfMetaData();
                struct BlockMetaData *res;
                res= (struct BlockMetaData *)test;
                //cprintf("result of block_FF = %x \n", res);
                return res;
            }
        }
    }

    if(allocated==0){
    	//cprintf("entered sbrk else in ff \n");
    	//size = ROUNDUP(size, PAGE_SIZE);
    	size = ROUNDUP(required_size, PAGE_SIZE);
    	//cprintf("required_size = %d \n", required_size);
    	//cprintf("rounded up size = %d \n", size);
    	int numofpages = size / PAGE_SIZE;

    	//cprintf("numofpages added = %d \n", numofpages);
    	void* sbrk_return = sbrk(required_size);
    	//cprintf("sbrk_return = %x \n", sbrk_return);

    	if (sbrk_return == (void *)-1){
    		//cprintf("entered invalid sbrk size \n");
    		return NULL;
    	}
    	else {
    		//cprintf("entered valid sbrk size \n");
    		temptr= (struct BlockMetaData *)sbrk_return;
    		//cprintf("temptr after calling sbrk = %x \n", temptr);
    		temptr->is_free=0;
    		temptr->size=required_size;

    		struct BlockMetaData* last = LIST_LAST(&BlockList);
    		LIST_INSERT_AFTER(&BlockList,last, temptr);

    		uint32 remainingsize = (numofpages * PAGE_SIZE) - required_size;
    		//cprintf("reamingsize should be = %d \n", size - required_size);
    		//cprintf("remainingsize actual = %d \n", remainingsize);
    		if(remainingsize >= sizeOfMetaData()){
    			//cprintf("entered spliting condition \n");
    			struct BlockMetaData *newblockk;
    			//cprintf("passed \n");
    			uint32 temp = (uint32)temptr + required_size;
    			newblockk= (struct BlockMetaData *)temp;
    			//cprintf("passed \n");

    			newblockk->is_free = 1;
    			newblockk->size = remainingsize;

    			//cprintf("address of newblock = %x \n", newblockk);
    			//cprintf("size of newblock = %d \n", newblockk->size);
    			//cprintf("address after newblock = %x \n", newblockk + required_size);

    			//problem with insert into list
    			//cprintf("trying to insert into list \n");
    			LIST_INSERT_AFTER(&BlockList,temptr, newblockk);
    			//cprintf("passed \n");
    		}
    		else {
    			temptr->size= required_size + remainingsize;
    		}

    		uint32 sum = (uint32)temptr +sizeOfMetaData();
    		struct BlockMetaData *res;
    		res= (struct BlockMetaData *)sum;
    		//cprintf("result of sbrk in block_ff = %x \n", res);
    		return res;
    	}
    }

    return NULL;
}


//=========================================
// [5] ALLOCATE BLOCK BY BEST FIT:
//=========================================
void *alloc_block_BF(uint32 size)
{
    //TODO: [PROJECT'23.MS1 - #6] [3] DYNAMIC ALLOCATOR - alloc_block_FF()

    uint32 required_size=size+sizeOfMetaData();

    uint8 allocated=0;
    uint8 foundBF=0;

    struct BlockMetaData *temptr=(struct BlockMetaData *)KERNEL_HEAP_START;
    struct BlockMetaData *bfptr=(struct BlockMetaData *)KERNEL_HEAP_START;

    uint32 mindif=USER_LIMIT;


    void* breakable=sbrk(required_size);

    if(size==0){
        return NULL;
    }

    if (!is_initialized)
    {
        uint32 required_size = size + sizeOfMetaData();
        uint32 da_start = (uint32)sbrk(required_size);
        //get new break since it's page aligned! thus, the size can be more than the required one
        uint32 da_break = (uint32)sbrk(0);
        initialize_dynamic_allocator(da_start, da_break - da_start);
    }

    LIST_FOREACH(temptr,&BlockList)
        {
            if(temptr->is_free==1)
            {
                if(temptr->size == required_size)
                {
                    temptr->is_free=0;
                    temptr->size=required_size;
                    allocated=1;
                    uint32 test = (uint32)temptr +sizeOfMetaData();
                    struct BlockMetaData *res;
                    res= (struct BlockMetaData *)test;
                   return res;
                }
            }
        }

    LIST_FOREACH(temptr,&BlockList)
    {
    	 if(temptr->is_free==1)
    	 {
    		 if(temptr->size > required_size)
    		 {
    		  uint32 remainingsize = (temptr->size)-required_size;
    		  if(remainingsize < mindif)
    			  {
    				  bfptr=temptr;
    				  mindif=remainingsize;
     				  foundBF=1;
    			  }
    	    }
    	 }
    }

    if(foundBF==1){
    	uint32 remainingsize = bfptr->size - required_size;

    	if(remainingsize>=sizeOfMetaData()){
    		struct BlockMetaData *newblockk;
    		uint32 temp = (uint32)bfptr + required_size;
    		newblockk= (struct BlockMetaData *)temp;
    		newblockk->is_free = 1;
    		newblockk->size = (bfptr->size)-required_size;
    		LIST_INSERT_AFTER(&BlockList,bfptr, newblockk);
    		bfptr->size=required_size;
    	}

    	bfptr->is_free=0;
    	allocated=1;

    	uint32 test = (uint32)bfptr +sizeOfMetaData();
    	struct BlockMetaData *res;
    	res= (struct BlockMetaData *)test;
    	return res;

    }

    if(allocated==0){
        if(breakable == (void *)-1){
            return NULL;
        }
        else{
        	size = ROUNDUP(size, PAGE_SIZE);
        	int numofpages = size / PAGE_SIZE;

        	temptr=sbrk(required_size);
        	temptr->is_free=0;
        	temptr->size=required_size;

        	uint32 remainingsize = (numofpages * PAGE_SIZE) - required_size;
        	if(remainingsize >= sizeOfMetaData()){
        		struct BlockMetaData *newblockk;
        		uint32 temp = (uint32)temptr + required_size;
        		newblockk= (struct BlockMetaData *)temp;

        		newblockk->is_free = 1;
        		newblockk->size = remainingsize;


        		LIST_INSERT_AFTER(&BlockList,temptr, newblockk);
        	}
        	else {
        		temptr->size= required_size + remainingsize;
        	}

        	uint32 sum = (uint32)temptr +sizeOfMetaData();
        	struct BlockMetaData *res;
        	res= (struct BlockMetaData *)sum;
        	return res;
        }
    }

    return NULL;
}

//=========================================
// [6] ALLOCATE BLOCK BY WORST FIT:
//=========================================
void *alloc_block_WF(uint32 size)
{
	panic("alloc_block_WF is not implemented yet");
	return NULL;
}

//=========================================
// [7] ALLOCATE BLOCK BY NEXT FIT:
//=========================================
void *alloc_block_NF(uint32 size)
{
	panic("alloc_block_NF is not implemented yet");
	return NULL;
}

//===================================================
// [8] FREE BLOCK WITH COALESCING:
//===================================================
void free_block(void *va)
{
	//TODO: [PROJECT'23.MS1 - #7] [3] DYNAMIC ALLOCATOR - free_block()
	//panic("free_block is not implemented yet");
	//cprintf("free block is called with address %x \n", va);
	    struct BlockMetaData *temptr=(struct BlockMetaData *)KERNEL_HEAP_START;

	    if(va==0){
	        return;
	    }



    LIST_FOREACH(temptr,&BlockList){

    	uint32 sum = (uint32)temptr +sizeOfMetaData();
    	struct BlockMetaData *res;
    	res= (struct BlockMetaData *)sum;
        if(res == va ){

	        temptr->is_free = 1;
	        //cprintf("tmptr address %x\n", res);
	        //cprintf("tmptr->is_free = %d\n", temptr->is_free);
	        struct BlockMetaData* prev_ptr = LIST_PREV((struct BlockMetaData*) temptr);
	        struct BlockMetaData* nxt_ptr = LIST_NEXT((struct BlockMetaData*) temptr);

	        struct BlockMetaData* first = LIST_FIRST(&BlockList);
	        struct BlockMetaData* last = LIST_LAST(&BlockList);

	        if(temptr != last && nxt_ptr != 0){
				if(nxt_ptr->is_free==1){
					//cprintf("merging with next\n");
					struct BlockMetaData *oldnxt = nxt_ptr;
					uint32 old_nxt_size=nxt_ptr->size;
					temptr->size=temptr->size + old_nxt_size;
					oldnxt->is_free=0;
					oldnxt->size=0;
					LIST_REMOVE(&BlockList, oldnxt);
				}
	        }
	        if(temptr != first && prev_ptr != 0){
	        	if(prev_ptr->is_free ==1){
	        		struct BlockMetaData *oldtemp = temptr;
	        		uint32 old_temptr_size = (temptr->size);
	        		temptr = prev_ptr;
	        		temptr->size=(prev_ptr->size)+old_temptr_size;
	        		oldtemp->is_free = 0;
	        		oldtemp->size=0;
	        		LIST_REMOVE(&BlockList, oldtemp);
	        	}
	        }
        }
    }
}


//=========================================
// [4] REALLOCATE BLOCK BY FIRST FIT:
//=========================================
void *realloc_block_FF(void* va, uint32 new_size)
{
	//TODO: [PROJECT'23.MS1 - #8] [3] DYNAMIC ALLOCATOR - realloc_block_FF()
	//panic("realloc_block_FF is not implemented yet");

	if (va == 0){
		return alloc_block_FF(new_size);
	}
	else if (new_size == 0){
		free_block(va);
		return NULL;
	}

	struct BlockMetaData *temptr=(struct BlockMetaData *)KERNEL_HEAP_START;
	uint32 requested_size=new_size+sizeOfMetaData();
	LIST_FOREACH(temptr,&BlockList){
		uint32 sum = (uint32)temptr + sizeOfMetaData();
		struct BlockMetaData *res=(struct BlockMetaData *)sum;
		if(res == va){
			struct BlockMetaData* nxt_ptr = LIST_NEXT((struct BlockMetaData*) temptr);
			struct BlockMetaData* last = LIST_LAST(&BlockList);

			//cprintf("res %x \n", res);
			//cprintf("temptr %x \n", temptr);
			if(temptr->size > requested_size){
				uint32 remainingsize = (temptr->size)-requested_size;
				if(remainingsize >= sizeOfMetaData()){
					struct BlockMetaData *newblock;
					uint32 newaddress = (uint32)temptr + requested_size;
					newblock= (struct BlockMetaData *)newaddress;
					newblock->is_free = 1;
					newblock->size = (temptr->size)-requested_size;
					LIST_INSERT_AFTER(&BlockList,temptr, newblock);
					temptr->size = requested_size;

					//free_block(newblock);
					if(temptr != last && nxt_ptr != 0){
						if(nxt_ptr->is_free==1){
							//cprintf("entered the nxt is free in smaller size \n");
							struct BlockMetaData *oldnxt = nxt_ptr;
							uint32 old_nxt_size=nxt_ptr->size;
							newblock->size=(newblock->size) + old_nxt_size;
							oldnxt->is_free=0;
							oldnxt->size=0;
							LIST_REMOVE(&BlockList, oldnxt);
						}
					}
				}
				temptr->is_free = 0;
				return res;
			}
			else if(temptr->size == requested_size){
				//cprintf("entered == case \n");
				temptr->is_free = 0;
				return res;
			}
			else if(temptr->size < requested_size){
				uint8 allocated = 0;
				//cprintf("entered second condition \n");
				if(temptr != last && nxt_ptr != 0){
					uint32 avaliableArea = (temptr->size) +(nxt_ptr->size);
					//cprintf("entered nxt \n");
					if(nxt_ptr->is_free==1 && avaliableArea >= requested_size){
							temptr->size= avaliableArea;
							temptr->is_free = 0;

							struct BlockMetaData *oldnxt = nxt_ptr;
							uint32 remainingsize = avaliableArea - requested_size;
							if(requested_size < avaliableArea && remainingsize >= sizeOfMetaData()){
									//cprintf("entered split in nxt \n");
									temptr->size= requested_size;

									uint32 newnxtaddr = (uint32)temptr + requested_size;
									nxt_ptr=(struct BlockMetaData *)newnxtaddr;
									nxt_ptr->is_free=1;
									nxt_ptr->size=avaliableArea - requested_size;
							}
							oldnxt->is_free=0;
							oldnxt->size=0;
							LIST_REMOVE(&BlockList, oldnxt);
							allocated = 1;
							return res;
					}
				}
				if(allocated == 0){
					//cprintf("entered the reallocate stage \n");
					uint8 freed = 0;
					struct BlockMetaData* prev_ptr = LIST_PREV((struct BlockMetaData*) temptr);
					struct BlockMetaData* first = LIST_FIRST(&BlockList);
					if (temptr != first && prev_ptr != 0){
						if(prev_ptr->is_free ==1){
							uint32 totalsize = (temptr->size) + (prev_ptr->size);
							if(temptr != last && nxt_ptr != 0){
								if(nxt_ptr->is_free==1){
									totalsize += (nxt_ptr->size);
								}
							}
							if (totalsize >= requested_size){
									free_block(va);
									freed = 1;
							}
						}
					}
					struct BlockMetaData *allocateRes = alloc_block_FF(new_size);
					if (freed == 0) {
						if (allocateRes != NULL){
							free_block(va);
						}
					}
					return allocateRes;
				}
			}
		}
	}
	return NULL;
}

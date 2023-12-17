#include "kheap.h"

#include <inc/memlayout.h>
#include <inc/dynamic_allocator.h>
#include "memory_manager.h"

struct SizeInfoBlock_LIST SizeBlockList;


int initialize_kheap_dynamic_allocator(uint32 daStart, uint32 initSizeToAllocate, uint32 daLimit)
{
	//TODO: [PROJECT'23.MS2 - #01] [1] KERNEL HEAP - initialize_kheap_dynamic_allocator()
	//Initialize the dynamic allocator of kernel heap with the given start address, size & limit
	//All pages in the given range should be allocated
	//Remember: call the initialize_dynamic_allocator(..) to complete the initialization
	//Return:
	//	On success: 0
	//	Otherwise (if no memory OR initial size exceed the given limit): E_NO_MEM

	//Comment the following line(s) before start coding...
	//panic("not implemented yet");

	kheapStart = daStart;
		brk = initSizeToAllocate + daStart;
		hardLimit = daLimit;
		//case no memory
		//size > limit
		if(brk > daLimit)
		{
			return E_NO_MEM;
		}

		for(uint32 i = kheapStart; i < brk; i +=PAGE_SIZE)
		{
			struct FrameInfo *frame_ptr;
			int ret = allocate_frame(&frame_ptr);
			if(ret !=0){
				return E_NO_MEM;
			}
			ret = map_frame(ptr_page_directory, frame_ptr, i, PERM_PRESENT | PERM_WRITEABLE);
			if(ret != 0){
				return E_NO_MEM;
			}
			frame_ptr->va = i;///////////////////////

		}

		initialize_dynamic_allocator(daStart, initSizeToAllocate);
		//cprintf("after calling dynamic allocator initialization \n");

		//cprintf("before initializing sizeblock_list \n");


		//uint32 sizeblockliststart = hardLimit;




		//cprintf("after initialzing sizeblock_list \n");
		//cprintf("max size of sizeblocklist = %d", sizeOfBlockInfo() * firstBlock->numofpagesleft);
		return 0;
}

void* sbrk(int increment)
{

	uint32 old_brk = brk; //ROUNDUP(brk, PAGE_SIZE);

	//cprintf("in kernel sbrk\n");


	if(increment > 0)
	{
		//cprintf("in increment\n");

		brk = brk + increment;
		brk = ROUNDUP(brk, PAGE_SIZE);

		if(brk > hardLimit)
		{
			panic("increment exceed hard limit!!");
			return (void *)old_brk;
		}

		for(int i = old_brk; i < brk; i += PAGE_SIZE)
		{
			struct FrameInfo *frame_ptr;
			int aloc = allocate_frame(&frame_ptr);
			if(aloc!=0)
			{
				panic("can't allocate any more frames!\n");
//				return (void *)-1;
			}

			int map = map_frame(ptr_page_directory, frame_ptr, i, PERM_PRESENT | PERM_WRITEABLE);

			if(map != 0)
			{
				panic("can't map frame to virtual address!\n");
			}
			frame_ptr->va = i;
		}

		return (void *)old_brk;
	}
	else if(increment < 0)
	{

		//cprintf("in decrement\n");

		brk = increment + old_brk;

		if(brk < kheapStart)
		{
			panic("break is below kheap start");
			return (void *)-1;
		}
		else
		{
			uint32 start_add = ROUNDUP(brk, PAGE_SIZE);
			for(int i = start_add; i < old_brk; i += PAGE_SIZE)
			{

				uint32* ptr_page_table = NULL;
				struct FrameInfo* frame_ptr = get_frame_info(ptr_page_directory, i, &ptr_page_table);

				if( frame_ptr != 0 )
				{
					unmap_frame(ptr_page_directory, i);
					frame_ptr->va = 0;
				}
			}
			return (void *)brk;
		}
	}
	else
	{
		//cprintf("in inc = 0 \n");
		return (void*)brk;
	}

}
int kheepInitialized = 0;
void initializeKheap()
{
	struct BlockSizeInfo *firstBlock;
	void * firstaddr = alloc_block_FF(sizeOfBlockInfo());
	firstBlock = (struct BlockSizeInfo *)firstaddr;


	uint32 PageSegementSize = KERNEL_HEAP_MAX - (hardLimit + (uint32)PAGE_SIZE);
	firstBlock->numofpagesleft = PageSegementSize / PAGE_SIZE;
	firstBlock->va = hardLimit + PAGE_SIZE;
	//firstBlock->size = PAGE_SIZE;
	firstBlock->size = PageSegementSize;
	firstBlock->is_free= 1;

	//cprintf("firstblock in sizeblock-list = %d \n", firstBlock);

	LIST_INIT(&SizeBlockList);
	LIST_INSERT_HEAD(&SizeBlockList,firstBlock);
	kheepInitialized = 1;
}



void* kmalloc(unsigned int size)
{

	if(!kheepInitialized)
	{
		initializeKheap();
	}

	cprintf("kmalloc is called with size = %d \n", size);
	//TODO: [PROJECT'23.MS2 - #03] [1] KERNEL HEAP - kmalloc()
	//refer to the project presentation and documentation for details
	// use "isKHeapPlacementStrategyFIRSTFIT() ..." functions to check the current strategy
	int is_allocated = 0;
	if (isKHeapPlacementStrategyFIRSTFIT() == 1){
		if (size <= DYN_ALLOC_MAX_BLOCK_SIZE){
			//cprintf("entered block allocation \n");
			return alloc_block_FF(size);
			is_allocated = 1;
		}
		else if (size > DYN_ALLOC_MAX_BLOCK_SIZE) {
			size = ROUNDUP(size, PAGE_SIZE);
			unsigned int numofpages = size / PAGE_SIZE;
			//the issue is the need to initialize the address of temptr
			struct BlockSizeInfo *temptr;

			LIST_FOREACH(temptr,&SizeBlockList){
				if(temptr->is_free == 1){
					if(temptr->size >= size){

						uint32 va = temptr->va;
						uint32 endofblock = (temptr->va) + size;


						for (va; va < endofblock; va+= PAGE_SIZE){

							uint32 * ptr_page_table = NULL;
							int tablestate = get_page_table(ptr_page_directory, va, &ptr_page_table);
							if (tablestate != TABLE_IN_MEMORY){
								create_page_table(ptr_page_directory, va);
							}
							uint32 *ptr_table = NULL;/////////////////////
							struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, va, &ptr_table);
							if (ptr_frame_info != NULL){
								continue;
							}
							int ret = allocate_frame(&ptr_frame_info);
							ret = map_frame(ptr_page_directory, ptr_frame_info, va, PERM_PRESENT | PERM_WRITEABLE);
							if (ret != 0){
								return NULL;
							}
							ptr_frame_info->va = va;

						}
						if (temptr->size > size){
							//cprintf("adding new item in sizeblocklist \n");
							struct BlockSizeInfo *newBlock = (struct BlockSizeInfo*)alloc_block_FF(sizeOfBlockInfo());


							uint32 newaddress = temptr->va + (uint32)size;
							newBlock->va = newaddress;
							newBlock->size = temptr->size - size;
							newBlock->is_free= 1;
							//newBlock->numofpagesleft = newBlock->size / PAGE_SIZE;

							LIST_INSERT_AFTER(&SizeBlockList,temptr, newBlock);
						}
						temptr->size = size;
						temptr->is_free = 0;
						//temptr->numofpagesleft = temptr->size / PAGE_SIZE;
						is_allocated = 1;

						uint32 * res = (uint32 *)temptr->va;
						//cprintf("kmalloc return %x \n", res);
						return res;
					}
				}
			}
		}
		if (is_allocated != 1){
			return NULL;
		}
	}
	//change this "return" according to your answer
	//kpanic_into_prompt("kmalloc() is not implemented yet...!!");
	return NULL;
}

void kfree(void* virtual_address)
{
	cprintf("free is called with va = %x \n", virtual_address);
	//TODO: [PROJECT'23.MS2 - #04] [1] KERNEL HEAP - kfree()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kfree() is not implemented yet...!!");

	int freed = 0;

	//if ((uint32)virtual_address >= KERNEL_HEAP_START){
	if((uint32)virtual_address < hardLimit && (uint32)virtual_address >= KERNEL_HEAP_START){
		//cprintf("entered free_block \n");
		return free_block(virtual_address);
		//freed = 1;
	}
	else if ((uint32)virtual_address < KERNEL_HEAP_MAX && (uint32)virtual_address >= (hardLimit + PAGE_SIZE)){
		struct BlockSizeInfo *temptr;
		LIST_FOREACH(temptr,&SizeBlockList){
			if(temptr->va == (uint32)virtual_address){
				temptr->is_free=1;
				//cprintf("marked segement of pages as free at va = %x and size = %d \n", temptr->va, temptr->size);
				uint32 endofblock = (temptr->va) + (temptr->size);
				uint32 va = temptr->va;
				for (va; va < endofblock; va+= PAGE_SIZE){
					//cprintf("freeing pages at va = %x \n", va);
					//if (numofpages != 0){
					uint32 *ptr_page_table;
					struct FrameInfo* ptr_frame_info = get_frame_info(ptr_page_directory, va, &ptr_page_table);
					if( ptr_frame_info != 0 ){
						unmap_frame(ptr_page_directory, va);
						ptr_frame_info->va = 0;
					}
//					if (ptr_frame_info->references == 0){
//						free_frame(ptr_frame_info);
//					}
					//numofpages--;
					//}
				}


				struct BlockSizeInfo* prev_ptr = LIST_PREV((struct BlockSizeInfo*) temptr);
				struct BlockSizeInfo* nxt_ptr = LIST_NEXT((struct BlockSizeInfo*) temptr);

				struct BlockSizeInfo* first = LIST_FIRST(&SizeBlockList);
				struct BlockSizeInfo* last = LIST_LAST(&SizeBlockList);

				if(temptr != last && nxt_ptr != 0){
					if(nxt_ptr->is_free==1){
						//cprintf("merging with next \n");
						struct BlockSizeInfo *oldnxt = nxt_ptr;
						uint32 old_nxt_size=nxt_ptr->size;
						temptr->size=temptr->size + old_nxt_size;
						//temptr->numofpagesleft = temptr->size / PAGE_SIZE;
						oldnxt->is_free=0;
						oldnxt->size=0;
						oldnxt->numofpagesleft = 0;
						oldnxt->va = 0;
						LIST_REMOVE(&SizeBlockList, oldnxt);
						free_block((void *)oldnxt);
					}
				}
				if(temptr != first && prev_ptr != 0){
					if(prev_ptr->is_free ==1){
						//cprintf("merging with previous \n");
						struct BlockSizeInfo *oldtemp = temptr;
						unsigned int old_temptr_size = (temptr->size);
						temptr = prev_ptr;
						temptr->va = prev_ptr->va;
						temptr->size=(prev_ptr->size)+old_temptr_size;
						oldtemp->is_free = 0;
						oldtemp->size=0;
						oldtemp->numofpagesleft = 0;
						oldtemp->va = 0;
						LIST_REMOVE(&SizeBlockList, oldtemp);
						free_block((void *)oldtemp);
					}
				}

				temptr->numofpagesleft = temptr->size / PAGE_SIZE;
				unsigned int numofpages = temptr->numofpagesleft;
			}
		}
	}
	//}
	else {
		panic("invalid address \n");
	}
//	if (freed != 1){
//		panic("invalid address");
//	}

}

unsigned int kheap_virtual_address(unsigned int physical_address)
{
	//TODO: [PROJECT'23.MS2 - #05] [1] KERNEL HEAP - kheap_virtual_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_virtual_address() is not implemented yet...!!");

	//EFFICIENT IMPLEMENTATION ~O(1) IS REQUIRED ==================

	uint32 *ptr_page_table=NULL;
	uint32 offset=(physical_address)&(0x00000fff);

	uint32 framenum=physical_address>>12;

	struct FrameInfo *ptr_frame_info;
	ptr_frame_info = to_frame_info(physical_address);

	get_page_table(ptr_page_directory,(ptr_frame_info->va),&ptr_page_table);

	if(ptr_page_table==NULL)
	{
		return 0;
	}

	return (ptr_frame_info->va)+offset;

}

unsigned int kheap_physical_address(unsigned int virtual_address)
{
	//TODO: [PROJECT'23.MS2 - #06] [1] KERNEL HEAP - kheap_physical_address()
	//refer to the project presentation and documentation for details
	// Write your code here, remove the panic and write your code
	//panic("kheap_physical_address() is not implemented yet...!!");

	uint32 *ptr_page_table=NULL;

	uint32 offset=(virtual_address)&(0x00000fff);

	uint32 dir_entry=ptr_page_directory[PDX(virtual_address)];

	get_page_table(ptr_page_directory,virtual_address,&ptr_page_table);

	if(ptr_page_table==NULL)
	{
		return 0;
	}

	int frameNum=ptr_page_table[PTX(virtual_address)]>>12;

	return (frameNum*PAGE_SIZE + offset);

	//struct FrameInfo *framenum  = get_frame_info(ptr_page_directory,virtual_address,&ptr_page_table);
	//return to_physical_address(frameNum);

}


void kfreeall()
{
	panic("Not implemented!");

}

void kshrink(uint32 newSize)
{
	panic("Not implemented!");
}

void kexpand(uint32 newSize)
{
	panic("Not implemented!");
}




//=================================================================================//
//============================== BONUS FUNCTION ===================================//
//=================================================================================//
// krealloc():

//	Attempts to resize the allocated space at "virtual_address" to "new_size" bytes,
//	possibly moving it in the heap.
//	If successful, returns the new virtual_address, in which case the old virtual_address must no longer be accessed.
//	On failure, returns a null pointer, and the old virtual_address remains valid.

//	A call with virtual_address = null is equivalent to kmalloc().
//	A call with new_size = zero is equivalent to kfree().

void *krealloc(void *virtual_address, uint32 new_size)
{
	//TODO: [PROJECT'23.MS2 - BONUS#1] [1] KERNEL HEAP - krealloc()
	// Write your code here, remove the panic and write your code
	return NULL;
	panic("krealloc() is not implemented yet...!!");
}

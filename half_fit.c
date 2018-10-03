#include "half_fit.h"
#include <lpc17xx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include "uart.h"
#include "type.h"


// REVEICES BUCKET INDEX AND RETURNS BIT MASK TO USE WITH BIT VECTOR FOR INDEX
// index: bucket index for which we want a bit mask
// *****************************************************************************
U16 get_Bucket_Bit_Mask(int index)
{
	switch (index)
	{
		case 0: return BUCKET0;
		break;
		case 1: return BUCKET1;
		break;
		case 2: return BUCKET2;
		break;
		case 3: return BUCKET3;
		break;
		case 4: return BUCKET4;
		break;
		case 5: return BUCKET5;
		break;
		case 6: return BUCKET6;
		break;
		case 7: return BUCKET7;
		break;
		case 8: return BUCKET8;
		break;
		case 9: return BUCKET9;
		break;
		case 10: return BUCKET10;
		break;
	}
	
	return NULL;
}
	


// REVEICES SIZE OF REQ MEMORY AND RETURNS INDEX OF SUGGESTED BUCKET
// size: size in chunks
// *****************************************************************************
int get_Bucket_Index( int size)
{
	return ceil( (log(size)/log(2)) );
}

// REVEICES SIZE OF UNNALOCATED MEMORY AND RETURNS INDEX OF BUCKET IT WOULD BE FOUND IN
// size: size in bytes
// *****************************************************************************
int get_Bucket_Index_Unalloc( int size)
{
		return floor( (log(size)/log(2)) );
}


// RECEIVES AN INDEX OF "RECOMMENDED BUCKET" AND RETURN INDEX OF NEXT LARGEST BUCKET THAT IS NOT EMPTY
// index: index of "recommended" bucket
// *****************************************************************************
int find_avail_bucket( int index)
{
	//printf("In find_avail_bucket index is %d bitvector is %d \n", index, bit_vector);
	if (index <= 10)
	{
		if (bit_vector & get_Bucket_Bit_Mask(index) )
		{
			return index;
		}
		else 
		{
			return find_avail_bucket(index + 1);
		}
	}

	//printf("ERROR! Trying to request bucket index that is too large");
	return 99999;
}


// RECEIVES AN ADDRESS AND RETURNS AN ADDRESS THAT HAS BEEN MOVED FORWARD
// init_Address: initial address
// mem_Jump: amount to move forward in bytes
// direction: direction to move; 1= forward, 0= backward
// *****************************************************************************
char *move_Address(char* init_Address, unsigned int mem_Jump, unsigned int direction)
{
	char *new_Address;
	if (direction == 1)
		new_Address = init_Address + mem_Jump;
	else
		new_Address = init_Address - mem_Jump;
	return new_Address;
}


// DELETES A NODE FROM THE BUCKET NODES LINKED LIST
// node: node to be deleted 
// index: index of bucket to which node belongs
// *****************************************************************************
void bucket_Linked_List_Del_Node(block_header_unalloc *node, unsigned int index)
{

	block_header_unalloc *next_node;
	block_header_unalloc *prev_node;

	if (node->prev == BEFORE_LIST && node->next == AFTER_LIST)
	{
		// HANDLE CASE OF NODE IS ONLY ONE IN LIST
		bucket_array[index] = NULL;
		bit_vector &= ~(get_Bucket_Bit_Mask(index));
		return;
	}

	if (node->prev == BEFORE_LIST)
	{
		// HANDLE CASE IF NODE IS FIRST IN LIST
		next_node = (block_header_unalloc *) move_Address((char *) (int_To_Mem_Address(node->next)), 4, 1);
		next_node->prev = BEFORE_LIST;
		bucket_array[index] = (block_header_t*) node->next;
		return;
	}

	if (node->next == AFTER_LIST)
	{
		// HANDLE CASE IF NODE IS LAST IN LIST
		prev_node = (block_header_unalloc *) move_Address((char*) (int_To_Mem_Address(node->prev)), 4, 1);
		prev_node->next = AFTER_LIST;
		return;
	}

	// HANDLE CASE IF NODE IS IN MIDDLE OF LIST
	next_node = (block_header_unalloc *) move_Address((char*) (int_To_Mem_Address(node->next)), 4, 1);
	prev_node = (block_header_unalloc *) move_Address((char*) (int_To_Mem_Address(node->prev)), 4, 1);

	next_node->prev = node->prev;
	prev_node->next = node->next;

	return;
}

// CHANGES ADDRESSES TO 10 BIT U32 FOR USE WITH PREV AND NEXT IN HEADER
// curr_Addr: address to change 
// *****************************************************************************
U32 mem_Address_To_Int(void* curr_Addr)
{
	int temp_int = (U32)curr_Addr;  //((U32)curr_Addr) & ~((U32)START_ADDR);
	int temp_int2 = (U32)START_ADDR;
	int temp_int3 = temp_int & ~temp_int2;
	return (temp_int >> 5);
}


// CHANGES U32 TO ADDRESSES
// value: value to be changed 
// *****************************************************************************
block_header_t* int_To_Mem_Address(unsigned int value){
	return (block_header_t*)( (value <<5 )|((U32)START_ADDR) );
}


// HANDLES THE ALLOCATION OF MEMORY GIVEN THE BUCKET INDEX AND SIZE IN BYTES AND CHUNKS
// index: index of bucket from which we are allocating
// size: size of memory to be allocated in bytes (inclusive of the 4 byte header)
// required_chunk: size / 32
// *****************************************************************************
void *handle_Mem_Alloc(int index, int size, int required_chunk)
{
	

	//A pointer to the block we plan on returning
	block_header_t *curr_Block;

	// NEW HEADER FOR REMAINDING MEMORY
	block_header_t *new_Block;

	// USED TO HANDLE NODE OF OLD UNALLOCATED CHUNK AND NEWLY UNALLOCATED CHUNK
	block_header_unalloc *old_bucket_Node;
	block_header_unalloc *new_bucket_Node;
	
	int new_index;
	int initial_Size;
	
	
	curr_Block = bucket_array[index];
	
	//printf (" in address %p \n",curr_Block);
	
	initial_Size = curr_Block->size;
	// set initial size to 1024 is stored size is 0; because we cannot store 1024 in 10 bits so 0 = 1024.. block will never have a size 0 anyways
	if (initial_Size == 0)
	{
		initial_Size = 1024;
	}
	
	// HANDLE IF THE WE ARE ALLOCATING THE ENTIRE UNALLOCATED BLOCK
	if (required_chunk == initial_Size)
	{
		curr_Block->is_alloc = 1;
		old_bucket_Node = (block_header_unalloc *) move_Address((char*) curr_Block, 4, 1);
		bucket_Linked_List_Del_Node(old_bucket_Node, index);
		return (void*)old_bucket_Node;
	}		
	
	
	
	new_Block = (block_header_t *) move_Address((char*) bucket_array[index], required_chunk*32, 1);
	
	new_Block->prev = mem_Address_To_Int((void*) curr_Block);
	new_Block->next = curr_Block->next;
	new_Block->size = initial_Size - required_chunk;
	new_Block->is_alloc = 0;

	// If there is a block to the right, link blockes together correctly for linked list
	if (curr_Block->next != AFTER_LIST)
	{
		(int_To_Mem_Address(curr_Block->next))->prev = mem_Address_To_Int((void*) new_Block);
	}



	// DELETE OLD NODE FROM LINKED LIST 
	old_bucket_Node = (block_header_unalloc *) move_Address((char*) curr_Block, 4, 1);
	bucket_Linked_List_Del_Node(old_bucket_Node, index);


	// CREATE AND HANDLE THE EXTRA 2 FLAGS IN THE NEW UNALLOC MEM HEADER; SET NODE AT BEGINNING OF BUCKET LINKED
	//========================================================================================================================
	new_bucket_Node = (block_header_unalloc *) move_Address((char*) new_Block, 4, 1);
	
	new_bucket_Node->prev = BEFORE_LIST;

	// get bucket index unallocated block will be found in given the size
	new_index = get_Bucket_Index_Unalloc(new_Block->size); 
	new_bucket_Node->next = mem_Address_To_Int((void*) bucket_array[new_index]);
	bucket_array[new_index] = new_Block;

	bit_vector |= get_Bucket_Bit_Mask(new_index);
	//========================================================================================================================

	// SET STUFF FOR curr_Block  (curr_Block being the now allocated block of memory) 
	curr_Block->next = mem_Address_To_Int((void*) new_Block);
	curr_Block->size = required_chunk;
	curr_Block->is_alloc= 1;

	// RETURN OLD BUCKET NODE SINCE THIS IS THE BEGINNING OF USABLE MEMORY TO BE ALLOCATED
	return (void*) old_bucket_Node;
}
	

// INITIALISES THE MEMORY TO BE MANAGED	
void  half_init(void){
	
	block_header_unalloc *bucket_Node;
	
	memset (bucket_array, 0, sizeof(bucket_array));
	
	// Initialize unallocated memory header 
	START_ADDR = (block_header_t *)array;
	//printf("START ADDRESS %p \n", START_ADDR);
	START_ADDR -> prev = BEFORE_LIST;
	START_ADDR -> next = AFTER_LIST;
	START_ADDR -> size = 1024;
	//printf(" START ADDR size %d \n", START_ADDR->size);
	START_ADDR -> is_alloc = 0;
	

	bucket_Node = (block_header_unalloc *) move_Address((char*) START_ADDR, 4, 1);
	//printf("START ADDRESS node %p \n", move_Address((char*) START_ADDR, 4, 1));
	bucket_Node->next = AFTER_LIST;
	bucket_Node->prev = BEFORE_LIST;

	bit_vector |= (1<<10);
	bucket_array[10] = START_ADDR;
}
	
// ALLOCATES MEMORY GIVEN THE SIZE IN BYTES
void *half_alloc(unsigned int size){
	
	int index;
	// CALULATE REQUIRED SIZE OF MEMORY AND ENSURE REQUEST IS NOT TOO LARGE
	int required_chunk =0;
	//printf("size is %d \n", size);
	size += 4;
	required_chunk = ceil(((double)size)/32);
	
	
	if(size > MAX_SIZE)
	{
		return NULL;
	}
	
	//printf("new size is %d \n", size);
	//printf("required chunk is %d", required_chunk);


	// FIND INDEX OF THE LOWEST AVAIL BUCKET THAT FITS CRITERIA
	index = find_avail_bucket( get_Bucket_Index(required_chunk) );
	//printf("required index is %d \n", index);
	
	// HANDLE IF BUCKET THAT IS TOO LARGE IS REQUESTED
	if (index > 10)
	{
		return NULL;
	}
	

	// HANDLE NEW HEADER CREATION, RE ARRANGEMENT OF LINKED LIST, UPDATING EVERYTHING ELSE THEN RETURN MEM ADDR OF NOW ALLOCATED MEM
	 return handle_Mem_Alloc(index, size, required_chunk);
	
	
	
}

// DEALLOCATES MEMORY GIVEN THE BEGINNING ADDRESS OF MEMORY
void  half_free(void * address)
{
	
	
	unsigned int temp_index;
	block_header_unalloc *temp_Node;
	block_header_unalloc *lower_Header_Bucket_Node; // Bucket header for lower_Header block
	
	// Move address from begin.ning of usable memory to beginning of header
	block_header_t *to_Del_Header = (block_header_t *) move_Address((char*) address, 4, 0);

	// Pointers to block headers at beginning and end of chunk of now unallocated memory .. lower_Header will become header for new unallocated block
	block_header_t *upper_Header = AFTER_LIST; 
	block_header_t *lower_Header = to_Del_Header;

	unsigned int new_size;
	new_size = to_Del_Header->size;
	if(new_size == 0)
 {
			new_size = 1024;
 }
	//printf("HALF FREE!! address is %p SIZE IS %d \n", address, new_size);
	//printf("address of to del header is %p \n", to_Del_Header);

 	// HANDLE IF THERE IS A BLOCK PRESENT TO THE RIGHT IF THE BLOCK WE ARE UNALLOCATING, IF IT IS UNALLOCATED THEN REMOVE THAT BLOCK FROM THE BUCKET LINKED LIST AND COALLESCE
	if (to_Del_Header->next != AFTER_LIST)
	{
		//printf("In del_header->next \n");
		upper_Header = int_To_Mem_Address(to_Del_Header->next); // case for when next block is present and allocated
		if ((int_To_Mem_Address(to_Del_Header->next))->is_alloc == 0)
		{
			new_size += int_To_Mem_Address(to_Del_Header->next)->size;
			// get index of bucket temp_header block is in
			temp_index = get_Bucket_Index_Unalloc((int_To_Mem_Address(to_Del_Header->next))->size);  
			temp_Node = (block_header_unalloc *) move_Address((char*) (int_To_Mem_Address(to_Del_Header->next)), 4, 1); 
			bucket_Linked_List_Del_Node(temp_Node , temp_index);
			upper_Header = AFTER_LIST; // case for when next block is present and unallocated AND is the last physical block
			if ( (int_To_Mem_Address(to_Del_Header->next))->next != AFTER_LIST)
			{
				upper_Header = int_To_Mem_Address((int_To_Mem_Address(to_Del_Header->next))->next); // case for when next block is present and unallocated AND the next next block is present and allocated
			}
		}
		
	}

	// HANDLE IF THERE IS A BLOCK PRESENT TO THE LEFT IF THE BLOCK WE ARE UNALLOCATING, IF IT IS UNALLOCATED THEN REMOVE THAT BLOCK FROM THE BUCKET LINKED LIST AND COALLESCE
	if (to_Del_Header->prev != BEFORE_LIST && (int_To_Mem_Address(to_Del_Header->prev))->is_alloc == 0)
	{
		//printf("In del_header->prev \n");
		new_size += (int_To_Mem_Address(to_Del_Header->prev))->size;
		lower_Header = int_To_Mem_Address(to_Del_Header->prev);
		// get index of bucket temp_header block is in
		temp_index = get_Bucket_Index_Unalloc((int_To_Mem_Address(to_Del_Header->prev))->size) ;  
		temp_Node = (block_header_unalloc *) move_Address((char*) (int_To_Mem_Address(to_Del_Header->prev)), 4, 1); 
		bucket_Linked_List_Del_Node(temp_Node , temp_index);
	}

	// link block (if any) after (newly formed) unallocated block (upper_Header) with the header of unallocated block (lower_Header)
	if (upper_Header != AFTER_LIST)
	{
		upper_Header->prev = mem_Address_To_Int((void*) lower_Header);
	}
	lower_Header->next = mem_Address_To_Int((void*) upper_Header);



	lower_Header->size = new_size;
	lower_Header->is_alloc = 0;

	// SET UP 2 EXTRA FLAGS FOR UNALLOCATED BLOCK TO ADD TO BUCKET BLOCKS LINKED LIST
	//========================================================================================================================
	lower_Header_Bucket_Node = (block_header_unalloc *) move_Address((char*) lower_Header, 4, 1);
	
	lower_Header_Bucket_Node->prev = BEFORE_LIST;

	temp_index = get_Bucket_Index_Unalloc(new_size); 
	lower_Header_Bucket_Node->next = mem_Address_To_Int((void*) bucket_array[temp_index]);
	bucket_array[temp_index] = lower_Header;

	bit_vector |= get_Bucket_Bit_Mask(temp_index);
	//========================================================================================================================

}

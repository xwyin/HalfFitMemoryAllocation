#ifndef HALF_FIT_H_
#define HALF_FIT_H_
#include "type.h"
#include "math.h"

/*
 * Author names:
 *   1.  uWaterloo User ID:  xxxxxxxx@uwaterloo.ca
 *   2.  uWaterloo User ID:  xxxxxxxx@uwaterloo.ca
 */

#define smlst_blk         5
#define	smlst_blk_sz     (1 << smlst_blk)
#define	lrgst_blk        15
#define	lrgst_blk_sz     (1 << lrgst_blk)


#define MAX_SIZE 32768

#define BEFORE_LIST 1023 // USED TO MARK BEGINNING OF LIST (1023 USED BECAUSE BEGINNING WILL NEVER BE AT 1023 WHICH IS THE LOCATION OF THE LAST ADDRESSABLE CHUNK)
#define AFTER_LIST 0 // USED TO MARK END OF LIST (0 USED BECAUSE END WILL NEVER BE AT 0 WHICH IS THE LOCATION OF THE FIRST ADDRESSABBLE CHUNK)

// BITMASKS USED TO ALTER BIT VECTOR WHEN BUCKETS ARE EMPTY OR NON-EMPTY
#define BUCKET10 1024
#define BUCKET9 512
#define BUCKET8 256
#define BUCKET7 128
#define BUCKET6 64
#define BUCKET5 32
#define BUCKET4 16
#define BUCKET3 8
#define BUCKET2 4
#define BUCKET1 2
#define BUCKET0 1


unsigned char array[MAX_SIZE] __attribute__ ((section(".ARM.__at_0x10000000"), zero_init));

U16 bit_vector = NULL; 

block_header_t *bucket_array[11];
block_header_t *START_ADDR;

// NORMAL HEADER USED BY ALL BLOCKS (MAKES A LINKED LIST OF ALL BLOCKS IN MEMORY)
typedef struct block_headers {
	U32 prev: 10;
	U32 next: 10;
	U32 size: 10;
	U32 is_alloc: 1;
}block_header_t; 

// HEADER USED BY ONLY UNALLOCATED BLOCKS (MAKES A LINKED LIST OF ALL BLOCKS IN A BUCKET)
typedef struct block_header_unalloc {
	U32 prev: 10;
	U32 next: 10;
}block_header_unalloc; 

void  half_init( void );
void *half_alloc( unsigned int );
void  half_free( void * );


U16 get_Bucket_Bit_Mask(int index);
int get_Bucket_Index( int size);
int get_Bucket_Index_Unalloc( int size);
int find_avail_bucket( int index);
char *move_Address(char* init_Address, unsigned int mem_Jump, unsigned int direction);
void bucket_Linked_List_Del_Node(block_header_unalloc *node, unsigned int index);
U32 mem_Address_To_Int(void* curr_Addr);
block_header_t* int_To_Mem_Address(unsigned int value);
void *handle_Mem_Alloc(int index, int size, int required_chunk);




#endif


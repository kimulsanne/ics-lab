/*
* mm-naive.c - The fastest, least memory-efficient malloc package.
* 
* In this naive approach, a block is allocated by simply incrementing
* the brk pointer.  A block is pure payload. There are no headers or
* footers.  Blocks are never coalesced or reused. Realloc is
* implemented directly using mm_malloc and mm_free.
*
* NOTE TO STUDENTS: Replace this header comment with your own header
* comment that gives a high level description of your solution.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
* NOTE TO STUDENTS: Before you do anything else, please
* provide your team information in the following struct.
********************************************************/
team_t team = {
	/* Team name */
	"5130379046",
	/* First member's full name */
	"Hu Jintao",
	/* First member's email address */
	"s474908647@sjtu.edu.cn",
	/* Second member's full name (leave blank if none) */
	"",
	/* Second member's email address (leave blank if none) */
	""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/* Basic constants and macros */
#define WSIZE 4 /* word size (bytes) */
#define DSIZE 8 /* double word size (bytes) */
#define OVERHEAD 8
#define CHUNKSIZE (1<<12) /* Extend heap by this amount (bytes) */

#define MAX(x, y) ((x) > (y)? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) +GET_SIZE(((char *)(bp)-WSIZE)))
#define PREV_BLKP(bp) ((void *)(bp) - GET_SIZE(((void *)(bp) - DSIZE)))

#define PREV_FREE(p) (* (size_t *)(p) )
#define SUCC_FREE(p) (* ((size_t *)(p) + 1))

#define PUT_PREV(p, val)  (*( size_t *)(p) = val)
#define PUT_SUCC(p, val)  (*((size_t *)(p) + 1) = val)

static char *heap_listp;

static void *block_16;
static void *block_32;
static void *block_64;
static void *block_128;
static void *block_256;
static void *block_512;
static void *block_1024;
static void *block_2048;
static void *block_4096;
static void *block_8192;
static void *block_16384;
static void *block_others;


static void *mem_heap;
static void *mem_brk;
static void *mem_max_addr;

static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void *place(void *bp, size_t asize);
static void *extend_heap(size_t words);

/*
int mem_init(void)
{
	mem_heap = (char *)Malloc(MAX_HEAP);
	mem_brk = (char *)mem_heap;
	mem_max_addr = (char *)(mem_heap + MAX_HEAP):
}


void *mem_sbrk(int incr)
{
	void *old_brk = mem_brk;
	if ((incr < 0) || ((mem_brk + incr) > mem_max_addr))
	{
		error = ENOMEM;
		fprintf(stderr, "ERROR:mem_sbrk failed.Ran out of memory...\n");
		return (void *) -1;
	}
	mem_brk += incr;
	return old_brk;
}
*/	


/* 
* mm_init - initialize the malloc package.
*/
int mm_init(void)
{
	block_16 = 0;
	block_32 = 0;
	block_64 = 0;
	block_128 = 0;
	block_256 = 0;
	block_512 = 0;
	block_1024 = 0;
	block_2048 = 0;
	block_4096 = 0;
	block_8192 = 0;
	block_16384 = 0;
	block_others = 0 ;

	/* create the initial empty heap */
	if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *) -1)
		return -1;
	PUT(heap_listp, 0); 			         /* alignment padding */
	PUT(heap_listp+(1*WSIZE), PACK(DSIZE, 1));  /* prologue header */
	PUT(heap_listp+(2*WSIZE), PACK(DSIZE, 1));  /* prologue footer */
	PUT(heap_listp+(3*WSIZE), PACK(0, 1)); 	         /* epilogue header */
	heap_listp += (2*WSIZE);

	/* Extend the empty heap with a free block of CHUNKSIZE bytes */
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}

static void alloc_block(void *bp)
{
        if (SUCC_FREE(bp) != NULL)
		PUT_PREV(SUCC_FREE(bp), PREV_FREE(bp));
        if (PREV_FREE(bp) !=  NULL)
		PUT_SUCC(PREV_FREE(bp), SUCC_FREE(bp));
	else 
	{	
		if (GET_SIZE(HDRP(bp)) == 16)		
		{	
			block_16 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 32)		
		{	
			block_32 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 64)		
		{	
			block_64 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 128)		
		{	
			block_128 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 256)		
		{	
			block_256 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 512)		
		{	
			block_512 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 1024)		
		{	
			block_1024 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 2048)		
		{	
			block_2048 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 4096)		
		{	
			block_4096 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 8192)		
		{	
			block_8192 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) <= 16384)		
		{	
			block_16384 = SUCC_FREE(bp); 
			return;
		}
		else if (GET_SIZE(HDRP(bp)) > 16384)					
		{	
			block_others = SUCC_FREE(bp); 
			return;
		}	
	}
}

static void free_block(void *bp)
{	
	if (GET_SIZE(HDRP(bp)) == 16)
	{	
		if (block_16!= NULL) PUT_PREV(block_16, bp);
		PUT_SUCC(bp, block_16); PUT_PREV(bp, NULL); block_16 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 32)
	{			
		if (block_32!= NULL) PUT_PREV(block_32, bp);
		PUT_SUCC(bp, block_32); PUT_PREV(bp, NULL); block_32 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 64)
	{				
		if (block_64!= NULL) PUT_PREV(block_64, bp);
		PUT_SUCC(bp, block_64); PUT_PREV(bp, NULL); block_64 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 128)
	{			
		if (block_128!= NULL) PUT_PREV(block_128, bp);
		PUT_SUCC(bp, block_128); PUT_PREV(bp, NULL); block_128 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 256)
	{	
		if (block_256!= NULL) PUT_PREV(block_256, bp);
		PUT_SUCC(bp, block_256); PUT_PREV(bp, NULL); block_256 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 512)
	{			
		if (block_512!= NULL) PUT_PREV(block_512, bp);
		PUT_SUCC(bp, block_512); PUT_PREV(bp, NULL); block_512 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 1024)
	{				
		if (block_1024= NULL) PUT_PREV(block_1024, bp);
		PUT_SUCC(bp, block_1024); PUT_PREV(bp, NULL); block_1024 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 2048)
	{				
		if (block_2048!= NULL) PUT_PREV(block_2048, bp);
		PUT_SUCC(bp, block_2048); PUT_PREV(bp, NULL); block_2048 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 4096)
	{				
		if (block_4096!= NULL) PUT_PREV(block_4096, bp);
		PUT_SUCC(bp, block_4096); PUT_PREV(bp, NULL); block_4096 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 8192)
	{				
		if (block_8192!= NULL) PUT_PREV(block_8192, bp);
		PUT_SUCC(bp, block_8192); PUT_PREV(bp, NULL); block_8192 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) <= 16384)
	{				
		if (block_16384!= NULL) PUT_PREV(block_16384, bp);
		PUT_SUCC(bp, block_16384); PUT_PREV(bp, NULL); block_16384 = bp; return;
	}
	else if (GET_SIZE(HDRP(bp)) > 16384)
	{				
		if (block_others!= NULL) PUT_PREV(block_others, bp);
		PUT_SUCC(bp, block_others); PUT_PREV(bp, NULL); block_others = bp; return;
	}	
}

static void *coalesce(void *bp)
{
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) { /* Case 1 */
		free_block(bp);
		return bp;
	}

	else if (prev_alloc && !next_alloc) { /* Case 2 */
		alloc_block(NEXT_BLKP(bp));		
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size,0));
		free_block(bp);
		return(bp);
	}

	else if (!prev_alloc && next_alloc) { /* Case 3 */
		alloc_block(PREV_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		free_block(PREV_BLKP(bp));
		return(PREV_BLKP(bp));
	}
	
	else { /* Case 4 */
		alloc_block(PREV_BLKP(bp));
		alloc_block(NEXT_BLKP(bp));
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
			GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		free_block(PREV_BLKP(bp));
		return(PREV_BLKP(bp));
	}
}

static void *extend_heap(size_t words)
{
	char *bp;
	size_t size;

	/* Allocate an even number of words to maintain alignment */
	size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
	if ((long)(bp = mem_sbrk(size))  == -1)
		return NULL;

	/* Initialize free block header/footer and the epilogue header */
	PUT(HDRP(bp), PACK(size, 0)); 		/* free block header */


	PUT(FTRP(bp), PACK(size, 0)); 		/* free block footer */
	PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* new epilogue header */

	/* Coalesce if the previous block was free */
	return coalesce(bp);
}


static void *find_fit(size_t asize)
{
	void **block;
	void *bp;

	if (asize <= 16)
		block = &block_16;
	else if (asize <=32)
		block = &block_32;
	else if (asize <=64)
		block = &block_64;
	else if (asize <=128)
		block = &block_128;
	else if (asize  <=256)
		block = &block_256;
	else if (asize  <=512)
		block = &block_512;
	else if (asize  <=1024)
		block = &block_1024;
	else if (asize  <=2048)
		block = &block_2048;
	else if (asize  <=4096)
		block = &block_4096;
	else if (asize  <=8192)
		block = &block_8192;
	else if (asize  <=16384)
		block = &block_16384;
	else block = &block_others;	
	
	while (block)	
	{
		bp = *block;
		while (bp)
		{
			if ((asize <= GET_SIZE(HDRP(bp)))) 
				return bp;
			bp = SUCC_FREE(bp);
		}		
    		if(block == &block_16) block= &block_32;
   		else if(block == &block_32) block = &block_64;
    		else if(block == &block_64) block = &block_128;
    		else if(block == &block_128) block = &block_256;
     		else if(block == &block_256) block = &block_512;
    		else if(block == &block_512) block= &block_1024;
    		else if(block == &block_1024) block= &block_2048;
    		else if(block == &block_2048) block= &block_4096;
   		else if(block == &block_4096) block= &block_8192;
   		else if(block == &block_8192) block= &block_16384;
   		else if(block == &block_16384) block= &block_others;
   		else block= NULL;
				
		
	}
	return NULL;  
}
 
static void *place(void *bp, size_t asize)
{

	size_t csize = GET_SIZE(HDRP(bp)) ;
	if ( (csize - asize) >= (DSIZE + OVERHEAD) ) {
		alloc_block(bp);
		PUT(HDRP(bp), PACK(csize - asize, 0)) ;
		PUT(FTRP(bp), PACK(csize - asize, 0)) ;
		free_block(bp);
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
	} else {
		alloc_block(bp);
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
	return bp;
}

/* 
* mm_malloc - Allocate a block by incrementing the brk pointer.
*     Always allocate a block whose size is a multiple of the alignment.
*/
void *mm_malloc (size_t size)
{
	size_t asize; /* adjusted block size */
	size_t extendsize; /* amount to extend heap if no fit */
	char *bp;

	/* Ignore spurious requests */
	if (size <= 0)
		return NULL;

	/* Adjust block size to include overhead and alignment reqs. */
	if (size <= DSIZE)
		asize = DSIZE + OVERHEAD;
	else
		asize = DSIZE * ((size - 1) / DSIZE + 2);

	/* Search the free list for a fit */
	if ((bp = find_fit(asize)) != NULL) {
		bp = place (bp, asize);
		return bp;
	}

	/* No fit found. Get more memory and place the block */
	extendsize = MAX (asize, CHUNKSIZE) ;
	if ((bp = extend_heap (extendsize/WSIZE)) == NULL)
		return NULL;
	bp = place (bp, asize);
	return bp;
}


/*
* mm_free - Freeing a block does nothing.
*/
void mm_free(void *bp)
{
	size_t size = GET_SIZE(HDRP(bp));

	PUT(HDRP(bp), PACK(size, 0));
	PUT(FTRP(bp), PACK(size, 0));
	coalesce(bp);
}

/*
* mm_realloc - Implemented simply in terms of mm_malloc and mm_free
*/
void *mm_realloc(void *ptr, size_t size)
{
	void *newptr;
	size_t copySize;
	size_t asize;
	size_t is_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
	size_t next_size = GET_SIZE(HDRP(NEXT_BLKP(ptr)));	

	if (ptr == NULL)
		return NULL;

	if (size == 0)
	{
		mm_free(ptr);
		return NULL;
	}

        if (size <= DSIZE)
                asize = DSIZE + OVERHEAD;
        else
                asize = DSIZE * ((size - 1) / DSIZE + 2);
	
	copySize = GET_SIZE(HDRP(ptr));
	
	if (asize <= copySize)
		return ptr;
	else
	{	if (!is_alloc && (copySize + next_size) >= asize)
		{
			alloc_block(NEXT_BLKP(ptr));
			PUT(HDRP(ptr), PACK(copySize + next_size, 1));
			PUT(FTRP(ptr), PACK(copySize + next_size, 1));
			return ptr;
		}
		else
		{
			newptr = mm_malloc(size);
			if (newptr == NULL)
				return NULL;
			memcpy(newptr, ptr, copySize);
			mm_free(ptr);
			return newptr;
		}
	}
}





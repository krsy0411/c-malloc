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

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/////////////////////////////////
static char* heap_listp = NULL;
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t asize);
/////////////////////////////////

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
   
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void*)-1)  return -1;

    PUT(heap_listp, 0);                            // 정렬 패딩
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 에필로그 헤더

    heap_listp += (2 * WSIZE); // heap_listp를 첫 가용 블록의 payload 주소로 이동

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;

    return 0;
}

/*
 * extend_heap - 힙을 words만큼 확장, 새로운 가용 블록을 리턴함
 */
static void* extend_heap(size_t words)
{
    char* bp;
    size_t size;

    // 항상 8바이트 단위로 정렬, 짝수 워드 할당
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;

    // 새 가용 블록의 헤더/푸터, 새로운 에필로그 헤더 초기화
    PUT(HDRP(bp), PACK(size, 0));         
    PUT(FTRP(bp), PACK(size, 0));         
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); 

    return bp
    // return coalesce(bp);
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    int newsize = ALIGN(size + SIZE_T_SIZE);
    void *p = mem_sbrk(newsize);
    if (p == (void *)-1)
        return NULL;
    else
    {
        *(size_t *)p = size;
        return (void *)((char *)p + SIZE_T_SIZE);
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
        return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
        copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}
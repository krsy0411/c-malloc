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
#include "mmdefs.h"

static void* extend_heap(size_t words);
static void* coalesce(void *bp);
int mm_init(void);
void* mm_malloc(size_t size);
void mm_free(void *ptr);
void* mm_realloc(void *ptr, size_t size);

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

static char* heap_listp;

static void* extend_heap(size_t words)
{
    char *bp; // 블록 포인터
    size_t size;

    /* 항상 더블 워드(8바이트) 정렬로 맞추기 위해 사이즈를 설정하고 힙을 확장 */
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if((long)(bp = mem_sbrk(size)) == -1)
    {
        return NULL;
    }

    /* 블록 헤더/푸터를 메모리 할당 해제(free)하고 에필로그 헤더를 다시 설정 */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}

static void* coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // 이전/이후 블록이 모두 할당되어있는 경우
    if(prev_alloc && next_alloc)
    {
        return bp;
    }
    // 이전 블록은 할당되어있고, 이후 블록은 할당 안 되어있는 경우
    else if(prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    // 이전 블록은 할당 안 되어있고, 이후 블록은 할당되어있는 경우
    else if(!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    // 이전/이후 모두 할당 안 된 경우
    else
    {
        size += (GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp))));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }

    return bp;
}

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk((4 * WSIZE))) == (void *) - 1)
    {
        return -1;
    }

    PUT(heap_listp, 0); // 정렬 패딩
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); // 에필로그 헤더
    heap_listp += (2*WSIZE);

    if (extend_heap(CHUNKSIZE / WSIZE) == NULL)
    {
        return -1;
    }

    return 0;
}

/* 
    fit한 공간(가용 블록) 찾아내기
*/
static void* find_fit_first(size_t size)
{
    // first fit
    void *bp;

    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    {
        // 1) 현재 블록의 헤더를 통해 가용 블록인지 확인
        // 2) 현재 블록의 크기가 요청한 크기(size)보다 크거나 같은지 확인
        if(!GET_ALLOC(HDRP(bp)) && (size <= GET_SIZE(HDRP(bp))))
        {
            return bp;
        }
    }

    // fit한 블록이 없는 경우
    return NULL;
}

/* 
    찾은 블록에 실제로 메모리를 배치 & 필요시 분할
*/
static void place(void* bp, size_t size)
{
    size_t full_size = GET_SIZE(HDRP(bp));

    // 할당하고 남는 블록의 전체 크기가 16바이트(최소 블록 크기)인 경우라면
    if((full_size - size) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(size, 1));
        PUT(FTRP(bp), PACK(size, 1));

        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(full_size - size, 0));
        PUT(FTRP(bp), PACK(full_size - size, 0));
    }
    else
    {
        // 분할할 수는 없는 경우
        PUT(HDRP(bp), PACK(full_size, 1));
        PUT(FTRP(bp), PACK(full_size, 1));
    }
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void* mm_malloc(size_t size)
{
    size_t adjusted_size; // 조정된 "블록 크기"
    size_t extend_size; // 확장된 힙 크기
    char *bp;

    // 가짜 요청은 무시
    if(size == 0)
    {
        return NULL;
    }

    // 블록 크기를 조정
    if(size <= DSIZE)
    {
        // 헤더(4바이트) + 푸터(4바이트) + 최소 페이로드 = 16바이트(최소 블록 크기)
        adjusted_size = (2 * DSIZE);
    }
    else
    {
        /* 
            우선, 결과값은 DSIZE의 배수가 되어야함(DSIZE = 정렬 단위)
            (DSIZE - 1)을 더하고 나눗셈해야, 나눗셈 연산 결과가 실수의 "올림 처리"됨 : (13/8 = 1) -> ((13+7)/8 = 2)
            size(사용자가 요청한 페이로드 크기), DSIZE(정렬 단위)
        */
        adjusted_size = DSIZE * ((size + DSIZE + (DSIZE - 1)) / DSIZE);
    }

    // 가용 리스트 찾기
    if((bp = find_fit_first(adjusted_size)) != NULL)
    {
        place(bp, adjusted_size);
        return bp;
    }

    // fit한 공간 못 찾으면, 추가 메모리를 얻어 블록 배치
    extend_size = MAX(adjusted_size, CHUNKSIZE);
    if((bp = extend_heap(extend_size / WSIZE)) == NULL)
    {
        return NULL;
    }

    place(bp, adjusted_size);

    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 * 전달받은 포인터가 가리키는 블록의 헤더 & 푸터에 "할당되지 않은 상태(0)"을 기록하고, 인접한 가용 블록과 병합하여 메모리 할당 해제
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void* mm_realloc(void *ptr, size_t size)
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
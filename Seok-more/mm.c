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
#include <stdbool.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Implicit: first-fit, just realloc",
    /* First member's full name */
    "Seok-more",
    /* First member's email address */
    "wjstjrah2000@gmail.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/////////////////////////////////
static char *heap_listp = NULL;
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
/////////////////////////////////

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void*)-1)  return -1;

    PUT(heap_listp, 0);                          // 정렬 패딩
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     // 에필로그 헤더

    heap_listp += (2*WSIZE); /

    if (extend_heap(CHUNKSIZE/WSIZE) == NULL) return -1;

    return 0;
}

/*
 * extend_heap - 힙을 words만큼 확장, 새로운 가용 블록을 리턴함
 */
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    // 항상 8바이트 단위로 정렬, 짝수 워드 할당
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;
    
 
    PUT(HDRP(bp), PACK(size, 0));         
    PUT(FTRP(bp), PACK(size, 0));         
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /

    return coalesce(bp);
}


/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;         
    size_t extendsize;    
    char *bp;

    if (size == 0) return NULL;

    // 1. 최소 블록 크기(헤더+payload+푸터) 맞추고 8바이트 단위로 정렬
    if (size <= DSIZE)
    {
        asize = 2 * DSIZE; // 최소 16바이트
    }
    else
    {
        // asize = ALIGN(size + DSIZE); 
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); 
    }

    // 2. 가용 리스트에서 asize만큼 맞는 블록 탐색
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);    
        return bp;           
    }

    // 3. 못 찾으면 힙 확장 후 새 블록 할당
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;

    place(bp, asize);
    return bp;
}

/*
 * find_fit - asize 크기 이상의 가용 블록을 찾아서 payload 포인터 반환
 */
static void *find_fit(size_t asize)
{
    // 전체 순회 
    char *bp = heap_listp;
    while (GET_SIZE(HDRP(bp)) > 0) 
    {
        if (!GET_ALLOC(HDRP(bp)) && (GET_SIZE(HDRP(bp)) >= asize))
        {
            return bp;
        }
        bp = NEXT_BLKP(bp);
    }
    return NULL;
}

/*
 * place - bp 위치의 free 블록에 asize만큼 할당, 필요하면 분할해버림
 */
static void place(void *bp, size_t asize)
{
    size_t totalsize = GET_SIZE(HDRP(bp)); 

    // 남은 크기가 최소 블록 크기(헤더+payload+푸터 = 16바이트) 이상이면 분할
    if ((totalsize - asize) >= (2 * DSIZE))
    {
        // 블록 분할
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));

        char *next_bp = NEXT_BLKP(bp); 
        PUT(HDRP(next_bp), PACK(totalsize - asize, 0)); 
        PUT(FTRP(next_bp), PACK(totalsize - asize, 0)); 
    }
    else
    {

        PUT(HDRP(bp), PACK(totalsize, 1));
        PUT(FTRP(bp), PACK(totalsize, 1));
    }
}


/*
 * coalesce - 인접 free 블록을 병합. 병합된 블록의 payload 포인터 반환
 */
static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp))); 
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp))); 
    size_t size = GET_SIZE(HDRP(bp));                   

    // Case 1: 이전/다음 모두 할당됨
    if (prev_alloc && next_alloc)
    {
        return bp;
    }

    // Case 2: 이전 할당, 다음 free
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // Case 3: 이전 free, 다음 할당
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    // Case 4: 이전/다음 모두 free
    else
    {
        size += ( GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp))) );
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    return bp;
}

/*
 * mm_free - 블록을 해제하고 인접 free 블록과 병합
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/*
 * mm_realloc - 블록 크기 재조정 (새 블록 할당, 데이터 복사, 기존 블록 해제)
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) return mm_malloc(size); // ! 포인터가 NULL이면 새 블록 할당해야함
    if (size == 0)
    {
        mm_free(ptr);
        return NULL;
    }

    size_t old_size = GET_SIZE(HDRP(ptr)) - DSIZE; // 기존 payload 크기
    void *newptr = mm_malloc(size);

    if (newptr == NULL) return NULL;

    size_t copySize = (size < old_size) ? size : old_size;
    memcpy(newptr, ptr, copySize); // memcpy : 메모리 영역을 "복사"하는 함수 memcpy(목적지_주소, 원본_주소, 복사_크기);
    
    mm_free(ptr);
    return newptr;
}
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

team_t team = {
    "team 14",
    "LEE SI YOUNG",
    "leesyungg@gmail.com",
    "",
    ""
};

#define ALIGNMENT 8
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define WSIZE sizeof(size_t)
#define DSIZE (2 * WSIZE)
#define CHUNKSIZE (1 << 12)
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc))
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))
#define PRED(bp) (*(void **)(bp))
#define SUCC(bp) (*(void **)((char *)(bp) + WSIZE))

static char *heap_listp = NULL;
static char *free_listp = NULL;

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);
static void insert_free_block(void *bp);
static void delete_free_block(void *bp);

int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1) return -1;
    PUT(heap_listp, 0);                          // 정렬 패딩
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 헤더
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); // 프롤로그 푸터
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     // 에필로그 헤더
    heap_listp += (2 * WSIZE);
    free_listp = NULL;
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
    return 0;
}

static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1) return NULL;
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));
    return coalesce(bp);
}

void *mm_malloc(size_t size)
{
    size_t asize;
    size_t extendsize;
    char *bp;
    if (size == 0) return NULL;
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = ALIGN(size + DSIZE);
    if ((bp = find_fit(asize)) != NULL)
    {
        place(bp, asize);
        return bp;
    }
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL) return NULL;
    place(bp, asize);
    return bp;
}

static void *find_fit(size_t asize)
{
    // first-fit 방식
    void *bp = free_listp;
    while (bp != NULL)
    {
        if (GET_SIZE(HDRP(bp)) >= asize)
            return bp;
        bp = SUCC(bp);
    }
    return NULL;
}

static void place(void *bp, size_t asize)
{
    size_t totalsize = GET_SIZE(HDRP(bp));
    delete_free_block(bp);
    if ((totalsize - asize) >= (2 * DSIZE))
    {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        char *next_bp = NEXT_BLKP(bp);
        PUT(HDRP(next_bp), PACK(totalsize - asize, 0));
        PUT(FTRP(next_bp), PACK(totalsize - asize, 0));
        insert_free_block(next_bp);
    }
    else
    {
        PUT(HDRP(bp), PACK(totalsize, 1));
        PUT(FTRP(bp), PACK(totalsize, 1));
    }
}

static void insert_free_block(void *bp)
{
    PRED(bp) = NULL;
    SUCC(bp) = free_listp;
    if (free_listp != NULL)
        PRED(free_listp) = bp;
    free_listp = bp;
}

static void delete_free_block(void *bp)
{
    if (bp == free_listp)
    {
        free_listp = SUCC(bp);
        if (free_listp != NULL)
            PRED(free_listp) = NULL;
    }
    else
    {
        if (PRED(bp) != NULL)
            SUCC(PRED(bp)) = SUCC(bp);
        if (SUCC(bp) != NULL)
            PRED(SUCC(bp)) = PRED(bp);
    }
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    if (prev_alloc && next_alloc)
    {
        insert_free_block(bp);
        return bp;
    }
    else if (prev_alloc && !next_alloc)
    {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_free_block(NEXT_BLKP(bp));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_free_block(bp);
        return bp;
    }
    else if (!prev_alloc && next_alloc)
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        bp = PREV_BLKP(bp);
        delete_free_block(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_free_block(bp);
        return bp;
    }
    else
    {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        delete_free_block(PREV_BLKP(bp));
        delete_free_block(NEXT_BLKP(bp));
        bp = PREV_BLKP(bp);
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        insert_free_block(bp);
        return bp;
    }
}

void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) 
    {
        return mm_malloc(size);
    }

    if (size == 0) 
    {
        mm_free(ptr); return NULL;
    }
    
    size_t old_size = GET_SIZE(HDRP(ptr));
    size_t asize = (size <= DSIZE) ? 2 * DSIZE : ALIGN(size + DSIZE);

    if (asize < old_size && (old_size - asize) >= (2 * DSIZE)) {
        PUT(HDRP(ptr), PACK(asize, 1));
        PUT(FTRP(ptr), PACK(asize, 1));
        char *next_blk = NEXT_BLKP(ptr);
        PUT(HDRP(next_blk), PACK(old_size - asize, 0));
        PUT(FTRP(next_blk), PACK(old_size - asize, 0));
        insert_free_block(next_blk);
        return ptr;
    }

    void *newptr = mm_malloc(size);
    if (newptr == NULL)
    {
        return NULL;
    }

    size_t copySize = (old_size < size) ? old_size : size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);

    return newptr;
}
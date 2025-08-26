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

static char *heap_listp = NULL;     //힙 공간 생성할 heap_listp생성

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void){
    heap_listp = mem_sbrk(4*WSIZE);     //비어있는 힙 생성 (힙 영역 brk기준으로 16 늘림)
    if (heap_listp == (void *) -1){     //빈 힙이면 생성 안된 것
        return -1;
    }

    PUT(heap_listp, 0);     //패딩 할당 - 맨 앞 4바이트 공간에 0을 넣음, 8바이트 정렬이라서!
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));    //프롤로그 헤더 : 힙 맨앞에 세워둠, 맨앞을 넘어가서 PREV_BLKP매크로 만나면 예외처리, 사이즈랑 할당 여부 1 지정
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));    //프롤로그 푸터 : 푸터는 헤더 4바이트 뒤 공간
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));        //에필로그 헤더 : 힙의 맨끝을 표시하는 경계선, 끝을 넘어가서 NEXT_BLKP매크로 만나면 예외처리
    heap_listp += (2*WSIZE);        //포인터 위치 8바이트 뒤로 옮김, 시작점 8 시작

    return 0;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;   //사이즈를 짝수 * 4 는 항상 8의 배수가 되므로(홀수 * 4는 8의 배수 안됨)
    if ((long)(bp = mem_sbrk(size)) == -1){     //size만큼 bp를 늘림, 안되면 NULL
        return NULL;
    }

    PUT(HDRP(bp), PACK(size, 0));           //가용 블럭 헤더(가용이니까 0)
    PUT(FTRP(bp), PACK(size, 0));           //가용 블럭 푸터
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));   //새로운 에필로그 헤더(할당 1)

    //return coalesce(bp);
    return bp;
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

    if (size==0){
        return NULL;
    }

    if (size <= DSIZE){
        asize = 2*DSIZE;
    }
    else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }

    if ((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE) == NULL)){
        return NULL;
    }

    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp){
    size_t size = GET_SIZE(HDRP(bp));       //bp헤더 사이즈 받아서

    PUT(HDRP(bp), PACK(size, 0));       //헤더 가용
    PUT(FTRP(bp), PACK(size, 0));       //푸터 가용
    coalesce(bp);       //블록 병합
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));     //이전 블럭의 푸터 할당
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));     //다음 블럭의 헤더 할당
    size_t size = GET_SIZE(HDRP(bp));       //bp 실제 크기

    if (prev_alloc && next_alloc){      //이전 블럭과 다음 블럭이 할당 돼있으면 합칠블록 없으니까
        return bp;      //현재 bp반환
    }

    else if (prev_alloc && !next_alloc){        //이전 블럭 = 할당, 다음블록 = 가용
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));      //다음 블록 크기를 사이즈에 더해줌
        PUT(HDRP(bp), PACK(size, 0));       //현재 블록 헤더를 업뎃
        PUT(FTRP(bp), PACK(size, 0));       //현재 블록 푸터를 업뎃
    }

    else if (!prev_alloc && next_alloc){        //이전 블록 = 가용, 다음블럭 = 할당 
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));      //이전블럭의 헤더 사이즈만큼 더해줌
        PUT(FTRP(bp), PACK(size, 0));       //현재블럭 푸터 업뎃
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));        //이전 블럭의 헤더 업뎃
        bp = PREV_BLKP(bp);     //이전 블럭으로 점프, 시작점 = 이전 블록
    }

    else {      //다음,이전블럭 다 없으면
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));      //이전 다음 블럭 사이즈 더해주고
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));        //이전 블럭의 헤더 업뎃
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));       //다음 블록의 푸터(끝점+4) 업뎃
        bp = PREV_BLKP(bp);     //이전 블록으로 점프, 시작점 = 이전블록
    }
    return bp;      //병합 완료된 bp 시작점 반환
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
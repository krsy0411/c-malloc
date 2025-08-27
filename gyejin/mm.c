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
//전방 선언
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_first_fit(size_t asize);
static void place(void *bp, size_t asize);

static char *heap_listp = NULL;     //힙 공간 생성할 heap_listp생성
static char *free_listp = NULL;     //[ex+] 가용 리스트 free_listp생성

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

    free_listp = NULL;      //[ex+] 

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL){ //[ex+] 
        return -1;      //[ex+] 
    }
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

    return coalesce(bp);
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;       //블록사이즈
    size_t extendsize;  //확장 힙 사이즈
    char *bp;

    //사이즈가 0이면 그냥 NULL반환
    if (size==0){
        return NULL;
    }

    //[ex+] 요청 사이즈가 8바이트보다 작으면
    if (size <= 2*DSIZE){     
        asize = 3*DSIZE;    //[ex+] 최소 블럭 크기인 24바이트로 세팅 (헤더 4 + 페이로드 16 + 푸터 4)=> explicit에서는 최소블럭크기 24바이트 (페이로드에 PREV, SUCC 두개의 포인터)
    }
    //8바이트보다 크면
    else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);   //[?](사이즈 + 8바이트 + 7바이트) / 8바이트 한거를 8바이트에 곱하기 => 8의 배수로 올림 처리?
    }

    if ((bp = find_first_fit(asize)) != NULL){      //배치 정렬 first-fit으로 호출
        place(bp, asize);       //찾은 공간에 블록 할당 및 분할
        return bp;
    }

    //적절한 가용 블럭이 없는 경우
    extendsize = MAX(asize, CHUNKSIZE);     //extend는 청크 사이즈와 asize중에서 큰거
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL){      //extend를 4바이트로 나눈만큼 힙을 늘림 왜? 
        return NULL;
    }

    place(bp, asize);       //확장된 힙 공간에 할당
    return bp;
}

//[exALL+] bp를 가용리스트에 삽입
static void mm_insert_block(void *bp){
    //새 블록이 가용리스트 맨앞(LIFO)
    PUT_SUCC(bp, free_listp);   //새 블록의 다음 SUCC는 원래 첫번째 블록을 가리키게

    //가용 리스트가 비어있지 않다면 첫번째 블록의 이전블록이 새블록을 가리키게
    if (free_listp != NULL){
        PUT_PREV(free_listp, bp);
    }

    PUT_PREV(bp, NULL); //새 블록의 이전은 없어졌으니 지움

    free_listp = bp;    //가용 리스트 시작점을 새로운 블록으로 업데이트
}

//[exALL+] bp를 가용리스트에서 제거(이중 연결 리스트)
static void mm_remove_block(void *bp){
    void *prev = GET_PREV(bp);
    void *next = GET_SUCC(bp);

    //중간에 껴있는 블럭이면
    if (prev != NULL && next != NULL){
        PUT_SUCC(prev, next);        //이전 블록의 SUCC블록은 next로
        PUT_PREV(next, prev);        //다음 블록의 PREV블록은 pre로
    }

    //맨 앞
    else if (prev == NULL && next != NULL){      //푸터와 같으면 맨뒤
        PUT_PREV(next, prev);        //다음 블록의 PREV블록은 pre로
        free_listp = next;      //가용 리스트 맨앞은 삭제한 다음 블록으로
    }

    //맨 뒤
    else if (prev != NULL && next == NULL){     //헤더와 같으면 맨앞
        PUT_SUCC(prev, next);        //이전 블록의 SUCC블록은 next로
    }

    else {      //단일 블록 제거
        free_listp = NULL;      //가용리스트는 반환
    }
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
        mm_insert_block(bp);        //[ex+] 현재 블록 리스트에 추가
        return bp;      //현재 bp반환
    }

    else if (prev_alloc && !next_alloc){        //이전 블럭 = 할당, 다음블록 = 가용
        mm_remove_block(NEXT_BLKP(bp));     //[ex+] 다음 블록을 리스트에서 제거
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));      //다음 블록 크기를 사이즈에 더해줌
        PUT(HDRP(bp), PACK(size, 0));       //현재 블록 헤더를 업뎃
        PUT(FTRP(bp), PACK(size, 0));       //현재 블록 푸터를 업뎃
        mm_insert_block(bp);        //[ex+] 합쳐진 블록 리스트에 추가
    }

    else if (!prev_alloc && next_alloc){        //이전 블록 = 가용, 다음블럭 = 할당 
        mm_remove_block(PREV_BLKP(bp));     //[ex+] 이전 블록을 리스트에서 제거
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));      //이전블럭의 헤더 사이즈만큼 더해줌
        PUT(FTRP(bp), PACK(size, 0));       //현재블럭 푸터 업뎃
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));        //이전 블럭의 헤더 업뎃
        bp = PREV_BLKP(bp);     //이전 블럭으로 점프, 시작점 = 이전 블록
        mm_insert_block(bp);    //[ex+] 합쳐진 블록 리스트에 추가
    }

    else {      //다음,이전블럭 다 없으면
        mm_remove_block(NEXT_BLKP(bp)); //[ex+] 이전 블록 제거
        mm_remove_block(PREV_BLKP(bp)); //[ex+] 다음 블록 제거
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));      //[ex+] 이전 다음 블럭 헤더 읽어와 사이즈 더해주고
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));        //이전 블럭의 헤더 업뎃
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));       //다음 블록의 푸터(끝점+4) 업뎃
        bp = PREV_BLKP(bp);     //이전 블록으로 점프, 시작점 = 이전블록
        mm_insert_block(bp);    //[ex+] 합쳐진 블록 리스트에 추가
    }

    return bp;      //병합 완료된 bp 시작점 반환
}

//first-fit 탐색 함수 (배치 정렬 함수)
static void *find_first_fit(size_t asize){
    void *bp;

    //[ex+] 가용리스트만 따라가면서 탐색, SUCC로 다음블록으로 하나씩 이동
    for (bp = free_listp; bp != NULL; bp = GET_SUCC(bp)){
        if (asize <= GET_SIZE(HDRP(bp))){     //[ex+] 가용인건 당연, 크기 확인 담기에 충분하다면
            return bp;      //블록 포인터 반환
        }
    }
    return NULL;        //적절한거 찾지 못하면 NULL
}

static void place(void *bp, size_t asize){
    size_t free_size = GET_SIZE(HDRP(bp));      //헤더 사이즈를 받아 free_size에 저장

    mm_remove_block(bp);        //[ex+] 할당할거니까 가용리스트에서 제거

    //[ex+] 현재 가용블럭 크기가 요청된 크기를 할당하고도 (헤더 크기 - 요청받은 할당 크기) 24바이트보다 크면, 최소블록크기인 24바이트 이상 남으면 블록 분할
    if ((free_size - asize) >= (3*DSIZE)) {   
        //앞부분 블록 : 요청한 크기만큼 할당  
        PUT(HDRP(bp), PACK(asize, 1));      //헤더 -> 할당
        PUT(FTRP(bp), PACK(asize, 1));      //푸터 -> 할당
        //다음 블록에 남은 공간을 가용으로 만듦
        bp = NEXT_BLKP(bp);     //분할된 뒷부분 블럭으로 이동
        PUT(HDRP(bp), PACK(free_size-asize, 0));    //요청 크기만큼 뺀 크기를 헤더 -> 가용 
        PUT(FTRP(bp), PACK(free_size-asize, 0));    // ""                  푸터 -> 가용
        mm_insert_block(bp);        //[ex+] 새로운 가용 블록 리스트에 추가
    }
    //(헤더 크기 - 요청받은 할당 크기) 가 24바이트보다 작으면 == 남은 공간이 24보다 작아서 분할 못하는 경우
    else{
        PUT(HDRP(bp), PACK(free_size, 1));      //헤더사이즈만큼만 헤더에 할당
        PUT(FTRP(bp), PACK(free_size, 1));      //헤더사이즈만큼만 푸터에 할당
    }
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
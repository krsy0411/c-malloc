#include <stdio.h>
#define WSIZE   4       //워드, 헤드, 푸터 사이즈
#define DSIZE   8       //더블 워드 사이즈
#define CHUNKSIZE   (1<<12)     //힙 - 초기 가용 블럭 생성

#define MAX(x, y)   ((x) > (y) ? (x) : (y))  //매크로 함수 : 괄호 필수, 일반 함수보다 단순 함수 구현에선 빠름

#define PACK(size, alloc)   ((size) | (alloc)) //하위 3비트로 할당/가용 상태인지 체크, or연산이니까 1이면 무조건 1, 0이면 무조건 0들어감

//주소값 읽고 쓰기
#define GET(p)          (*(unsigned int *)(p))
#define PUT(p, val)     (*(unsigned int *)(p) = (val))

#define GET_SIZE(p)     (GET(p) & ~0x7)     //[?] 00...000111 ~로 다 뒤집어(마스크), AND연산하면 강제로 000이 되고 실제 크기정보만 남게됨, 하위3비트 0으로 초기화 해주는 작업
#define GET_ALLOC(p)    (GET(p) & 0x1)      //할당 받으면 하위 1비트 1로 변경

#define HDRP(bp)        ((char *)(bp) - WSIZE)      //헤더 주소 계산 : 페이로드 시작주소 - 블록의 전체 크기
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)     //푸터 주소 계산 : 페이로드 시작주소 + 블록 전체 크기 => 다음 블록 시작주소, -8바이트(헤더,푸터) 뺴면 => 풋터 시작주소

#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))       //다음 블럭으로 점프
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))       //이전 블럭으로 점프

//가용 리스트의 이전/다음 포인터 값을 읽어옴
#define GET_PREV(bp)    (*(char**)(bp))       //페이로드 시작점에 PREV 포인터
#define GET_SUCC(bp)    (*(char**)(bp + DSIZE))         //페이로드 시작점 + 8바이트 뒤에 SUCC 포인터
//가용 리스트의 이전/다음 포인터 값을 씀
#define PUT_PREV(bp, val)   (*(char**)(bp) = (val))
#define PUT_SUCC(bp, val)  (*(char**)(bp + DSIZE) = (val))  

extern int mm_init (void);
extern void *mm_malloc (size_t size);
extern void mm_free (void *ptr);
extern void *mm_realloc(void *ptr, size_t size);

/* 
 * Students work in teams of one or two.  Teams enter their team name, 
 * personal names and login IDs in a struct of this
 * type in their bits.c file.
 */
typedef struct {
    char *teamname; /* ID1+ID2 or ID1 */
    char *name1;    /* full name of first member */
    char *id1;      /* login ID of first member */
    char *name2;    /* full name of second member (if any) */
    char *id2;      /* login ID of second member */
} team_t;

extern team_t team;


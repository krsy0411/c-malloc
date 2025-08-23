///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WSIZE       sizeof(size_t)       
#define DSIZE       (2 * WSIZE)          
#define CHUNKSIZE   (1<<12)              

#define MAX(x, y)   ((x) > (y)? (x) : (y)) 

#define PACK(size, alloc)   ((size) | (alloc)) // 사이즈와 할당여부를 하나의 워드로 합침: size는 8의 배수로 맞춤(하위 3비트가 0임) 
                                               // -> 할당 여부(0x1/0x0)를 LSB에 저장해도 크기 정보가 안겹치니까 합칠 수 있음

                                               
#define GET(p)      (*(size_t *)(p))           
#define PUT(p, val) (*(size_t *)(p) = (val))   

#define GET_SIZE(p)     (GET(p) & ~0x7)      
#define GET_ALLOC(p)    (GET(p) & 0x1)       

#define HDRP(bp)    ((char *)(bp) - WSIZE)                      
#define FTRP(bp)    ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) 

#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) 
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#include <stdio.h>

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


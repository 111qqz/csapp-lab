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
#include "mm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "st",
    /* First member's full name */
    "111qqz",
    /* First member's email address */
    "hust.111qqz",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4
#define DSIZE 8

#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

// explict free list时，最小的block size是16.
// header和footer各占4字节，两个指针各占4字节，一共16字节
#define BLOCK_MIN_SIZE 16

#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p) (*(unsigned int *)(p))               // line:vm:mm:get
#define PUT(p, val) (*(unsigned int *)(p) = (val))  // line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p) (GET(p) & ~0x7)  // line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)  // line:vm:mm:getalloc
#define GET_LAST_THREE_BIT(p) (GET(p) & 0x7)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp) ((char *)(bp)-WSIZE)                         // line:vm:mm:hdrp
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)  // line:vm:mm:ftrp

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLK(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLK(bp) ((void *)(bp)-GET_SIZE((void *)(bp)-DSIZE))

// 用于explict free list
// 需要注意GET_PRE_PTR得到不是pre 在free bliok中的地址，而是这个地址中的值
#define GET_PRE_PTR(bp) (*(char **)(bp))
#define GET_NXT_PTR(bp) (*(char **)(bp + WSIZE))

#define SET_NEXT_PTR(bp, addr) (GET_NEXT_PTR(bp) = addr)
#define SET_PREV_PTR(bp, addr) (GET_PREV_PTR(bp) = addr)

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);

// 定义一些global 的var
static char *heap_listp = 0;
static char *free_list_start = 0;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  printf("mm_init begin\n");
  if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)  // line:vm:mm:begininit
    return -1;

  // 保留开头和结尾块的设计，方便合并 的时候处理corner case
  PUT(heap_listp, 0);                            /* Alignment padding */
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
  heap_listp += (2 * WSIZE);                     // line:vm:mm:endinit
  // 链表头位于下次要分配的位置的头部
  free_list_start = heap_listp + 2 * WSIZE;
  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
  printf("mm_init finish with heap_listp[%p]  free_list_start[%p]\n",
         heap_listp, free_list_start);
  return 0;
}

// 每次扩展words个字的内存，一个字是4
// byte,也就是扩展words*4的Byte内存，words向上对偶数取整
static void *extend_heap(size_t words) {
  printf("in extend_heap function\n");
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE
                     : words * WSIZE;                  // line:vm:mm:beginextend
  if ((long)(bp = mem_sbrk(size)) == -1) return NULL;  // line:vm:mm:endextend
  printf("size in extend_heap:%d\n", size);

  /* Initialize free block header/footer and the epilogue header */
  PUT(HDRP(bp), PACK(size, 0));
  /* Free block header */  // line:vm:mm:freeblockhdr
  PUT(FTRP(bp), PACK(size, 0));
  /* Free block footer */  // line:vm:mm:freeblockftr
  PUT(HDRP(NEXT_BLK(bp)), PACK(0, 1));
  /* New epilogue header */  // line:vm:mm:newepihdr

  /* Coalesce if the previous block was free */
  // return coalesce(bp);  // line:vm:mm:returnblock
  return bp;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  // ignore
  printf("malloc begin of size:[%d] free_list_start:[%p]\n", size,
         free_list_start);
  if (size == 0) return NULL;
  // adjusted block size
  size_t asize = size <= DSIZE
                     ? 2 * DSIZE
                     : DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  printf("malloc ajust size:%d\n", asize);
  char *bp;
  if ((bp = find_fit(asize)) != NULL) {
    printf("find a free block suitable for size[%d]\n", asize);
    place(bp, 0);
    return bp;
  }
  printf("cannot find a free block suitable for [%d]\n", asize);

  size_t extend_size = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extend_size / WSIZE)) == NULL) return NULL;
  place(bp, 0);
  return bp;
}

// Place block of asize bytes at start of free block bp
// no split now
static void *find_fit(size_t asize) {
  printf("find fit begin of size[%d]\n", asize);
  void *bp;
  for (bp = free_list_start; bp && GET_SIZE(HDRP(bp)) > 0;
       bp = GET_NXT_PTR(bp)) {
    printf("in find_fit for loop, cur bp:[%p]\n", bp);
    if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
      printf("find suitable free block[%p] for size[%d]\n", bp, asize);
      printf("GET_SIZE(HDRP(bp))):%d\n", GET_SIZE(HDRP(bp)));
      return bp;
    }
    printf("in find_fit for loop end if\n");
  }
  printf("cannot find any free block in find_fit\n");

  return NULL;
}

// place block
// 不进行split,直接将一整个free block占满
static void place(void *bp, size_t asize) {
  printf("place begin at %p\n", bp);
  size_t csize = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(csize, 1));
  PUT(FTRP(bp), PACK(csize, 1));
  printf("place end \n");
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) { printf("mm free begin\n"); }

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
  void *oldptr = ptr;
  void *newptr;
  size_t copySize;

  newptr = mm_malloc(size);
  if (newptr == NULL) return NULL;
  copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
  if (size < copySize) copySize = size;
  memcpy(newptr, oldptr, copySize);
  mm_free(oldptr);
  return newptr;
}

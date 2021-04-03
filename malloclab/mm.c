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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memlib.h"

// for debug purpose
#define print(a, args...) \
  printf("%s(%s:%d) " a, __func__, __FILE__, __LINE__, ##args)

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
    "hust.111qqz@gmail.com",
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
// #define GET_SIZE(p) (GET(p) & ~0x7)  // line:vm:mm:getsize
#define GET_SIZE(p) (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p) (GET(p) & 0x1)  // line:vm:mm:getalloc
#define GET_LAST_THREE_BIT(p) (GET(p) & 0x7)

// bp might means "base pointer",是一个block中有效载荷的起始位置
/* Given block ptr bp, compute address of its header and footer */
//
// 一种值得提的特殊情况是。。对于序言快。。footer可以看作是bp,
// 因为footer往前WSIZE也恰好是header
#define HDRP(bp) ((char *)(bp)-WSIZE)  //
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)
// line:vm:mm:ftrp

#define NEXT_BLK(bp) ((void *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLK(bp) ((void *)(bp)-GET_SIZE((void *)(bp)-DSIZE))

/* Given block ptr bp, compute address of next and previous blocks */

// explict free list的想法是，对于每一个free block，存放上一个free
// block的地址和下一个free block的地址在payload中(因为free
// block的payload反正也没有被使用) 用于explict free list
// 需要注意GET_PREV_PTR得到不是pre 在free bliok中的地址，而是这个地址中的值
#define GET_PREV_PTR(bp) (*(char **)(bp))
#define GET_NEXT_PTR(bp) (*(char **)(bp + WSIZE))

#define SET_NEXT_PTR(bp, addr) (GET_NEXT_PTR(bp) = addr)
#define SET_PREV_PTR(bp, addr) (GET_PREV_PTR(bp) = addr)

/* Function prototypes for internal helper routines */
static void *extend_heap(size_t words);
static void place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);

static void remove_node_from_free_list(void *bp);
static void insert_node_in_free_list(void *bp);

// 定义一些global 的var
static char *heap_listp = 0;
static char *free_list_start = 0;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
  if ((heap_listp = mem_sbrk(8 * WSIZE)) == NULL)  // line:vm:mm:begininit
    return -1;
  // 序言快很重要的一点是, HDRP和FTPR中间没有payload,
  //     这其实意味着，可以把footer当成bp,
  //     往前4个字节就是header
  // 因此把free_list_start
  //
  // 放在序言快的footer的部分，作为一个结尾标志。判断条件可以统一成SIZE(HEAD(bp))
  // = 0 这里是个挺巧妙的做法。
  // 以及，由于是头插，其实free_list_start更合适的叫法是free_list_end?

  // 保留开头和结尾块的设计，方便合并 的时候处理corner case
  PUT(heap_listp, 0);                            /* Alignment padding */
  PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
  PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
  PUT(heap_listp + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */

  free_list_start = heap_listp + 2 * WSIZE;
  if (extend_heap(CHUNKSIZE / WSIZE) == NULL) return -1;
  return 0;
}

// 每次扩展words个字的内存，一个字是4
// byte,也就是扩展words*4的Byte内存，words向上对偶数取整
static void *extend_heap(size_t words) {
  // print("in extend_heap function\n");
  char *bp;
  size_t size;

  /* Allocate an even number of words to maintain alignment */
  size = (words % 2) ? (words + 1) * WSIZE
                     : words * WSIZE;  // line:vm:mm:beginextend
  if (size < 16) {
    size = 16;
  }
  if ((long)(bp = mem_sbrk(size)) == NULL) return NULL;

  /* Initialize free block header/footer and the epilogue header */

  PUT(HDRP(bp), PACK(size, 0));
  /* Free block header */  // line:vm:mm:freeblockhdr
  PUT(FTRP(bp), PACK(size, 0));
  /* Free block footer */  // line:vm:mm:freeblockftr
  PUT(HDRP(NEXT_BLK(bp)), PACK(0, 1));
  // PUT(FTRP(NEXT_BLK(bp)), PACK(0, 1));
  /* New epilogue header */  // line:vm:mm:newepihdr

  /* Coalesce if the previous block was free */
  // coelesce 返回合并相邻空闲块之后的块地址
  bp = coalesce(bp);
  insert_node_in_free_list(bp);
  return bp;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
  // ignore
  if (size == 0) return NULL;
  // adjusted block size
  size_t asize = size <= DSIZE
                     ? 2 * DSIZE
                     : DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
  char *bp;
  if ((bp = find_fit(asize)) != NULL) {
    // print("find a free block suitable for size[%d]\n", asize);
    place(bp, asize);
    return bp;
  }

  size_t extend_size = MAX(asize, CHUNKSIZE);
  if ((bp = extend_heap(extend_size / WSIZE)) == NULL) return NULL;
  place(bp, asize);
  return bp;
}

// 合并连续的free block
static void *coalesce(void *bp) {
  size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLK(bp))) || PREV_BLK(bp) == bp;
  size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
  size_t size = GET_SIZE(HDRP(bp));
  // print("pre_alloc:[%d] next_alloc:[%d] size:[%d]\n", prev_alloc, next_alloc,
  // size);
  if (prev_alloc && next_alloc) {
    return bp;
  }

  if (prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLK(bp)));
    // 下一块和当前合并了，在free list中变成一个node,因此删除下一块
    remove_node_from_free_list(NEXT_BLK(bp));
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    return bp;
  }

  if (!prev_alloc && next_alloc) {
    size += GET_SIZE(HDRP(PREV_BLK(bp)));
    bp = PREV_BLK(bp);
    // 删除的是pre,之后再插入的是prev+cur. 虽然地址是一样的
    remove_node_from_free_list(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    return bp;
  }

  if (!prev_alloc && !next_alloc) {
    size += GET_SIZE(HDRP(NEXT_BLK(bp))) + GET_SIZE(HDRP(PREV_BLK(bp)));
    remove_node_from_free_list(PREV_BLK(bp));
    remove_node_from_free_list(NEXT_BLK(bp));
    bp = PREV_BLK(bp);
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));

    return bp;
  }
  print("never run here\n");
  return NULL;
}

// Place block of asize bytes at start of free block bp
// no split now
static void *find_fit(size_t asize) {
  // print("find fit begin of size[%d]\n", asize);
  void *bp;
  for (bp = free_list_start; GET_ALLOC(HDRP(bp)) == 0; bp = GET_NEXT_PTR(bp)) {
    if ((asize <= GET_SIZE(HDRP(bp)))) {
      return bp;
    }
  }

  return NULL;
}

// place block
static void place(void *bp, size_t asize) {
  // csize是这个free block的大小
  // asize是这次malloc需要的大小。 rsize(r means "remain")就是剩下的大小
  // 逻辑是，如果剩下的比较多，避免浪费就进行拆分，否则不进行拆分
  size_t csize = GET_SIZE(HDRP(bp));
  size_t rsize = csize - asize;
  if (rsize >= (2 * DSIZE)) {
    PUT(HDRP(bp), PACK(asize, 1));
    PUT(FTRP(bp), PACK(asize, 1));
    remove_node_from_free_list(bp);
    bp = NEXT_BLK(bp);
    PUT(HDRP(bp), PACK(rsize, 0));
    PUT(FTRP(bp), PACK(rsize, 0));
    bp = coalesce(bp);
    insert_node_in_free_list(bp);

  } else {
    PUT(HDRP(bp), PACK(csize, 1));
    PUT(FTRP(bp), PACK(csize, 1));
    remove_node_from_free_list(bp);
  }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
  if (bp == NULL) return;
  size_t size = GET_SIZE(HDRP(bp));
  PUT(HDRP(bp), PACK(size, 0));
  PUT(FTRP(bp), PACK(size, 0));
  bp = coalesce(bp);
  insert_node_in_free_list(bp);
}

/*
 * mm_realloc - 借鉴了 https://github.com/HarshTrivedi/malloc
 */
void *mm_realloc(void *bp, size_t size) {
  if (bp == NULL) {
    return mm_malloc(size);
  }
  if (size == 0) {
    mm_free(bp);
    return NULL;
  }
  size_t oldsize = GET_SIZE(HDRP(bp));
  size_t newsize = size + 2 * WSIZE;  // 2 words for header and footer
  /*if newsize is less than oldsize then we just return bp */
  if (newsize <= oldsize) {
    return bp;
  }
  /*if newsize is greater than oldsize */
  else {
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLK(bp)));
    size_t csize;
    /* next block is free and the size of the two blocks is greater than or
     * equal the new size  */
    /* then we only need to combine both the blocks  */
    if (!next_alloc &&
        ((csize = oldsize + GET_SIZE(HDRP(NEXT_BLK(bp))))) >= newsize) {
      remove_node_from_free_list(NEXT_BLK(bp));
      PUT(HDRP(bp), PACK(csize, 1));
      PUT(FTRP(bp), PACK(csize, 1));
      return bp;
    } else {
      void *new_ptr = mm_malloc(newsize);
      place(new_ptr, newsize);
      memcpy(new_ptr, bp, newsize);
      mm_free(bp);
      return new_ptr;
    }
  }
  return NULL;
}

// remove node in free list
// a->b->c ，remove b的做法是，将a->next指向c,c->prev 指向a
// 特殊情况: bp是free list的初始节点，此时直接调整free_list_start
static void remove_node_from_free_list(void *bp) {
  if (GET_PREV_PTR(bp)) {
    SET_NEXT_PTR(GET_PREV_PTR(bp), GET_NEXT_PTR(bp));
  } else {
    free_list_start = GET_NEXT_PTR(bp);
  }
  SET_PREV_PTR(GET_NEXT_PTR(bp), GET_PREV_PTR(bp));
}

// 向free list中插入node
// 这里涉及插入到哪里的策略
// 暂时使用头插法,也就是每次插入到free list的头部
// 对于 b->c,插入a,做法是
// a->next = free_list_start
// a->prev = null
// free_list_start->prev = a
// free_list_start = a

static void insert_node_in_free_list(void *bp) {
  SET_NEXT_PTR(bp, free_list_start);
  SET_PREV_PTR(free_list_start, bp);
  SET_PREV_PTR(bp, NULL);
  free_list_start = bp;
}

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
#include <stddef.h>
#include <stdbool.h>

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
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

/*=============================================================
 *                    Macros
 *============================================================*/
#define WSIZE 4             
#define DSIZE 8
#define CHUNKSIZE (1 << 12)

#define PACK(size, prev_alloc, alloc) ((size) | ((prev_alloc)<<1) | (alloc))

#define GET(p) (*(word_t *)(p))
#define PUT(p, val) (*(word_t *)(p) = (val))

#define GET_SIZE(p)  (GET(p) & ~0x7)         // 去掉低3位
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PREV_ALLOC(p) ((GET(p) >> 1) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)          // 指向 header
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


// dealing with seglist
#define NUM_CLASSES 21
static void *seglist[NUM_CLASSES]; 

const size_t size_classes[NUM_CLASSES] = {
    16, 32, 48, 64, 80, 96, 112, 128,
    144, 160, 176, 192, 208, 224, 240, 256,
    512, 1024, 2048, 4096, SIZE_MAX
};

static int get_index(size_t size) {
    for (int i = 0; i < NUM_CLASSES; ++i) {
        if (size <= size_classes[i])
            return i;
    }
    return NUM_CLASSES - 1;  // fallback
}

static void insert_block(void * bp, size_t size){
    int index = get_index(size);
    void ** head = &(seglist[index]); 
    void * next = * head;
    *(void **)bp = next;
    *((void **)bp + 1) = NULL;
    if(next){
        *((void **)next + 1) = bp;
    }
    *head = bp;
}

static void remove_block(void * bp, size_t size){
    int index = get_index(size);
    void * next = *(void **)bp;
    void * prev = *((void **)bp + 1);
    if(prev){
        *(void **)prev = next;
    }
    else{
        seglist[index] = next;
    }
    if(next){
        *((void **)next + 1) = prev;
    }
}

static void * coalesce(void * bp){
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(!next_alloc){
        void * next = NEXT_BLKP(bp);
        remove_block(next, GET_SIZE(HDRP(next)));
        size += GET_SIZE(HDRP(next));
    }
    if(!prev_alloc){
        void * prev = PREV_BLKP(bp);
        remove_block(prev, GET_SIZE(HDRP(prev)));
        size += GET_SIZE(HDRP(prev));
        bp = prev;
    }
    PUT(HDRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0));
    PUT(FTRP(bp), PACK(size, GET_PREV_ALLOC(HDRP(bp)), 0));
    insert_block(bp, size);

    //
    void *next_block = NEXT_BLKP(bp);
    if (GET_SIZE(HDRP(next_block)) != 0) {
        size_t next_size = GET_SIZE(HDRP(next_block));
        size_t next_alloc = GET_ALLOC(HDRP(next_block));
        PUT(HDRP(next_block), PACK(next_size, 0, next_alloc)); // 设置prev_alloc=0
    }
    return bp;
}

static void * extend_heap(size_t words){
    size_t asize = ALIGN(words * WSIZE);
    void * bp = mem_sbrk(asize);
    if(bp == (void * )-1){
        return NULL;
    }
    size_t prev_alloc = GET_PREV_ALLOC(HDRP(bp));
    PUT(HDRP(bp), PACK(asize, prev_alloc, 0));
    PUT(FTRP(bp), PACK(asize, prev_alloc, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(1, 0, 1));
    return coalesce(bp);
}

static void * find_fit(size_t asize){
    int index = get_index(asize);
    for (int i = index; i < NUM_CLASSES; ++i) {
        void *bp = seglist[i];
        while (bp) {
            if (GET_SIZE(HDRP(bp)) >= asize)
                return bp;
            bp = *(void **)bp;
        }
    }
    return NULL;
}

static void place(void * bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    remove_block(bp, csize); 
    if(csize - asize >= 2 *DSIZE){
        PUT(HDRP(bp), PACK(asize, GET_PREV_ALLOC(HDRP(bp)), 1));
        PUT(FTRP(bp), PACK(asize, GET_PREV_ALLOC(HDRP(bp)), 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(csize - asize, 1, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(csize - asize, 1, 0));
        insert_block(NEXT_BLKP(bp), csize - asize);
    }
    else{
        PUT(HDRP(bp), PACK(csize, GET_PREV_ALLOC(HDRP(bp)), 1));
        PUT(FTRP(bp), PACK(csize, GET_PREV_ALLOC(HDRP(bp)), 1));
        PUT(HDRP(NEXT_BLKP(bp)), PACK(GET_SIZE(HDRP(NEXT_BLKP(bp))), 1, GET_ALLOC(HDRP(NEXT_BLKP(bp)))));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(GET_SIZE(HDRP(NEXT_BLKP(bp))), 1, GET_ALLOC(HDRP(NEXT_BLKP(bp)))));
    }
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    // init seglist
    for(int i = 0; i < NUM_CLASSES; i++){
        seglist[i] = NULL;
    }

    // init heap
    void *heap = mem_sbrk(4 * WSIZE);
    if(heap == (void * )-1)
        return -1;
    PUT(heap, 0);
    PUT(heap + WSIZE, PACK(DSIZE, 1, 1));
    PUT(heap + 2 * WSIZE, PACK(DSIZE, 1, 1));
    PUT(heap + 3 * WSIZE, PACK(1, 1, 1));
    return extend_heap(CHUNKSIZE / WSIZE) != NULL ? 0 : -1;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    if(size == 0)
        return NULL;
    size_t asize = ALIGN(size + DSIZE);
    void * bp = find_fit(asize);
    if(!bp)
        bp = extend_heap(asize / WSIZE);
    if(!bp)
        return NULL;
    place(bp, asize);
    return bp;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    if(ptr==0)
        return;
    size_t size = GET_SIZE(HDRP(ptr));
    PUT(HDRP(ptr), PACK(size, GET_PREV_ALLOC(HDRP(ptr)), 0));
    PUT(FTRP(ptr), PACK(size, GET_PREV_ALLOC(HDRP(ptr)), 0));
    coalesce(ptr);
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */


// void *mm_realloc(void *ptr, size_t size)
// {
//     if (!ptr)
//         return mm_malloc(size);
//     if (size == 0) {
//         mm_free(ptr);
//         return NULL;
//     }
//     void *newptr = mm_malloc(size);
//     if (!newptr)
//         return NULL;
//     size_t oldsize = GET_SIZE(HDRP(ptr)) - DSIZE;
//     if (size < oldsize)
//         oldsize = size;
//     memcpy(newptr, ptr, oldsize);
//     mm_free(ptr);
//     return newptr;
// }

// void *mm_realloc(void *ptr, size_t size)
// {
//     if (ptr == NULL)
//         return mm_malloc(size);
//     if (size == 0) {
//         mm_free(ptr);
//         return NULL;
//     }

//     size_t oldsize = GET_SIZE(HDRP(ptr));
//     size_t asize = ALIGN(size + DSIZE);

//     // 如果原块就够用，不动
//     if (asize <= oldsize) {
//         return ptr;
//     }

//     // 尝试原地扩展到下一块（如果下一块是 free）
//     void *next_bp = NEXT_BLKP(ptr);
//     if (!GET_ALLOC(HDRP(next_bp))) {
//         size_t combined_size = oldsize + GET_SIZE(HDRP(next_bp));
//         if (combined_size >= asize) {
//             remove_block(next_bp, GET_SIZE(HDRP(next_bp)));
//             PUT(HDRP(ptr), PACK(combined_size, GET_PREV_ALLOC(HDRP(ptr)), 1));
//             PUT(FTRP(ptr), PACK(combined_size, GET_PREV_ALLOC(HDRP(ptr)), 1));
//             return ptr;
//         }
//     }

//     // 否则只好分配新空间
//     void *newptr = mm_malloc(size);
//     if (!newptr)
//         return NULL;
//     size_t copySize = oldsize - DSIZE;
//     if (size < copySize)
//         copySize = size;
//     memcpy(newptr, ptr, copySize);
//     mm_free(ptr);
//     return newptr;
// }

/* mm_realloc*/
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return NULL;
    }

    size_t oldsize = GET_SIZE(HDRP(ptr)) - DSIZE;
    size_t asize = ALIGN(size + DSIZE);

    if (asize <= oldsize) {
        return ptr;
    }

    // 尝试原地扩展到下一块（如果下一块是free）
    void *next_bp = NEXT_BLKP(ptr);
    size_t next_size = GET_SIZE(HDRP(next_bp));
    size_t next_alloc = GET_ALLOC(HDRP(next_bp));
    if (!next_alloc) {
        size_t combined_size = oldsize + DSIZE + next_size; // 修复：加上头部大小
        if (combined_size >= asize) {
            remove_block(next_bp, next_size);
            PUT(HDRP(ptr), PACK(combined_size, GET_PREV_ALLOC(HDRP(ptr)), 1));
            PUT(FTRP(ptr), PACK(combined_size, GET_PREV_ALLOC(HDRP(ptr)), 1));
            
            // 更新下一个块的prev_alloc标志
            void *next_next = NEXT_BLKP(ptr);
            if (GET_SIZE(HDRP(next_next)) != 0) {
                size_t nn_size = GET_SIZE(HDRP(next_next));
                size_t nn_alloc = GET_ALLOC(HDRP(next_next));
                PUT(HDRP(next_next), PACK(nn_size, 1, nn_alloc));
            }
            
            // 如果合并后还有剩余空间，分割块
            if (combined_size - asize >= 2 * DSIZE) {
                PUT(HDRP(ptr), PACK(asize, GET_PREV_ALLOC(HDRP(ptr)), 1));
                PUT(FTRP(ptr), PACK(asize, GET_PREV_ALLOC(HDRP(ptr)), 1));
                
                void *remainder = NEXT_BLKP(ptr);
                size_t rem_size = combined_size - asize;
                PUT(HDRP(remainder), PACK(rem_size, 1, 0));
                PUT(FTRP(remainder), PACK(rem_size, 1, 0));
                
                // 更新剩余块的下一个块的prev_alloc标志
                void *next_block = NEXT_BLKP(remainder);
                if (GET_SIZE(HDRP(next_block)) != 0) {
                    size_t nb_size = GET_SIZE(HDRP(next_block));
                    size_t nb_alloc = GET_ALLOC(HDRP(next_block));
                    PUT(HDRP(next_block), PACK(nb_size, 0, nb_alloc));
                }
                
                insert_block(remainder, rem_size);
            }
            return ptr;
        }
    }

    // 否则分配新空间
    void *newptr = mm_malloc(size);
    if (!newptr)
        return NULL;
    size_t copySize = oldsize; // 旧有效负载大小
    if (size < copySize)
        copySize = size;
    memcpy(newptr, ptr, copySize);
    mm_free(ptr);
    return newptr;
}












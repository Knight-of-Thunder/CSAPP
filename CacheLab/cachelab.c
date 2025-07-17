/*
 * cachelab.c - Cache Lab helper functions
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "cachelab.h"
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <string.h>
#include <math.h>

trans_func_t func_list[MAX_TRANS_FUNCS];
int func_counter = 0; 

void printSummary(int hits, int misses, int evictions) {
    printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
    FILE* output_fp = fopen(".csim_results", "w");
    assert(output_fp);
    fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
    fclose(output_fp);
}


/* 
 * initMatrix - Initialize the given matrix 
 */
void initMatrix(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;
    srand(time(NULL));
    for (i = 0; i < N; i++){
        for (j = 0; j < M; j++){
            // A[i][j] = i+j;  /* The matrix created this way is symmetric */
            A[i][j]=rand();
            B[j][i]=rand();
        }
    }
}

void randMatrix(int M, int N, int A[N][M]) {
    int i, j;
    srand(time(NULL));
    for (i = 0; i < N; i++){
        for (j = 0; j < M; j++){
            // A[i][j] = i+j;  /* The matrix created this way is symmetric */
            A[i][j]=rand();
        }
    }
}

/* 
 * correctTrans - baseline transpose function used to evaluate correctness 
 */
void correctTrans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;
    for (i = 0; i < N; i++){
        for (j = 0; j < M; j++){
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    
}



/* 
 * registerTransFunction - Add the given trans function into your list
 *     of functions to be tested
 */
void registerTransFunction(void (*trans)(int M, int N, int[N][M], int[M][N]), 
                           char* desc)
{
    func_list[func_counter].func_ptr = trans;
    func_list[func_counter].description = desc;
    func_list[func_counter].correct = 0;
    func_list[func_counter].num_hits = 0;
    func_list[func_counter].num_misses = 0;
    func_list[func_counter].num_evictions =0;
    func_counter++;
}

static uint16_t getSet(uint64_t addr) {
    return (addr >> args.b) & ((1 << args.s) - 1);
}

static uint64_t getTag(uint64_t addr) {
    return addr >> (args.s + args.b);
}

struct cache_entry **cache_simulator = NULL;




struct cache_entry * getLRU(uint64_t addr){
    struct cache_entry * head = cache_simulator[getSet(addr)];
    struct cache_entry * victim = head->prev;
    return victim;
}



static void printHelp() {
    printf("Usage: ./csim [-hv] -s <s> -E <E> -b <b> -t <tracefile>\n"
           "  -h            Print usage info\n"
           "  -v            Verbose mode\n"
           "  -s <s>        Number of set index bits\n"
           "  -E <E>        Associativity\n"
           "  -b <b>        Number of block bits\n"
           "  -t <tracefile> Trace file to replay\n");
}

int hits = 0;
int misses = 0;
int evictions = 0;
struct command_args args = {0};

void parseArgs(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt) {
            case 'h': printHelp(); exit(0);
            case 'v': args.v = 1; break;
            case 'E': args.E = atoi(optarg); break;
            case 's': args.s = atoi(optarg); break;
            case 'b': args.b = atoi(optarg); break;
            case 't': args.t = optarg; break;
        }
    }
}

void initCache() {
    int sets = 1 << args.s;
    cache_simulator = malloc(sizeof(struct cache_entry*) * sets);
    for (int i = 0; i < sets; i++) {
        struct cache_entry *head = calloc(1, sizeof(struct cache_entry));
        head->prev = head;
        head->next = head;
        for (int j = 0; j < args.E; j++) {
            struct cache_entry *entry = calloc(1, sizeof(struct cache_entry));
            head->prev->next = entry;
            entry->prev = head->prev;
            entry->next = head;
            head->prev = entry;
        }
        cache_simulator[i] = head;
    }
}



void freeCache() {
    int sets = 1 << args.s;
    int lines = args.E;
    for (int i = 0; i < sets; i++) {
        struct cache_entry *p = cache_simulator[i];
        for (int j = 0; j < lines + 1; j++) {
            struct cache_entry *tmp = p;
            p = p->next;
            free(tmp);
        }
    }
    free(cache_simulator);
}


// get the block in cache
struct cache_entry * getCacheBlock(unsigned long addr){
  unsigned short set = getSet(addr);
  unsigned long tag = getTag(addr);
  struct cache_entry * head = cache_simulator[set];
  struct cache_entry * p = head->next;
  for(int i = 0; i < args.E; i++){
    if(tag == p->tag && p->valid == 1){
      return p;
    }
    p = p->next;
  }
  return NULL;
}

// NOTE: LRU
void storeHit(struct cache_entry *hit_block, uint64_t addr) {
    hit_block->tag = getTag(addr);
}

void storeMissed(uint64_t addr) {
    struct cache_entry *victim = getLRU(addr);
    victim->valid = 1;
    victim->tag = getTag(addr);
}

int needEvicted(uint64_t addr) {
    struct cache_entry *victim = getLRU(addr);
    return victim && victim->valid;
}

static uint64_t getAccessAddr(char *access_command) {
    char *c = access_command + 2;
    return strtoul(strtok(c, ","), NULL, 16);
}


void reorderByLRU(uint64_t addr){
    int set = getSet(addr);
    struct cache_entry * recent_block = getCacheBlock(addr);
    recent_block->prev->next = recent_block->next;
    recent_block->next->prev = recent_block->prev;

    struct cache_entry * head = cache_simulator[set];
    head->next->prev = recent_block;
    recent_block->next = head->next;
    head->next = recent_block;
    recent_block->prev = head;
    
}

void handleLoad(uint64_t addr) {
    struct cache_entry *block = getCacheBlock(addr);
    if (block) {
        if (args.v) printf(" hit");
        hits++;
    } else {
        if (args.v) printf(" miss");
        if (needEvicted(addr)) {
            if (args.v) printf(" eviction");
            evictions++;
        }
        storeMissed(addr);
        misses++;
    }
    reorderByLRU(addr);
}

void handleStore(uint64_t addr) {
    struct cache_entry *block = getCacheBlock(addr);
    if (block) {
        if (args.v) printf(" hit");
        storeHit(block, addr);
        hits++;
    } else {
        if (args.v) printf(" miss");
        if (needEvicted(addr)) {
            if (args.v) printf(" eviction");
            evictions++;
        }
        storeMissed(addr);
        misses++;
    }
    reorderByLRU(addr);
}

void handleModify(uint64_t addr) {
    handleLoad(addr);
    handleStore(addr);
}

void excuteAccess(char *access_command) {
    if (*access_command++ != ' ') return;

    if (args.v) {
        access_command[strcspn(access_command, "\n")] = '\0';
        printf("%s", access_command);
    }

    uint64_t addr = getAccessAddr(access_command);
    switch (*access_command) {
        case 'M': handleModify(addr); break;
        case 'L': handleLoad(addr); break;
        case 'S': handleStore(addr); break;
    }

    if (args.v) printf("\n");
}

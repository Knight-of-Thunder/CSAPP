/* 
 * cachelab.h - Prototypes for Cache Lab helper functions
 */

#ifndef CACHELAB_TOOLS_H
#define CACHELAB_TOOLS_H

#define MAX_TRANS_FUNCS 100

typedef struct trans_func{
  void (*func_ptr)(int M,int N,int[N][M],int[M][N]);
  char* description;
  char correct;
  unsigned int num_hits;
  unsigned int num_misses;
  unsigned int num_evictions;
} trans_func_t;

struct command_args {
    int v;
    int s;
    int E;
    int b;
    char *t;
};

extern struct command_args args;  // 只是声明，不分配空间


struct cache_entry
{
  struct cache_entry * prev;
  struct cache_entry * next;
  int valid;
  int tag;
};

// int hits = 0;
// int misses = 0;
// int evictions = 0;

extern int hits;
extern int misses;
extern int evictions;


extern struct cache_entry ** cache_simulator;

/* 
 * printSummary - This function provides a standard way for your cache
 * simulator * to display its final hit and miss statistics
 */ 
void printSummary(int hits,  /* number of  hits */
				  int misses, /* number of misses */
				  int evictions); /* number of evictions */

/* Fill the matrix with data */
void initMatrix(int M, int N, int A[N][M], int B[M][N]);

/* The baseline trans function that produces correct results. */
void correctTrans(int M, int N, int A[N][M], int B[M][N]);

/* Add the given function to the function list */
void registerTransFunction(
    void (*trans)(int M,int N,int[N][M],int[M][N]), char* desc);

// parse the command-line argument
void parseArgs(int argc, char *argv[]);

// Init cache
void initCache();

// Free cache for memory allocated by malloc
void freeCache();

// parse and excute each memory access in trace file
void excuteAccess(char * access_command);
#endif /* CACHELAB_TOOLS_H */

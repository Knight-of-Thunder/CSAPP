/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void trans_32x32(int M, int N, int A[N][M], int B[M][N]);
void trans_64x64(int M, int N, int A[N][M], int B[M][N]);
void trans_61x67(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    if(M == 32)
        trans_32x32(M, N, A, B);
    else if(M == 64)
        trans_32x32(M, N, A, B);
    else
        trans_61x67(M, N, A, B);
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

// 32x32
// use 8x8 block to optimize transposition
char trans_32x32_desc[] = "dealing with 32 x 32 matrix";
void trans_32x32(int M, int N, int A[N][M], int B[M][N]){
    // for(int i = 0; i < N; i += 8){
    //     for(int j = 0; j < M; j += 8){
    //         // 8 x 8 block handling
    //         for(int i1 = 0; i1 < 8; i1++){
    //             // store a line in local variable
    //             int a0 = A[i + i1][j];
    //             int a1 = A[i + i1][j + 1];                
    //             int a2 = A[i + i1][j + 2];
    //             int a3 = A[i + i1][j + 3];
    //             int a4 = A[i + i1][j + 4];
    //             int a5 = A[i + i1][j + 5];
    //             int a6 = A[i + i1][j + 6];
    //             int a7 = A[i + i1][j + 7];
    //             B[j][i + i1] = a0;
    //             B[j + 1][i + i1] = a1;
    //             B[j + 2][i + i1] = a2;
    //             B[j + 3][i + i1] = a3;
    //             B[j + 4][i + i1] = a4;
    //             B[j + 5][i + i1] = a5;
    //             B[j + 6][i + i1] = a6;
    //             B[j + 7][i + i1] = a7;                    
    //         }
    //     }
    // }

    // a better implementation: simplify the code 
    for (int i = 0; i < N; i += 8) {
        int tmp; int diag;
        for (int j = 0; j < M; j += 8) {
            for (int ii = i; ii < i + 8; ++ii) {
                for (int jj = j; jj < j + 8; ++jj) {
                    if (ii != jj)
                        B[jj][ii] = A[ii][jj];
                    else
                        tmp = A[ii][jj], diag = ii;
                }
                if (i == j) B[diag][diag] = tmp;
            }
        }
    }
}

// 64x64
char trans_64x64_desc[] = "dealing with 64 x 64 matrix";
void trans_64x64(int M, int N, int A[N][M], int B[M][N]){
    int a0, a1, a2, a3, a4,a5, a6, a7;
    for (int i = 0; i < N; i += 8) {
        for (int j = 0; j < M; j += 8) {
            for (int k = i; k < i + 4; k++) {
                // 
                a0 = A[k][j];
                a1 = A[k][j + 1];
                a2 = A[k][j + 2];
                a3 = A[k][j + 3];
                a4 = A[k][j + 4];
                a5 = A[k][j + 5];
                a6 = A[k][j + 6];
                a7 = A[k][j + 7];

                //
                B[j][k] = a0;
                B[j + 1][k] = a1;
                B[j + 2][k] = a2;
                B[j + 3][k] = a3;
                B[j + 0][k + 4] = a4;
                B[j + 1][k + 4] = a5;
                B[j + 2][k + 4] = a6;
                B[j + 3][k + 4] = a7;

            }

            for(int k = j; k < j + 4; k++){
                a0 = B[k][i + 4];
                a1 = B[k][i + 5];
                a2 = B[k][i + 6];
                a3 = B[k][i + 7];

                B[k][i + 4] = A[i + 4][k];
                B[k][i + 5] = A[i + 5][k];
                B[k][i + 6] = A[i + 6][k];
                B[k][i + 7] = A[i + 7][k];

                B[k + 4][i] = a0;
                B[k + 4][i + 1] = a1;
                B[k + 4][i + 2] = a2;
                B[k + 4][i + 3] = a3;
            }

            for(int k = i + 4; k < i + 8; k++){
                B[j + 4][k] = A[k][j + 4];
                B[j + 5][k] = A[k][j + 5];
                B[j + 6][k] = A[k][j + 6];
                B[j + 7][k] = A[k][j + 7];
            }
        }
    }
    
}


char trans_61x67_desc[] = "dealing with 61 x 67 matrix";
void trans_61x67(int M, int N, int A[N][M], int B[M][N]){
    for (int i = 0; i < N; i += 16) {
        for (int j = 0; j < M; j += 16) {
            for (int ii = i; ii < i + 16 && ii < N; ++ii) {
                for (int jj = j; jj < j + 16 && jj < M; ++jj) {
                        B[jj][ii] = A[ii][jj];
                }
            }
        }
    }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc); 
    registerTransFunction(trans_32x32, trans_32x32_desc);
    registerTransFunction(trans_64x64, trans_64x64_desc);
    registerTransFunction(trans_61x67, trans_61x67_desc);

}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}


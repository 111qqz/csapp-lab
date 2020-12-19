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
int min(int x, int y) { return x < y ? x : y; }
/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
  if (N == 32) {
    int tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    for (int i = 0; i < N; i += 8) {
      for (int j = 0; j < M; j++) {
        tmp = A[i][j];
        tmp1 = A[i + 1][j];
        tmp2 = A[i + 2][j];
        tmp3 = A[i + 3][j];
        tmp4 = A[i + 4][j];
        tmp5 = A[i + 5][j];
        tmp6 = A[i + 6][j];
        tmp7 = A[i + 7][j];
        B[j][i] = tmp;
        B[j][i + 1] = tmp1;
        B[j][i + 2] = tmp2;
        B[j][i + 3] = tmp3;
        B[j][i + 4] = tmp4;
        B[j][i + 5] = tmp5;
        B[j][i + 6] = tmp6;
        B[j][i + 7] = tmp7;
      }
    }
  } else if (N == 67) {
    // int tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    int tmp[8];
    for (int block_i_start = 0; block_i_start < N; block_i_start += 8) {
      for (int block_j_start = 0; block_j_start < M; block_j_start += 8) {
        for (int i = block_i_start; i < min(block_i_start + 8, N); i++) {
          for (int j = block_j_start; j < min(M, block_j_start + 8); j++) {
            tmp[j - block_j_start] = A[i][j];
          }
          for (int j = block_j_start; j < min(M, block_j_start + 8); j++) {
            B[j][i] = tmp[j - block_j_start];
          }
          //   tmp = A[i][block_j_start];
          //   tmp1 = A[i][block_j_start + 1];
          //   tmp2 = A[i][block_j_start + 2];
          //   tmp3 = A[i][block_j_start + 3];
          //   tmp4 = A[i][block_j_start + 4];
          //   tmp5 = A[i][block_j_start + 5];
          //   tmp6 = A[i][block_j_start + 6];
          //   tmp7 = A[i][block_j_start + 7];
          //   B[block_j_start][i] = tmp;
          //   B[block_j_start + 1][i] = tmp1;
          //   B[block_j_start + 2][i] = tmp2;
          //   B[block_j_start + 3][i] = tmp3;
          //   B[block_j_start + 4][i] = tmp4;
          //   B[block_j_start + 5][i] = tmp5;
          //   B[block_j_start + 6][i] = tmp6;
          //   B[block_j_start + 7][i] = tmp7;
        }
      }
    }

  } else {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
      for (j = 0; j < M; j++) {
        tmp = A[i][j];
        B[j][i] = tmp;
      }
    }
  }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
  int i, j, tmp;

  for (i = 0; i < N; i++) {
    for (j = 0; j < M; j++) {
      tmp = A[i][j];
      B[j][i] = tmp;
    }
  }
}

char trans_desc_loop_unroll[] = "loop unroll";
void trans_loop_unroll(int M, int N, int A[N][M], int B[M][N]) {
  // 循环展开。。
  int tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  for (int i = 0; i < N; i += 8) {
    for (int j = 0; j < M; j++) {
      tmp = A[i][j];
      tmp1 = A[i + 1][j];
      tmp2 = A[i + 2][j];
      tmp3 = A[i + 3][j];
      tmp4 = A[i + 4][j];
      tmp5 = A[i + 5][j];
      tmp6 = A[i + 6][j];
      tmp7 = A[i + 7][j];
      B[j][i] = tmp;
      B[j][i + 1] = tmp1;
      B[j][i + 2] = tmp2;
      B[j][i + 3] = tmp3;
      B[j][i + 4] = tmp4;
      B[j][i + 5] = tmp5;
      B[j][i + 6] = tmp6;
      B[j][i + 7] = tmp7;
    }
  }
}

char trans_desc_blocking[] = "Simple row-wise scan transpose";
void trans_blocking(int M, int N, int A[N][M], int B[M][N]) {
  int tmp, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  for (int block_i_start = 0; block_i_start < N; block_i_start += 8) {
    for (int block_j_start = 0; block_j_start < M; block_j_start += 8) {
      for (int i = block_i_start; i < min(block_i_start + 8, N); i++) {
        tmp = A[i][block_j_start];
        tmp1 = A[i][block_j_start + 1];
        tmp2 = A[i][block_j_start + 2];
        tmp3 = A[i][block_j_start + 3];
        tmp4 = A[i][block_j_start + 4];
        tmp5 = A[i][block_j_start + 5];
        tmp6 = A[i][block_j_start + 6];
        tmp7 = A[i][block_j_start + 7];
        B[block_j_start][i] = tmp;
        B[block_j_start + 1][i] = tmp1;
        B[block_j_start + 2][i] = tmp2;
        B[block_j_start + 3][i] = tmp3;
        B[block_j_start + 4][i] = tmp4;
        B[block_j_start + 5][i] = tmp5;
        B[block_j_start + 6][i] = tmp6;
        B[block_j_start + 7][i] = tmp7;
      }
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
void registerFunctions() {
  /* Register your solution function */
  registerTransFunction(transpose_submit, transpose_submit_desc);
  /* Register any additional transpose functions */
  //   registerTransFunction(trans, trans_desc);

  //   registerTransFunction(trans_loop_unroll, trans_desc_loop_unroll);

  registerTransFunction(trans_blocking, trans_desc_blocking);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
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

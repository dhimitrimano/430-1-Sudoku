// Sudoku puzzle verifier and solver

#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <math.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// compile: gcc -Wall -Wextra -pthread -lm -std=c99 -g sudoku.c -o sudoku
// run: ./sudoku [puzzle-file-name.txt]
// compile and run after clearing line: clear; gcc -Wall -Wextra -pthread -lm -std=c99 -g sudoku.c -o sudoku; ./sudoku [puzzle-file-name.txt]

// a nice struct as required
typedef struct params {
  int row;
  int col;
  int **table;
  int size;
  bool comp; // checks whether the puzzle is complete
  bool val; // checks whether the puzzle is valid
} params;

const int MAX_PTHREAD = 999; // max pthreads, a nice big number
pthread_t *pthreads; // pthread array
pthread_attr_t *attribs; // attribute array
int pthread_num = 0; // which index to put next pthread
bool completeV = false; // checks whether the puzzle is complete
bool solved = false; // to stop the ongoing threads in the solve function

void *rowCheck(void *); // checks in grid[here][const]
void *colCheck(void *); // checks in grid[const][here]
void *sqrCheck(void *); // checks a square in grid, each square having sqrt(psize) side length

void *solver(void *); // solves the puzzle for extra credit

// takes puzzle size and grid[][] representing sudoku puzzle
// and tow booleans to be assigned: complete and valid.
// row-0 and column-0 is ignored for convenience, so a 9x9 puzzle
// has grid[1][1] as the top-left element and grid[9][9] as bottom right
// A puzzle is complete if it can be completed with no 0s in it
// If complete, a puzzle is valid if all rows/columns/boxes have numbers from 1
// to psize For incomplete puzzles, we cannot say anything about validity
void checkPuzzle(int psize, int **grid, bool *complete, bool *valid) {
  // YOUR CODE GOES HERE and in HELPER FUNCTIONS
  pthreads = (pthread_t *)malloc(MAX_PTHREAD * 4 * sizeof(pthread_t));
  attribs = (pthread_attr_t *)malloc(MAX_PTHREAD * 4 * sizeof(pthread_attr_t));
  params *data = (params *)malloc(MAX_PTHREAD * 4 * sizeof(params));
  int proot = (int) sqrt(psize);
  int rowNum = 0;
  int colNum = 1;
  for (int i = 0; i < psize * 3 && i < MAX_PTHREAD; i += 3) {
    if (rowNum >= proot) {
      colNum += 1;
      rowNum = 1;
    } else {
      rowNum += 1;
    }
    pthread_attr_init(&attribs[i]);
    data[i].col = ((colNum - 1) * proot) + rowNum;
    data[i].row = 0;
    data[i].table = grid;
    data[i].size = psize;
    data[i].comp = true;
    data[i].val = true;
    pthread_create(&pthreads[i], &attribs[i], rowCheck, &data[i]);
    pthread_attr_init(&attribs[i + 1]);
    data[i + 1].col = 0;
    data[i + 1].row = ((colNum - 1) * proot) + rowNum;
    data[i + 1].table = grid;
    data[i + 1].size = psize;
    data[i + 1].comp = true;
    data[i + 1].val = true;
    pthread_create(&pthreads[i + 1], &attribs[i + 1], colCheck, &data[i + 1]);
    pthread_attr_init(&attribs[i + 2]);
    data[i + 2].col = (colNum - 1) * proot;
    data[i + 2].row = (rowNum - 1) * proot;
    data[i + 2].table = grid;
    data[i + 2].size = psize;
    data[i + 2].comp = true;
    data[i + 2].val = true;
    pthread_create(&pthreads[i + 2], &attribs[i + 2], sqrCheck, &data[i + 2]);
    pthread_num += 3;
  }
  for (int i = 0; i < pthread_num; i += 1) {
    pthread_join(pthreads[i], NULL);
    if (!*complete && !*valid) {
      for (i += 1; i < pthread_num; i += 1) {
        pthread_cancel(pthreads[i]);
      }
      break;
    }
    if (!data[i].comp) {
      *complete = false;
    }
    if (!data[i].val) {
      *valid = false;
    }
  }
  pthread_num = 0;
  if (*valid && !*complete) {
    colNum = 1;
    rowNum = 1;
    while (grid[rowNum][colNum] != 0) {
      if (rowNum > psize) {
        colNum += 1;
        rowNum = 1;
      } else {
        rowNum += 1;
      }
    }
    // for (rowNum = 1; rowNum <= psize && grid[rowNum][colNum] != 0; rowNum += 1) {
    //   for (colNum = 1; colNum <= psize && grid[rowNum][colNum] != 0; colNum += 1) {
    //     if (grid[rowNum][colNum] == 0) {
    //       break;
    //     }
    //   }
    //   if (grid[rowNum][colNum] == 0) {
    //     break;
    //   }
    //   fprintf(stderr,"\n");
    // }
    pthread_attr_init(&attribs[0]);
    data[0].col = colNum;
    data[0].row = rowNum;
    data[0].table = grid;
    data[0].size = psize;
    data[0].comp = true;
    data[0].val = false;
    pthread_create(&pthreads[0], &attribs[0], solver, &data[0]);
    pthread_join(pthreads[0], NULL);
    if (data[0].val) {
      *valid = true;
    }
  }
  free(data);
  free(pthreads);
  free(attribs);
}

// takes filename and pointer to grid[][]
// returns size of Sudoku puzzle and fills grid
int readSudokuPuzzle(char *filename, int ***grid) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("Could not open file %s\n", filename);
    exit(EXIT_FAILURE);
  }
  int psize;
  fscanf(fp, "%d", &psize);
  int **agrid = (int **)malloc((psize + 1) * 2 * sizeof(int *));
  for (int row = 1; row <= psize; row++) {
    agrid[row] = (int *)malloc((psize + 1) * 2 * sizeof(int));
    for (int col = 1; col <= psize; col++) {
      fscanf(fp, "%d", &agrid[row][col]);
    }
  }
  fclose(fp);
  *grid = agrid;
  return psize;
}

// takes puzzle size and grid[][]
// prints the puzzle
void printSudokuPuzzle(int psize, int **grid) {
  printf("%d\n", psize);
  for (int row = 1; row <= psize; row++) {
    for (int col = 1; col <= psize; col++) {
      printf("%d ", grid[row][col]);
    }
    printf("\n");
  }
  printf("\n");
}

// takes puzzle size and grid[][]
// frees the memory allocated
void deleteSudokuPuzzle(int psize, int **grid) {
  for (int row = 1; row <= psize; row++) {
    free(grid[row]);
  }
  free(grid);
}

// expects file name of the puzzle as argument in command line
int main(int argc, char **argv) {
  if (argc != 2) {
    printf("usage: ./sudoku puzzle.txt\n");
    return EXIT_FAILURE;
  }
  // grid is a 2D array
  int **grid = NULL;
  // find grid size and fill grid
  int sudokuSize = readSudokuPuzzle(argv[1], &grid);
  bool valid = true;
  bool complete = true;
  checkPuzzle(sudokuSize, grid, &complete, &valid);
  printf("Complete puzzle? ");
  printf(complete ? "true\n" : "false\n");
  if (complete) {
    printf("Valid puzzle? ");
    printf(valid ? "true\n" : "false\n");
  } else {
    printf("Solvable puzzle? ");
    printf(solved ? "true\n" : "false\n");
  }
  printSudokuPuzzle(sudokuSize, grid);
  deleteSudokuPuzzle(sudokuSize, grid);
  return EXIT_SUCCESS;
}

// checks the rows of the grid
void *rowCheck(void *arg) {
  params *p = (params *)arg;
  bool ret = true;
  bool nums[p->size + 1]; // stores whether numbers have appeared in column
  for (int i = 0; i < p->size + 1; i++) {
    nums[i] = false;
  }
  for (int i = 1; i <= p->size; i += 1) {
    if (p->table[i][p->col] == 0) {
      p->comp = false;
    } else if (nums[p->table[i][p->col]] == false) {
      nums[p->table[i][p->col]] = true;
    } else {
      p->val = false;
    }
    if (!p->val && !p->comp) {
      ret = false;
      break;
    }
  }
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
  return (void *)ret;
  #pragma GCC diagnostic pop
}

// checks the columns of the grid
void *colCheck(void *arg) {
  params *p = (params *)arg;
  bool ret = true;
  bool nums[p->size + 1]; // stores whether numbers have appeared in column
  for (int i = 0; i < p->size + 1; i++) {
    nums[i] = false;
  }
  for (int i = 1; i <= p->size; i += 1) {
    if (p->table[p->row][i] == 0) {
      p->comp = false;
    } else if (nums[p->table[p->row][i]] == false) {
      nums[p->table[p->row][i]] = true;
    } else {
      p->val = false;
    }
    if (!p->val && !p->comp) {
      ret = false;
      break;
    }
  }
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
  return (void *)ret;
  #pragma GCC diagnostic pop
}

// checks the inner squares of the grid
void *sqrCheck(void *arg) {
  params *p = (params *)arg;
  bool ret = true;
  int proot = (int) sqrt(p->size);
  int rowN = 0;
  int colN = 1;
  bool nums[p->size + 1]; // stores whether numbers have appeared in column
  for (int i = 0; i < p->size + 1; i++) {
    nums[i] = false;
  }
  for (int i = 1; i <= p->size; i += 1) {
     if (rowN >= proot) {
      colN += 1;
      rowN = 1;
    } else {
      rowN += 1;
    }
    if (p->table[rowN + p->row][colN + p->col] == 0) {
      p->comp = false;
    } else if (nums[p->table[rowN + p->row][colN + p->col]] == false) {
      nums[p->table[rowN + p->row][colN + p->col]] = true;
    } else {
      p->val = false;
    }
    if (!p->val && !p->comp) {
      ret = false;
      break;
    }
  }
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
  return (void *)ret;
  #pragma GCC diagnostic pop
}

// solves the grid using recursion, how else
void *solver(void *arg) {
  params *p = (params *)arg;
  bool ret = false;
  int proot = (int) sqrt(p->size);
  pthread_t *outerPthreads = (pthread_t *)malloc(6 * sizeof(pthread_t));
  pthread_attr_t *outerAttribs = (pthread_attr_t *)malloc(12 * sizeof(pthread_attr_t));
  params *outerData = (params *)malloc(6 * sizeof(params));
  pthread_attr_init(&outerAttribs[0]);
  outerData[0].col = p->col;
  outerData[0].row = p->row;
  outerData[0].table = p->table;
  outerData[0].size = p->size;
  outerData[0].comp = true;
  outerData[0].val = true;
  pthread_create(&outerPthreads[0], &outerAttribs[0], rowCheck, &outerData[0]);
  pthread_attr_init(&outerAttribs[1]);
  outerData[1].col = p->col;
  outerData[1].row = p->row;
  outerData[1].table = p->table;
  outerData[1].size = p->size;
  outerData[1].comp = true;
  outerData[1].val = true;
  pthread_create(&outerPthreads[1], &outerAttribs[1], colCheck, &outerData[1]);
  pthread_attr_init(&outerAttribs[2]);
  outerData[2].col = ((p->col - 1) / proot) * proot;
  outerData[2].row = ((p->row - 1) / proot) * proot;
  outerData[2].table = p->table;
  outerData[2].size = p->size;
  outerData[2].comp = true;
  outerData[2].val = true;
  pthread_create(&outerPthreads[2], &outerAttribs[2], sqrCheck, &outerData[2]);
  pthread_join(outerPthreads[2], NULL);
  pthread_join(outerPthreads[0], NULL);
  pthread_join(outerPthreads[1], NULL);
  printf("%d %d %d look at that, all fours. terrific. it doesn't make sense, but what does?\n", p->row, p->col, p->table[p->row][p->col]);
  if (outerData[0].val && outerData[1].val && outerData[2].val) {
    p->val = true;
    int rowNum = 1;
    int colNum = 1;
    int pastRow;
    int pastCol;
    pastRow = p->size - 1;
    pastCol = p->size - 1;
    pthread_t *innerPthreads = (pthread_t *)malloc(p->size * 4 * sizeof(pthread_t));
    pthread_attr_t *innerAttribs = (pthread_attr_t *)malloc(p->size * 4 * sizeof(pthread_attr_t));
    params *data = (params *)malloc(p->size * 4 * sizeof(params));
    if (!solved) {
      while (p->table[rowNum][colNum] != 0) {
        if (colNum > p->size) {
          rowNum = 0;
          colNum = 1;
        }
        if (rowNum > p->size) {
          colNum += 1;
          rowNum = 1;
        } else {
          rowNum += 1;
        }
        if (rowNum == pastRow && colNum == pastCol) {
          p->val = true;
          solved = true;
          ret = true;
          free(data);
          free(innerPthreads);
          free(innerAttribs);
          #pragma GCC diagnostic push
          #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
          return (void *)ret;
          #pragma GCC diagnostic pop
        }
      }
      if (p->table[rowNum][colNum] == 0) {
        int num[p->size];
        for (int i = 0; i < p->size; i++) {
          num[i] = i + 1;
        }
        for (int i = 0; i < p->size; i += 1) {
          pthread_attr_init(&innerAttribs[i]);
          data[i].col = colNum;
          data[i].row = rowNum;
          data[i].size = p->size;
          data[i].comp = true;
          data[i].val = false;
          data[i].table = p->table;
          data[i].table[rowNum][colNum] = num[i];
          pthread_create(&innerPthreads[i], &innerAttribs[i], solver, &data[i]);
        }
        for (int i = 0; i < p->size; i += 1) {
          pthread_join(innerPthreads[i], NULL);
          if (data[i].val) {
            for (i += 1; i < p->size; i++) {
              pthread_cancel(innerPthreads[i]);
            }
            p->val = true;
            solved = true;
            break;
          } else if (solved) {
            for (i += 1; i < p->size; i++) {
              pthread_cancel(innerPthreads[i]);
            }
            free(data);
            free(innerPthreads);
            free(innerAttribs);
            break;
          }
        }
      }
    }
  }
  free(outerData);
  free(outerPthreads);
  free(outerAttribs);
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
  return (void *)ret;
  #pragma GCC diagnostic pop
}
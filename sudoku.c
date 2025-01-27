// Sudoku puzzle verifier

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

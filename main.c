/*
Name: Ethan Ciavolella
Id: 916328107
Homework: #2
To Compile: Run "make" in this directory on a unix machine
To Run: ./GameOfLife <Board size> <Number of generations> <Number of threads> (Grid width) (Grid height)
        Grid width and grid height are optional parameters. If one is specified, both must be specified.
        Specifying grid width and height activates 2d distribution mode
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/time.h>
#include <omp.h>
#include <math.h>

//Uncomment this macro to replace the randomized board with a glider
//Use this with a small board size (10-20) to evaluate correctness
//#define DEBUG

//Testing to see if forcing gcc to inline the functions significantly affected performance.
//It did not.
//#define MAYBE_INLINE // Uncomment the macro to disable inlining
#ifndef MAYBE_INLINE
#define MAYBE_INLINE __attribute__((always_inline)) inline
#endif

typedef uint8_t cell_t;
typedef cell_t **board_t;

int board_size;
int num_generations;
int threads;

// Optional arguments for 2D distribution mode
int grid_width;
int grid_height;

// Set to 1 if data is being distributed in 2D
uint8_t grid_mode = 0;

// Return current time in seconds
double gettime() {
        struct timeval tval;
        gettimeofday(&tval, NULL);
        return ((double)tval.tv_sec + (double)tval.tv_usec/1000000.0);
}

// Update the border cells, which are not part of the simulation
// and are only there to make the board appear to wrap around
MAYBE_INLINE void clone_edges(board_t board) {
        #pragma omp for
        for (int j = 1; j < board_size + 1; j++ ) {
                board[j][0] = board[j][board_size];
                board[j][board_size + 1] = board[j][1];
        }
        #pragma omp for
        for (int i = 0; i < board_size + 2; i++) {
                board[0][i] = board[board_size][i];
                board[board_size + 1][i] = board[1][i];
        }
}

MAYBE_INLINE uint8_t update_cell(
        board_t current,
        board_t next,
        int x,
        int y) 
{
        uint8_t n_neighbors = current[y-1][x-1];
        uint8_t has_updated = 0;

        n_neighbors += current[y-1][x];
        n_neighbors += current[y-1][x+1];

        n_neighbors += current[y][x-1];
        n_neighbors += current[y][x+1];

        n_neighbors += current[y+1][x-1];
        n_neighbors += current[y+1][x];
        n_neighbors += current[y+1][x+1];

        if (current[y][x]) {
                if (n_neighbors < 2) { // Living cell killed by underpopulation

                        next[y][x] = 0;
                        has_updated = 1;
                } else if (n_neighbors > 3) { // Living cell killed by overpopulation
                        next[y][x] = 0;
                        has_updated = 1;
                } else {
                        next[y][x] = 1;
                }
        } else {
                if (n_neighbors == 3) { // Dead cell is resurrected
                        next[y][x] = 1;
                        has_updated = 1;
                } else {
                        next[y][x] = 0;
                }
        }
        return has_updated;
} 
               
// Update next's state by applying Game of Life rules to current
// Returns 1 if the board was updated, and 0 if not. This is used for early
// exit if the board state becomes finalized
MAYBE_INLINE uint8_t update(board_t current, board_t next) {
    int has_updated = 0;

    #pragma omp for
    for (int i = 1; i < board_size + 1; i++) {
        for (int j = 1; j < board_size + 1; j++) {
                if (update_cell(current, next, j, i)) {
                        has_updated = 1;
                }
        }
    }
    if (has_updated) {
        clone_edges(next);
    }
    return has_updated;
}

MAYBE_INLINE uint8_t grid_update(board_t current, board_t next) {

        int grid_row_length = (int)ceil((double)board_size / (double)grid_width);
        int grid_col_length = (int)ceil((double)board_size / (double)grid_height);

        uint8_t has_updated = 0;
        
        #pragma omp for
        for (int i = 0; i < grid_row_length * grid_col_length; i++) {
              int grid_row = i / grid_row_length, grid_col = i % grid_row_length;

              for (int j = 0; j < grid_height; j++) {
                int y = grid_row*grid_height + j + 1;
                if (y > board_size) break;

                for (int k = 0; k < grid_width; k++) {
                        int x = grid_col*grid_width + k + 1;
                        if (x > board_size) break;

                        has_updated |= update_cell(current, next, x, y);
                }
              }
        }
        if (has_updated) {
                clone_edges(next);
        }
        return has_updated;
}

void print_board(board_t board) {
    for (int i = 1; i < board_size + 1; i++) {
        for (int j = 1; j < board_size + 1; j++) {
            printf("%d ", board[i][j]);
        }
        printf("\n");
    }
    printf("\n");
}

int main(int argc, char **argv) {

    if ( (argc != 4) && (argc != 6) ) {
        printf("Usage: GameOfLife <Size> <Generations> <Number of threads> (Grid width) (Grid height)\n");
        return 1;
    }

    board_size = atoi(argv[1]);
    if (board_size < 2) {
        printf("Board size must be greater than 1\n");
        return 2;
    }

    num_generations = atoi(argv[2]);
    if (num_generations < 1) {
        printf("Number of generations must be at least 1\n");
        return 3;
    }

    threads = atoi(argv[3]);
    if (threads < 1) {
        printf("Number of threads must be at least 1\n");
        return 4;
    }

    if (argc > 4) {
        grid_width = atoi(argv[4]);
        if (grid_width < 1) {
                printf("Grid width must be at least 1\n");
                return 5;
        }
        grid_height = atoi(argv[5]);
        if (grid_height < 1) {
                printf("Grid height must be at least 1\n");
                return 6;
        }
        grid_mode = 1;        
    }

    board_t board1 = malloc((board_size + 2) * sizeof(cell_t*));
    board_t board2 = malloc((board_size + 2) * sizeof(cell_t*));

    for (int i = 0; i < board_size + 2; i++) {
        board1[i] = malloc((board_size + 2) * sizeof(cell_t));
        board2[i] = malloc((board_size + 2) * sizeof(cell_t));
    }

    // Set up board
    srand(1337);

    for (int i = 1; i < board_size + 1; i++) {
        for (int j = 1; j < board_size + 1; j++) {
#ifdef DEBUG
            board1[i][j] = 0;
#else
            board1[i][j] = rand() % 2;
#endif
        }
    }

#ifdef DEBUG
    board1[1][2] = 1;
    board1[2][3] = 1;
    board1[3][1] = 1;
    board1[3][2] = 1;
    board1[3][3] = 1;

    printf("Before:\n");
    print_board(board1);
#endif
    double before = gettime();
    int generations_completed = 0;
    uint8_t updates = 0;

    printf("Starting simulation for a board of size %dx%d for %d generations using %d threads.\n",
        board_size, board_size, num_generations, threads);
    if (grid_mode) {
        printf("Using a two-dimensional distribution with grids of size %dx%d.\n",
                grid_width, grid_height);
    }

    #pragma omp parallel num_threads(threads)
    while (generations_completed < num_generations) {
       
    if (grid_mode) {
        updates |= grid_update(board1, board2);
    } else {
        updates |= update(board1, board2);
    }
        
    #pragma omp barrier

    #pragma omp single
    {
        if (!updates) {
                generations_completed = num_generations;
        }

        board_t temp = board1;
        board1 = board2;
        board2 = temp;

        generations_completed++;
        updates = 0;
    }

    }

    double after = gettime();

    printf("Completed %d generations. Runtime in seconds: %lf\n", generations_completed, after - before);

#ifdef DEBUG
    printf("After:\n");
    print_board(board1);
#endif
    for (int i = 0; i < board_size + 2; i++) {
        free(board1[i]);
        free(board2[i]);
    }
    free(board1);
    free(board2);
    return 0;
}

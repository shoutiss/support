// Implements Game of Fifteen (generalized to d x d)

#define _XOPEN_SOURCE 500

#include <cs50.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// Constants
#define DIM_MIN 3
#define DIM_MAX 9
#define CHILDREN 4
#define SOLVED 0
#define ERROR 82
#define COLOR "\033[32m"
#define MAX_INT 65535

// Board
int** board;

// Dimensions
int d;

// Coordinates of tile struct
typedef struct
{
    int row;
    int col;
}
location;

// Node struct for our graph
typedef struct node
{
    // 2d array for board-state
    int** grid;
    // Current f_score: number of moves + distance
    int f_score;
    // Tile moved to get to this state
    int tile_moved;
    // Parent node
    struct node *parent;
}
node;

// Struct to hold moves for solver
struct
{
    int index;
    int tiles[200];
}
solver;

// Pointer to root of tree for searching
node *start;

// Blank tile location
location blank;

// Prototypes
void clear(void);
void greet(void);
void init(void);
void draw(void);
bool move(int tile, int** grid);
bool won(int** grid);
location search(int num, int** grid);
void swap(int *a, int *b);
void shuffle(int array[], int n);
bool solvable(int array[], int n);
int manhattan(int** grid);
int* find_moves(int** grid);
void copy_board(int** source, int** destination);

// Here we go - testing this out...
bool ida(void);
void free_grid(int** grid);
int tree_search(node* current, int g_score, int threshhold);
void push(int move);
int pop(void);

int main(int argc, string argv[])
{
    // Ensure proper usage
    if (argc != 2)
    {
        printf("Usage: fifteen d\n");
        return 1;
    }

    // Ensure valid dimensions
    d = atoi(argv[1]);
    if (d < DIM_MIN || d > DIM_MAX)
    {
        printf("Board must be between %i x %i and %i x %i, inclusive.\n",
            DIM_MIN, DIM_MIN, DIM_MAX, DIM_MAX);
        return 2;
    }

    // Allocate memory for global board
    board = malloc(sizeof(int*) * d);
    for (int i = 0; i < d; i++)
    {
        board[i] = malloc(sizeof(int) * d);
    }

    // Open log
    FILE *file = fopen("log.txt", "w");
    if (file == NULL)
    {
        return 3;
    }

    // Greet user with instructions
    greet();

    // Initialize the board
    init();

    // Accept moves until game is won
    while (true)
    {
        // Clear the screen
        // clear();

        // Draw the current state of the board
        draw();

        // Log the current state of the board (for testing)
        for (int i = 0; i < d; i++)
        {
            for (int j = 0; j < d; j++)
            {
                fprintf(file, "%i", board[i][j]);
                if (j < d - 1)
                {
                    fprintf(file, "|");
                }
            }
            fprintf(file, "\n");
        }
        fflush(file);

        // Check for win
        if (won(board))
        {
            printf("ftw!\n");
            break;
        }

        // Prompt for move
        printf("Tile to move: ");
        int tile = get_int();

        // Quit if user inputs 0 (for testing)
        if (tile == 0)
        {
            break;
        }

        // Log move (for testing)
        fprintf(file, "%i\n", tile);
        fflush(file);

        // Move if possible, else report illegality
        if (!move(tile, board))
        {
            printf("\nIllegal move.\n");
            usleep(500000);
        }

        // Sleep thread for animation's sake
        usleep(50000);
    }

    // Close log
    fclose(file);

    // Free board
    for (int i = 0; i < 3; i++)
    {
        free(board[i]);
    }
    free(board);

    // Success
    return 0;
}

// Clears screen using ANSI escape sequences
void clear(void)
{
    printf("\033[2J");
    printf("\033[%d;%dH", 0, 0);
}

// Greets player
void greet(void)
{
    clear();
    printf("WELCOME TO GAME OF FIFTEEN\n");
    usleep(2000000);
}

// Initializes the game's board with tiles numbered 1 through d*d - 1
// (i.e., fills 2D array with values but does not actually print them)
void init(void)
{

    // Create array of 0 to n - 1
    int size = d * d;
    int array[size];
    for (int i = 0; i < size; i++)
    {
        array[i] = i;
    }

    // Shuffle array until solvable
    do
    {
        shuffle(array, size);
    }
    while (!solvable(array, size));

    // Assign array to the 2d board
    int counter = 0;
    for (int row = 0; row < d; row++)
    {
        for (int col = 0; col < d; col++)
        {
            board[row][col] = array[counter];
            counter++;
        }
    }

    // Store blank location
    blank = search(0, board);
    // DEBUG - ASSIGN ALMOST SOLVED BOARD
    // board[0][0] = 1;
    // board[0][1] = 2;
    // board[0][2] = 3;
    // board[1][0] = 4;
    // board[1][1] = 5;
    // board[1][2] = 6;
    // board[2][0] = 7;
    // board[2][1] = 0;
    // board[2][2] = 8;

    // blank.row = 2;
    // blank.col = 1;
    if (ida())
    {
        while (solver.index > 0)
        {
            move(pop(), board);
            draw();
        }
    }
    return;
}

// Prints the board in its current state
void draw(void)
{
    // Iterate through board
    for (int row = 0; row < d; row++)
    {
        for (int col = 0; col < d; col++)
        {
            // Print non-zero values, blank for zero
            if (board[row][col] != 0)
            {
                printf(" %2i ", board[row][col]);
            }
            else
            {
                printf(" __ ");
            }
        }
        printf("\n");
    }
}

// If tile borders empty space, moves tile and returns true, else returns false
bool move(int tile, int** grid)
{
    // Find tile and check that it is on board
    location spot = search(tile, grid);
    location zero = search(0, grid);
    if (spot.row == ERROR)
    {
        return false;
    }

    // Calculate distances
    int row_dist = abs(spot.row - zero.row);
    int col_dist = abs(spot.col - zero.col);

    // Move if valid, update blank location
    if (row_dist + col_dist == 1)
    {
        swap(&grid[spot.row][spot.col], &grid[zero.row][zero.col]);
        blank = spot;
        return true;

    }
    return false;
}

// Returns true if game is won (i.e., board is in winning configuration), else false
bool won(int** grid)
{
    // Initialize check variable
    int check_num = 1;
    int max = d * d;

    // Iterate through board
    for (int row = 0; row < d; row++)
    {
        for (int col = 0; col < d; col++)
        {
            // Increment check if current value matches board location
            if (check_num % max == grid[row][col])
            {
                check_num++;
            }
        }
    }
    // If every board location matched, success
    if (check_num == max + 1)
    {
        return true;
    }
    return false;
}

// Determine the row and column of a given tile - returns ERROR for these values if not in the array
location search(int num, int** grid)
{
    // Create location variable and set default values
    location result;
    result.row = ERROR;
    result.col = ERROR;

    // Iterate through board
    for (int i = 0; i < d; i++)
    {
        for (int j = 0; j < d; j++)
        {
            // If tile found, assign row and col numbers
            if (grid[i][j] == num)
            {
                result.row = i;
                result.col = j;
            }
        }
    }
    return result;
}

// Swap the values of two variables
void swap(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Function to randomly shuffle an array
void shuffle(int array[], int n)
{
    // Seed random generator
    srand48((long int) time(NULL));

    // Iterate through list and swap a each index with a random index
    for (int i = 0; i < n; i++)
    {
        int rand_index = drand48() * n;
        swap(&array[i], &array[rand_index]);
    }
    return;
}

// Function to test if an array produces a solvable game of 15
bool solvable(int array[], int n)
{
    // Count inversions and determine blank row
    int inversions = 0;
    int zero_row;
    for (int i = 0; i < n; i++)
    {
        // Count Inversions
        for(int j = i; j < n; j++)
        {
            if (array[i] > array[j] && array[i] != 0 && array[j] != 0)
            {
                inversions++;
            }
        }

        // Determine zero row
        if (array[i] == 0)
        {
            zero_row = d - 1 - i / d;
        }

    }

    // Odd dimensions, inversions must be even
    if (d % 2 == 1 && inversions % 2 == 0)
    {
        return true;
    }
    // Even dimensiions, sum of inversions and zero row distance must be even
    else if (((zero_row + inversions) % 2 == 0) && d % 2 == 0)
    {
        return true;
    }
    // Neither condition met, unsolvable!
    else
    {
        return false;
    }

}

// Calculate manhattan distance of grid from a solved state
int manhattan(int** grid)
{
    int distance = 0;
    int counter = 1;
    for (int i = 0; i < d; i++)
    {
        for (int j = 0; j < d; j++)
        {
            // Find tile and calculate distance from desired location
            if (counter != 0)
            {
                location tile = search(counter, grid);
                distance += abs(tile.row - i) + abs(tile.col - j);
            }

            // Increment tile counter - mod dimension to ensure bound to actual tiles are found
            counter = (counter + 1) % (d * d);
        }
    }
    return distance;
}

// Determines the possible tiles to move for a board
int* find_moves(int** grid)
{
    // Store blank location
    location zero = search(0, grid);

    // Dynamic array to store possible moves
    int* moves = malloc(sizeof(int) * 4);

    int move_index = 0;
    // Iterate through and test to see if a tile can move
    for (int i = 1, n = d * d; i < n; i++)
    {
        // Find each tile and find distance to blank tile
        location tile = search(i, grid);
        int distance = abs(zero.row - tile.row) + abs(zero.col - tile.col);

        // If valid move, store in array of moves
        if (distance == 1)
        {
            moves[move_index] = i;
            move_index++;
        }
    }

    return moves;
}

// Copy a board into a new location
void copy_board(int** source, int** destination)
{
    for (int i = 0; i < d; i++)
    {
        for (int j = 0; j < d; j++)
        {
            destination[i][j] = source[i][j];
        }
    }
}

// Main IDA* search function beginning
bool ida(void)
{
    // Allocate memory for root node
    start = malloc(sizeof(node));
    if (start == NULL)
    {
        return false;
    }

    // Allocate memory for root node grid
    start->grid = malloc(sizeof(int*) * d);
    for (int i = 0; i < d; i++)
    {
        start->grid[i] = malloc(sizeof(int) * d);
    }

    // Copy board to node and initilize values
    copy_board(board, start->grid);
    start->f_score = manhattan(start->grid);
    start->tile_moved = -1;
    start->parent = NULL;

    // Start recursive serach looping
    int threshhold = start->f_score;
    while (true)
    {
        //printf("HERE 1\n");
        int temp = tree_search(start, 0, threshhold);
        if (temp == SOLVED)
        {
            printf("HOLY SMOKES IT WORKS!\n");
            return true;
        }
        threshhold = temp;
    }

    free_grid(start->grid);
    free(start);
    return true;
}

// Free the memory allocated for a grid
void free_grid(int** grid)
{
    for (int i = 0; i < 3; i++)
    {
        free(grid[i]);
    }
    free(grid);
}

// Recursive search of nodes looking for solved grid
int tree_search(node* current, int g_score, int threshhold)
{
    // Set current node f_score
    current->f_score = g_score + manhattan(current->grid);
    // printf("Current threshhold: %i \n", threshhold);

    // Check for threshhold
    if (current->f_score > threshhold)
    {
        // if (current->parent != NULL)
        // {
        //     int score = current->f_score;
        //     free_grid(current->grid);
        //     free(current);
        //     return score;
        // }
        return current->f_score;

    }

    // Check for solved state
    if (won(current->grid))
    {
        return SOLVED;
    }

    int min = MAX_INT;
    int* moves = malloc(sizeof(int) * CHILDREN);
    moves = find_moves(current->grid);

    for (int i = 0; i < CHILDREN; i++)
    {
        if (moves[i] != 0)
        {
            //printf("HERE 2\n");
            node *newnode = malloc(sizeof(node));
            newnode->parent = current;
            newnode->tile_moved = moves[i];

            newnode->grid = malloc(sizeof(int*) * d);
            for (int j = 0; j < d; j++)
            {
                newnode->grid[j] = malloc(sizeof(int) * d);
            }

            copy_board(current->grid, newnode->grid);
            move(moves[i], newnode->grid);

            // printf("NEW BOARD:\n");
            // for (int k = 0; k < 3; k++)
            // {
            //     for (int j = 0; j < 3; j++)
            //     {
            //         printf("%2i   ", newnode->grid[k][j]);
            //     }
            //     printf("\n");
            // }

            int temp = tree_search(newnode, g_score + 1, threshhold);
            if (temp == SOLVED)
            {
                // Free memory!
                push(newnode->tile_moved);
                free_grid(newnode->grid);
                free(newnode);

                return SOLVED;
            }

            if (temp < min)
            {
                min = temp;
            }

            // Free memory!
            free_grid(newnode->grid);

            free(newnode);
        }

    }
    free(moves);
    return min;
}



// Function to push an element onto a stack
void push(int tile)
{
    solver.tiles[solver.index] = tile;
    solver.index++;
}


// Function to pop an element off of the stack
int pop(void)
{
    solver.index--;
    return solver.tiles[solver.index];
}

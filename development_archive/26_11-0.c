#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  // For sleep()

typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    position_t center;
    position_t size;
    char **art;
} object_t;

typedef struct {
    int size;
    char **board;
    object_t frog;
    // Other objects (cars, trees, etc.) can be added here later
} board_t;

void initializeBoard(board_t *b) {
    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            // Top-left corner
            if ((i == 0 && j == 0)|| (i == 0 && j == b->size - 1) ||  (i == b->size - 1 && j == 0) || (i == b->size - 1 && j == b->size - 1)) {
                b->board[i][j] = '+';
            }
            else if (i == 0 || i == b->size - 1) {
                b->board[i][j] = '-';
            }
            else if (j == 0 || j == b->size - 1) {
                b->board[i][j] = '|';
            }
            // Inner space
            else {
                b->board[i][j] = ' ';  // Empty space inside the borders
            }
        }
    }
}

// Function to create the frog object
void createFrog(object_t *frog) {
    frog->center.x = 5;
    frog->center.y = 5;
    frog->size.x = 3;   // Width of the frog (3 characters wide)
    frog->size.y = 3;   // Height of the frog (3 characters tall)
    
    // Allocate memory for the frog's ASCII art (3x3 grid)
    frog->art = malloc(frog->size.y * sizeof(char *));
    for (int i = 0; i < frog->size.y; i++) {
        frog->art[i] = malloc(frog->size.x * sizeof(char));
    }

    // Define the frog's ASCII art
    frog->art[0][0] = 'o'; frog->art[0][1] = '_'; frog->art[0][2] = 'o';
    frog->art[1][0] = '^'; frog->art[1][1] = 'O'; frog->art[1][2] = '^';
    frog->art[2][0] = '^'; frog->art[2][1] = ' '; frog->art[2][2] = '^';
}

// Function to place an object on the board at a specified center position
void placeObject(board_t *b, object_t *obj, position_t target) {
    for (int i = 0; i < obj->size.x; i++) {
        for (int j = 0; j < obj->size.y; j++) {
            // Calculate the position of the top-left corner based on the target center
            int posX = target.x - obj->size.x / 2 + i;
            int posY = target.y - obj->size.y / 2 + j;
            
            // Ensure the coordinates are within the bounds of the board
            if (posX >= 0 && posX < b->size && posY >= 0 && posY < b->size) {
                b->board[posX][posY] = obj->art[i][j];  // Place the object's symbol
            }
        }
    }
}

// Function to draw the board to the terminal
void drawBoard(board_t *b) {
    printf("\033[H\033[J");  // Clear the screen (works in most terminals)

    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            printf("%c", b->board[i][j]);
        }
        printf("\n");
    }
}

// Function to move the frog
void moveFrog(board_t *b, object_t *frog, char direction) {
    // Remove the frog from its current position
    for (int i = 0; i < frog->size.x; i++) {
        for (int j = 0; j < frog->size.y; j++) {
            int posX = frog->center.x - frog->size.x / 2 + i;
            int posY = frog->center.y - frog->size.y / 2 + j;
            if (posX >= 0 && posX < b->size && posY >= 0 && posY < b->size) {
                b->board[posX][posY] = ' ';  // Clear the space
            }
        }
    }

    // Update frog's position based on the direction
    switch (direction) {
        case 'w':  // Move up
            if (frog->center.x > 1) frog->center.x--;
            break;
        case 's':  // Move down
            if (frog->center.x < b->size - 2) frog->center.x++;
            break;
        case 'a':  // Move left
            if (frog->center.y > 1) frog->center.y--;
            break;
        case 'd':  // Move right
            if (frog->center.y < b->size - 2) frog->center.y++;
            break;
        default:
            break;
    }

    // Place the frog at the new position
    placeObject(b, frog, frog->center);
}

int main() {
    const int SIZE = 20;  // Set board size (adjust as needed)

    // Initialize the board
    board_t board;
    board.size = SIZE;
    board.board = malloc(SIZE * sizeof(char*));

    // Allocate memory for each row of the board
    for (int i = 0; i < SIZE; i++) {
        board.board[i] = malloc(SIZE * sizeof(char));
    }

    // Initialize the frog object
    object_t frog;
    createFrog(&frog);

    // Initialize the board with borders
    initializeBoard(&board);

    // Place the frog on the board
    placeObject(&board, &frog, frog.center);

    // Draw the board
    drawBoard(&board);

    // Move the frog in a few directions and update the board
    sleep(1);  // Sleep to see the result for 1 second
    moveFrog(&board, &frog, 'w');  // Move up
    drawBoard(&board);

    sleep(1);  // Sleep to see the result for 1 second
    moveFrog(&board, &frog, 'd');  // Move right
    drawBoard(&board);

    // Clean up memory
    for (int i = 0; i < frog.size.y; i++) {
        free(frog.art[i]);
    }
    free(frog.art);

    for (int i = 0; i < SIZE; i++) {
        free(board.board[i]);
    }
    free(board.board);

    return 0;
}

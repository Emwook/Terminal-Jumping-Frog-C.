#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
} board_t;

void initializeBoard(board_t *b) {
    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == b->size - 1) || (i == b->size - 1 && j == 0) || (i == b->size - 1 && j == b->size - 1)) {
                b->board[i][j] = '+';
            }
            else if (i == 0 || i == b->size - 1) {
                b->board[i][j] = '-';
            }
            else if (j == 0 || j == b->size - 1) {
                b->board[i][j] = '|';
            }
            else {
                b->board[i][j] = ' ';
            }
        }
    }
}

void readArtFromFile(const char *filename, object_t *o) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    if (fscanf(file, "%d %d", &o->size.x, &o->size.y) != 2) {
        perror("Error reading dimensions from file");
        exit(1);
    }

    o->art = malloc(o->size.x * sizeof(char *));
    for (int i = 0; i < o->size.x; i++) {
        o->art[i] = malloc(o->size.y * sizeof(char));
    }

    fgetc(file);

    for (int i = 0; i < o->size.x; i++) {
        for (int j = 0; j < o->size.y; j++) {
            int ch = fgetc(file);

            if (ch == EOF || ch == '\n') {
                o->art[i][j] = ' ';
            } else {
                o->art[i][j] = (char)ch;
            }
        }
        while (1) {
            int ch = fgetc(file);
            if (ch == '\n' || ch == EOF) {
                break;
            }
        }
    }

    fclose(file);
}

void createFrog(object_t *frog) {
    frog->center.x = 5;
    frog->center.y = 5;
    frog->size.x = 3;
    frog->size.y = 3;

    readArtFromFile("input.txt", frog);
}

void placeObject(board_t *b, object_t *obj, position_t target) {
    for (int i = 0; i < obj->size.x; i++) {
        for (int j = 0; j < obj->size.y; j++) {
            int posX = target.x - obj->size.x / 2 + i;
            int posY = target.y - obj->size.y / 2 + j;
            
            if (posX >= 0 && posX < b->size && posY >= 0 && posY < b->size) {
                b->board[posX][posY] = obj->art[i][j];
            }
        }
    }
}

void drawBoard(board_t *b) {
    printf("\033[H\033[J");

    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            printf("%c", b->board[i][j]);
        }
        printf("\n");
    }
}

void moveFrog(board_t *b, object_t *frog, char direction) {
    for (int i = 0; i < frog->size.x; i++) {
        for (int j = 0; j < frog->size.y; j++) {
            int posX = frog->center.x - frog->size.x / 2 + i;
            int posY = frog->center.y - frog->size.y / 2 + j;
            if (posX >= 0 && posX < b->size && posY >= 0 && posY < b->size) {
                b->board[posX][posY] = ' ';
            }
        }
    }

    switch (direction) {
        case 'w':
            if (frog->center.x > 1) frog->center.x--;
            break;
        case 's':
            if (frog->center.x < b->size - 2) frog->center.x++;
            break;
        case 'a':
            if (frog->center.y > 1) frog->center.y--;
            break;
        case 'd':
            if (frog->center.y < b->size - 2) frog->center.y++;
            break;
        default:
            break;
    }

    placeObject(b, frog, frog->center);
}

int main() {
    const int SIZE = 20;

    board_t board;
    board.size = SIZE;
    board.board = malloc(SIZE * sizeof(char*));

    for (int i = 0; i < SIZE; i++) {
        board.board[i] = malloc(SIZE * sizeof(char));
    }

    object_t frog;
    createFrog(&frog);

    initializeBoard(&board);

    placeObject(&board, &frog, frog.center);

    drawBoard(&board);

    sleep(1);  
    moveFrog(&board, &frog, 'w');
    drawBoard(&board);

    sleep(1);  
    moveFrog(&board, &frog, 'd');
    drawBoard(&board);

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

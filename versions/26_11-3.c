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
    char *name;
} object_t;

typedef struct {
    int size;
    char **board;
    char **elements;
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

    char line[256];
    int found = 0;
    
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, o->name) != NULL) {
            found = 1;

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
                if (fgets(line, sizeof(line), file)) {
                    for (int j = 0; j < o->size.y && line[j] != '\0'; j++) {
                        o->art[i][j] = line[j] != '\n' ? line[j] : ' ';
                    }
                }
            }

            break;
        }
    }
    
    if (!found) {
        printf("Object name not found in file!\n");
    }

    fclose(file);
}

void createObject(object_t *o, position_t pos, position_t size) {
    o->center.x = pos.x;
    o->center.y = pos.y;
    o->size.x = size.x;
    o->size.y = size.y;
    readArtFromFile("input.txt", o);
}

void placeObject(board_t *b, object_t *obj, position_t target) {
    const char symbol = obj->name[0];
    for (int i = 0; i < obj->size.x; i++) {
        for (int j = 0; j < obj->size.y; j++) {
            int posX = target.x - obj->size.x / 2 + i;
            int posY = target.y - obj->size.y / 2 + j;

            if (posX >= 0 && posX < b->size && posY >= 0 && posY < b->size) {
                b->board[posX][posY] = obj->art[i][j];
                b->elements[posX][posY] = symbol;
            }
        }
    }
}

void drawBoard(board_t *b) {
    printf("\033[H\033[J");

    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            char color_code[10] = "\033[0m";

            switch (b->elements[i][j]) {
                case 'f': 
                    snprintf(color_code, sizeof(color_code), "\033[92m");
                    break;
                case 'c': 
                    snprintf(color_code, sizeof(color_code), "\033[31m");
                    break;
                case 'r':
                    snprintf(color_code, sizeof(color_code), "\033[31m");
                    break;
                case 'l':
                    snprintf(color_code, sizeof(color_code), "\033[93m");
                    break;
                case 't':
                    snprintf(color_code, sizeof(color_code), "\033[32m");
                    break;
                case ' ':
                    snprintf(color_code, sizeof(color_code), "\033[30m");
                    break;
                default:
                    snprintf(color_code, sizeof(color_code), "\033[31m");
            }

            if (b->board[i][j] != ' ') {
                printf("%s%c" "\033[0m", color_code, b->board[i][j]);
            } else {
                printf("%c", b->board[i][j]);
            }
        }
        printf("\n");
    }
}

void moveFrog(board_t *b, object_t *frog, char direction, position_t step) {
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
            if (frog->center.x > 1) frog->center.x-=step.x;
            break;
        case 's':
            if (frog->center.x < b->size - 2) frog->center.x+=step.x;
            break;
        case 'a':
            if (frog->center.y > 1) frog->center.y-=step.y;
            break;
        case 'd':
            if (frog->center.y < b->size - 2) frog->center.y+=step.y;
            break;
        default:
            break;
    }

    placeObject(b, frog, frog->center);
}

int main() {
    const int SIZE = 30;
    const int STEP = 5;

    board_t board;
    board.size = SIZE;
    board.board = malloc(SIZE * sizeof(char*));
    board.elements = malloc(SIZE * sizeof(char*));

    for (int i = 0; i < SIZE; i++) {
        board.board[i] = malloc(SIZE * sizeof(char));
        board.elements[i] = malloc(SIZE * sizeof(char));
        for (int j = 0; j < SIZE; j++) {
            board.elements[i][j] = ' ';
        }
    }

    object_t frog;
    frog.name = "frog";
    position_t frogStartPos = {SIZE - 10, SIZE / 2};
    position_t defaultFrogSize = {STEP, STEP};
    createObject(&frog, frogStartPos, defaultFrogSize);

    object_t lane;
    lane.name = "lane";
    position_t laneStartPos = {2*STEP, SIZE/2};
    position_t defaultLaneSize = {STEP, STEP};
    createObject(&lane, laneStartPos, defaultLaneSize);

    object_t car;
    car.name = "car";
    position_t carStartPos = {2*STEP+2, SIZE/3};
    position_t defaultCarSize = {STEP, STEP};
    createObject(&car, carStartPos, defaultCarSize);

    initializeBoard(&board);
    
    placeObject(&board, &frog, frog.center);
    placeObject(&board, &lane, lane.center);
    placeObject(&board, &car, car.center);

    drawBoard(&board);

    int repeat = 5;

    while (repeat > 0){
        sleep(1);  
        placeObject(&board, &lane, lane.center);
        placeObject(&board, &car, car.center);
        moveFrog(&board, &frog, 'w', frog.size);
        drawBoard(&board);
        repeat--;
    }

    for (int i = 0; i < frog.size.y; i++) {
        free(frog.art[i]);
    }
    free(frog.art);

    for (int i = 0; i < SIZE; i++) {
        free(board.board[i]);
        free(board.elements[i]);
    }
    free(board.board);
    free(board.elements);

    return 0;
}

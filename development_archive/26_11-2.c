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
    object_t frog;
    object_t* carLanes;
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
        // Check if the current line matches the object's name
        if (strstr(line, o->name) != NULL) {
            found = 1;

            // Read the dimensions
            if (fscanf(file, "%d %d", &o->size.x, &o->size.y) != 2) {
                perror("Error reading dimensions from file");
                exit(1);
            }

            // Allocate memory for the art based on its size
            o->art = malloc(o->size.x * sizeof(char *));
            for (int i = 0; i < o->size.x; i++) {
                o->art[i] = malloc(o->size.y * sizeof(char));
            }

            // Skip the newline after reading dimensions
            fgetc(file);  // Consume the newline character

            // Read the art data
            for (int i = 0; i < o->size.x; i++) {
                if (fgets(line, sizeof(line), file)) {
                    // Read each line and handle the art properly
                    for (int j = 0; j < o->size.y && line[j] != '\0'; j++) {
                        o->art[i][j] = line[j] != '\n' ? line[j] : ' ';  // Replace newline with space
                    }
                }
            }

            break;  // Found the correct object, so stop looking
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
    readArtFromFile("input.txt", o);  // Pass the object to the function instead of just the name
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

    for (int i = 0; i < SIZE; i++) {
        board.board[i] = malloc(SIZE * sizeof(char));
    }
object_t frog;
    frog.name = "frog";
    position_t frogStartPos = {SIZE - 10, SIZE / 2}; // need to change it be like a max frog size or sth
    position_t defaultFrogSize = {STEP, STEP}; //change it to something more sophisticated
    createObject(&frog, frogStartPos, defaultFrogSize);

    object_t lane;
    lane.name = "lane";
   
    position_t laneStartPos = {2*STEP, SIZE-STEP+1};

    position_t defaultLaneSize = {STEP, STEP};
    createObject(&lane, laneStartPos, defaultLaneSize);


    initializeBoard(&board);

    drawBoard(&board);
    
    placeObject(&board, &frog, frog.center);
    placeObject(&board, &lane, lane.center);

    sleep(1);  
    moveFrog(&board, &frog, 'w', frog.size);
    drawBoard(&board);

    sleep(1);  
    moveFrog(&board, &frog, 's', frog.size);
    drawBoard(&board);

    sleep(1);  
    moveFrog(&board, &frog, 'd', frog.size);
    drawBoard(&board);

    sleep(1);  
    moveFrog(&board, &frog, 'a', frog.size);
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

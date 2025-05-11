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
    char **visual_layer;
    char **gameplay_layer;
    object_t frog;
} board_t;

void initializeVisualLayer(board_t *b) {
    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == b->size - 1) || (i == b->size - 1 && j == 0) || (i == b->size - 1 && j == b->size - 1)) {
                b->visual_layer[i][j] = '+';
            }
            else if ((i == 0 && (j<(b->size/2 -3) || j>(b->size/2 + 3)))|| i == b->size - 1) { // change to being based on frog size and not '5'
                b->visual_layer[i][j] = '-';
            }
            else if (j == 0 || j == b->size - 1) {
                b->visual_layer[i][j] = '|';
            }
            else {
                b->visual_layer[i][j] = ' ';
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

void placeObject(board_t *b, object_t *obj) {
    if (!b || !obj || !obj->name || !obj->art) {
        fprintf(stderr, "Invalid input to placeObject\n");
        return;
    }

    const char symbol = obj->name[0]; // The symbol used to represent the object
    // Check if the object is meant to span the full width
    int is_full_length = (strcmp(obj->name, "lane") == 0) ? 1 : 0;

    // Calculate the size
    const int sizeY = is_full_length ? b->size : obj->size.y; // Use board size for full-length objects
    const int sizeX = obj->size.x; // Use board size for full-length objects
    // const int sizeY = obj->size.y;

    for (int i = 0; i < sizeX; i++) {
        for (int j = 0; j < sizeY; j++) {
            // Adjust position to center the object around its center point
            int posX = obj->center.x - (obj->size.x / 2) + i;
            int posY = obj->center.y - (obj->size.y / 2) + j;

            // Wrap horizontally if the object spans the full width (is_full_length is true)
            int artY = j;  // For non-full-length objects, just use i
            if (is_full_length) {
                artY = j % obj->size.y;  // Wrap horizontally based on object width
                posY = ( (obj->size.y)/ 2) + j -3;
            }
            int artX = i;  // No vertical wrapping, just use `j` for artY

            // Ensure the position is within bounds of the board
            if (posX >= 1 && posX < b->size && posY >= 1 && posY < b->size) {
                // Place the object on the visual layer
                b->visual_layer[posX][posY] = obj->art[artX][artY];

                // Set the gameplay layer symbol (to represent the object)
                b->gameplay_layer[posX][posY] = symbol;
            }
        }
    }
}


void drawVisualLayer(board_t *b) {
    printf("\033[H\033[J");

    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            char color_code[10] = "\033[0m";

            switch (b->gameplay_layer[i][j]) {
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

            if (b->visual_layer[i][j] != ' ') {
                printf("%s%c" "\033[0m", color_code, b->visual_layer[i][j]);
            } else {
                printf("%c", b->visual_layer[i][j]);
            }
        }
        printf("\n");
    }
}

void drawGameplayLayer(board_t *b) {
    printf("\033[H\033[J"); // Clear the console

    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            // Print the character from the gameplay_layer
            printf("%c", b->gameplay_layer[i][j]);
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
                b->visual_layer[posX][posY] = ' ';
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

    placeObject(b, frog);
}
void createMultipleLanes(board_t *b, object_t *lane, int max_lanes) {
    // Seed random generator (if not already seeded elsewhere)
    srand(time(NULL));

    int lanes_created = 0;
    int position_x = b->size; // Start at the far edge of the board

    while (lanes_created < max_lanes) {
        // Random gap between 1 and 2 lane widths
        int gap = lane->size.x + rand() % (lane->size.x + 1);

        // Calculate next lane's starting position
        position_x -= (lane->size.x + gap);

        // Check if the lane fits on the board
        if (position_x < 0) {
            break; // Stop placing lanes if there's no room left
        }

        // Create and place the lane
        position_t laneStartPos = {position_x, 0};
        createObject(lane, laneStartPos, lane->size);
        placeObject(b, lane);

        lanes_created++;
    }
}



int main() {
    const int SIZE = 40;
    const int STEP = 5;
    const int MAX_LANES = 4;

    board_t board;
    board.size = SIZE;
    board.visual_layer = malloc(SIZE * sizeof(char*));
    board.gameplay_layer = malloc(SIZE * sizeof(char*));

    for (int i = 0; i < SIZE; i++) {
        board.visual_layer[i] = malloc(SIZE * sizeof(char));
        board.gameplay_layer[i] = malloc(SIZE * sizeof(char));
        for (int j = 0; j < SIZE; j++) {
            board.gameplay_layer[i][j] = ' ';
        }
    }

    object_t frog;
    frog.name = "frog";
    position_t frogStartPos = {SIZE - 5, SIZE / 2};
    position_t defaultFrogSize = {STEP, STEP};
    createObject(&frog, frogStartPos, defaultFrogSize);

    object_t lane;
    lane.name = "lane";
    position_t laneStartPos = {2*STEP, SIZE/2};
    position_t defaultLaneSize = {STEP, 2*STEP};
    createObject(&lane, laneStartPos, defaultLaneSize);

    object_t car;
    car.name = "car";
    position_t carStartPos = {2*STEP+2, SIZE/3};
    position_t defaultCarSize = {STEP, STEP};
    createObject(&car, carStartPos, defaultCarSize);

    initializeVisualLayer(&board);
    
    placeObject(&board, &frog);
    createMultipleLanes(&board, &lane, MAX_LANES);
    placeObject(&board, &car);

    drawVisualLayer(&board);

    int repeat = 4;

    while (repeat > 0){
        sleep(1);  
        // placeObject(&board, &lane);
        createMultipleLanes(&board, &lane, MAX_LANES);
        placeObject(&board, &car);
        moveFrog(&board, &frog, 'w', frog.size);
        drawVisualLayer(&board);
        // drawGameplayLayer(&board);
        repeat--;
    }

    for (int i = 0; i < frog.size.y; i++) {
        free(frog.art[i]);
    }
    free(frog.art);

    for (int i = 0; i < SIZE; i++) {
        free(board.visual_layer[i]);
        free(board.gameplay_layer[i]);
    }
    free(board.visual_layer);
    free(board.gameplay_layer);

    return 0;
}

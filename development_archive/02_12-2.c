#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>

//structs
typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    position_t size;
    char **art;
} model_t;

typedef struct {
    char *name;
    position_t center;
    model_t model;
} object_t;

typedef struct {
    int size;
    char **visual_layer;
    char **gameplay_layer;
    object_t *models;
    object_t frog;
} board_t;

typedef struct {
    int score;
    int current_turn;
    int number_of_turns;
    char* difficulty;
    char* name;
    int university_id;
}  scoreboard_t;



// functions split into different categories
void readArtFromFile(const char *filename, object_t *o); // reads art from file and saves it as an art attribute in a specific object
void setObjectSizeAndCenter(object_t *o, position_t pos, position_t size); // sets position and size of a specific object 

void placeObject(board_t *b, object_t *obj); // places an object on both game and visual layers, repeats the object if necessery

void initializeVisualLayer(board_t *b); // draws the board without any objects, just the border
void drawVisualLayer(board_t *b); // takes the visual layer components and with proper coloring prints it to terminal 

void drawGameplayLayer(board_t *b); // takes the game layer components and prints it to terminal

void moveObject(board_t *b, object_t *o, char direction); // shifts the object in a given direction if it would fit afterwards
void createMultipleLanes(board_t *b, object_t *lane, int max_lanes); // checks if the given amount of lanes would fit on the board, and creates as many as possible if not, and less if given less than maximum

void readGameSettings(const char *filename, scoreboard_t *s);

void allocateArtMemory(object_t *obj);
void freeArtMemory(object_t *obj);

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

            if (fscanf(file, "%d %d", &o->model.size.x, &o->model.size.y) != 2) {
                perror("Error reading dimensions from file");
                exit(1);
            }

            o->model.art = malloc(o->model.size.x * sizeof(char *));
            for (int i = 0; i < o->model.size.x; i++) {
                o->model.art[i] = malloc(o->model.size.y * sizeof(char));
            }

            fgetc(file);

            for (int i = 0; i < o->model.size.x; i++) {
                if (fgets(line, sizeof(line), file)) {
                    for (int j = 0; j < o->model.size.y && line[j] != '\0'; j++) {
                        o->model.art[i][j] = line[j] != '\n' ? line[j] : ' ';
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

void setObjectSizeAndCenter(object_t *o, position_t pos, position_t size) {
    o->center.x = pos.x;
    o->center.y = pos.y;
    o->model.size.x = size.x;
    o->model.size.y = size.y;
}

void placeObject(board_t *b, object_t *obj) {
    if (!b || !obj || !obj->name || !obj->model.art) {
        fprintf(stderr, "Invalid input to placeObject\n");
        return;
    }

    const char symbol = obj->name[0];

    int is_full_length = (strcmp(obj->name, "lane") == 0) ? 1 : 0;

    const int sizeY = is_full_length ? b->size : obj->model.size.y;
    const int sizeX = obj->model.size.x;

    for (int i = 0; i < sizeX; i++) {
        for (int j = 0; j < sizeY; j++) {

            int posX = obj->center.x - (obj->model.size.x / 2) + i;
            int posY = obj->center.y - (obj->model.size.y / 2) + j;

            int artY = j;
            if (is_full_length) {
                artY = j % obj->model.size.y;
                posY = ( (obj->model.size.y)/ 2) + j -3;
            }
            int artX = i;

            if (posX >= 1 && posX < b->size && posY >= 1 && posY < b->size) {
                b->visual_layer[posX][posY] = obj->model.art[artX][artY];

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
    printf("\033[H\033[J");

    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            printf("%c", b->gameplay_layer[i][j]);
        }
        printf("\n");
    }
}


void moveObject(board_t *b, object_t *o, char direction) {
    position_t stepBy = {4,4};
    for (int i = 0; i < o->model.size.x; i++) {
        for (int j = 0; j < o->model.size.y; j++) {
            int posX = o->center.x - o->model.size.x / 2 + i;
            int posY = o->center.y - o->model.size.y / 2 + j;
            if (posX >= 0 && posX < b->size && posY >= 0 && posY < b->size) {
                b->visual_layer[posX][posY] = ' ';
                b->gameplay_layer[posX][posY] = ' ';
            }
        }
    }

    switch (direction) {
    case 'w':
        if (o->center.x - o->model.size.x / 2 > 2) 
            o->center.x -= stepBy.x;
        break;
    case 's':
        if (o->center.x + o->model.size.x / 2 < b->size-3) 
            o->center.x += stepBy.x;
        break;
    case 'a':
        if (o->center.y - o->model.size.y / 2 > 4) 
            o->center.y -= stepBy.y;
        break;
    case 'd':
        if (o->center.y + o->model.size.y / 2 < b->size - 3) 
            o->center.y += stepBy.y;
        break;
    default:
        break;
    }

    placeObject(b, o);
}

void createMultipleLanes(board_t *b, object_t *lane, int max_lanes) {
    int lanes_amount = (b->size - 2) / (lane->model.size.x * 1.5); 
    lanes_amount = lanes_amount > max_lanes ? max_lanes : lanes_amount;

    for (int i = 0; i < lanes_amount; i++) {
        position_t laneStartPos = {(i + 1) * (1.4*lane->model.size.x), b->size / 2};
        setObjectSizeAndCenter(lane, laneStartPos, lane->model.size);
        placeObject(b, lane);
    }
}

void readGameSettings(const char *filename, scoreboard_t *s) {
    if (!filename || !s) {
        fprintf(stderr, "Invalid filename or scoreboard pointer\n");
        return;
    }

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    setlocale(LC_ALL, "");

    if (fscanf(file, "%d\n", &(s->number_of_turns)) != 1) {
        fprintf(stderr, "Failed to read number_of_turns\n");
        fclose(file);
        return;
    }

    char buffer[256];
    if (fscanf(file, "%255s\n", buffer) != 1) {
        fprintf(stderr, "Failed to read difficulty\n");
        fclose(file);
        return;
    }
    s->difficulty = strdup(buffer);

    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fprintf(stderr, "Failed to read name\n");
        free(s->difficulty);
        fclose(file);
        return;
    }
    buffer[strcspn(buffer, "\n")] = '\0';
    s->name = strdup(buffer);

    if (fscanf(file, "%d", &(s->university_id)) != 1) {
        fprintf(stderr, "Failed to read university_id\n");
        free(s->difficulty);
        free(s->name);
        fclose(file);
        return;
    }

    fclose(file);
}

void printScoreboard(const scoreboard_t *s) {
    if (!s) {
        fprintf(stderr, "Invalid scoreboard pointer\n");
        return;
    }

    printf("===== Scoreboard =====\n");
    printf("score: %d \t", s->score);
    printf("turn: %d/%d\n", s->current_turn, s->number_of_turns);
    printf("difficulty: %s\n", s->difficulty ? s->difficulty : "N/A");
    printf("name: %s\n", s->name ? s->name : "N/A");
    printf("university ID: %d\n", s->university_id);
    printf("======================\n");
}


void allocateArtMemory(object_t *obj) {
    obj->model.art = malloc(obj->model.size.x * sizeof(char *));
    if (!obj->model.art) {
        perror("Error allocating art memory");
        exit(1);
    }
    for (int i = 0; i < obj->model.size.x; i++) {
        obj->model.art[i] = malloc(obj->model.size.y * sizeof(char));
        if (!obj->model.art[i]) {
            perror("Error allocating art row memory");
            for (int j = 0; j < i; j++) free(obj->model.art[j]);
            free(obj->model.art);
            exit(1);
        }
    }
}

void freeArtMemory(object_t *obj) {
    if (!obj->model.art) return;
    for (int i = 0; i < obj->model.size.x; i++) {
        free(obj->model.art[i]);
    }
    free(obj->model.art);
    obj->model.art = NULL;
}

// void initializeArt(object_t *obj, char symbol) {
    // readArtFromFile("default_input.txt", obj);
    // for (int i = 0; i < obj->model.size.x; i++) {
    //     for (int j = 0; j < obj->model.size.y; j++) {
    //         obj->model.art[i][j] = symbol;
    //     }
    // }
// }



int main() {
    // Data that needs to be later taken in from a file
    const int SIZE = 40;
    const int STEP = 4;
    const int MAX_LANES = 4;
    const char defaultArt[] = "default_input.txt";

    scoreboard_t scoreboard;
    scoreboard.score = 0;
    scoreboard.current_turn = 1;

    // Define the game board and allocate memory
    board_t board;
    board.size = SIZE;
    board.visual_layer = malloc(SIZE * sizeof(char *));
    board.gameplay_layer = malloc(SIZE * sizeof(char *));
    if (!board.visual_layer || !board.gameplay_layer) {
        perror("Error allocating board layers");
        exit(1);
    }

    for (int i = 0; i < SIZE; i++) {
        board.visual_layer[i] = malloc(SIZE * sizeof(char));
        board.gameplay_layer[i] = malloc(SIZE * sizeof(char));
        if (!board.visual_layer[i] || !board.gameplay_layer[i]) {
            perror("Error allocating rows in board layers");
            exit(1);
        }
        for (int j = 0; j < SIZE; j++) {
            board.gameplay_layer[i][j] = ' ';
        }
    }

    // Initialize objects
    object_t frog, lane, car;

    frog.name = "frog";
    position_t frogStartPos = {SIZE - (STEP / 2) * 2, SIZE / 2};
    position_t defaultFrogSize = {STEP, STEP};
    setObjectSizeAndCenter(&frog, frogStartPos, defaultFrogSize);
    allocateArtMemory(&frog);
    readArtFromFile(defaultArt, &frog);

    lane.name = "lane";
    position_t laneStartPos = {2 * STEP, SIZE / 2};
    position_t defaultLaneSize = {STEP, 2 * STEP};
    setObjectSizeAndCenter(&lane, laneStartPos, defaultLaneSize);
    allocateArtMemory(&lane);
    readArtFromFile(defaultArt, &lane);

    car.name = "car_right";
    position_t carStartPos = {2 * STEP + 2, SIZE / 3};
    position_t defaultCarSize = {STEP, STEP};
    setObjectSizeAndCenter(&car, carStartPos, defaultCarSize);
    allocateArtMemory(&car);
    readArtFromFile(defaultArt, &car);

    // Allocate memory for model.art for each object


    readGameSettings("game_setup.txt", &scoreboard);
    printScoreboard(&scoreboard);

    // Uncommented critical game logic
    initializeVisualLayer(&board);
    placeObject(&board, &frog);
    createMultipleLanes(&board, &lane, MAX_LANES);
    placeObject(&board, &car);
    drawVisualLayer(&board);

    // Game loop (testing for 5 iterations)
    int repeat = 5;
    while (repeat > 0) {
        sleep(1);
        createMultipleLanes(&board, &lane, MAX_LANES);
        placeObject(&board, &car);
        moveObject(&board, &frog, 'w');
        moveObject(&board, &car, 'd');
        drawVisualLayer(&board);
        // drawGameplayLayer(&board);
        printScoreboard(&scoreboard);
        repeat--;
    }

    // Free allocated memory
    freeArtMemory(&frog);
    freeArtMemory(&lane);
    freeArtMemory(&car);

    for (int i = 0; i < SIZE; i++) {
        free(board.visual_layer[i]);
        free(board.gameplay_layer[i]);
    }
    free(board.visual_layer);
    free(board.gameplay_layer);

    return 0;
}

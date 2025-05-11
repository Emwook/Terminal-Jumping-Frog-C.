#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> // nie mozna
#include <time.h>
#include <locale.h>
#include <math.h>


typedef struct {
    int x;
    int y;
} position_t;

typedef struct {
    char *name;
    position_t size;
    char **art;
} model_t;

typedef struct {
    char intent;
    char *name;
    position_t center;
    model_t model;
} object_t;

typedef struct {
    object_t object;
    char direction;
    int speed;
} moving_object_t;

typedef struct {
    moving_object_t *objects;
    int amount;
} moving_cars_set_t;

typedef struct {
    int *car_lanes;
    int car_lane_amount;
    object_t *car_lanes_array;
} car_lanes_set_t;

typedef struct {
    int size;
    char **visual_layer;
    char **gameplay_layer;
    car_lanes_set_t car_lanes;
    moving_cars_set_t cars;
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

void initializeMovingCars(moving_cars_set_t *cars, int amount);
void createBoardBorder(board_t *b);
void allocateArtMemory(model_t *model);
void freeArtMemory(model_t *model);
void readArtFromFile(const char *filename, model_t *model);
void readGameSettings(const char *filename, scoreboard_t *s);
void placeObject(board_t *b, object_t *obj);
void createMultipleLanes(board_t *b, model_t *lane, int max_lanes);
void drawVisualLayer(board_t *b);
void createCarsOnLanes(board_t *b, model_t *car_m, int cars_amount, char direction);
void printScoreboard(const scoreboard_t *s);
// void refreshObjects(board_t *b, object_t *o);

//testing functions
void printCarLanes(const board_t *b);
void drawGameplayLayer(board_t *b);
void printCarsPositions(const board_t *b);

void initializeMovingCars(moving_cars_set_t *cars, int amount) {
    if (!cars) {
        return;
    }

    cars->amount = amount;
    cars->objects = (moving_object_t *)malloc(amount * sizeof(moving_object_t));

    if (!cars->objects) {
       perror("Error allocating cars object memory");
        exit(1);
    }

    for (int i = 0; i < amount; i++) {
        cars->objects[i].object.intent = 'e';
        cars->objects[i].object.name = "car_right";
        cars->objects[i].object.center.x = 0;
        cars->objects[i].object.center.y = 0;
        cars->objects[i].direction = 'd';
        cars->objects[i].speed = 1;
    }
}

void createBoardBorder(board_t *b) {
    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == b->size - 1) || (i == b->size - 1 && j == 0) || (i == b->size - 1 && j == b->size - 1)) {
                b->visual_layer[i][j] = '+';
                b->gameplay_layer[i][j] = 'b'; // as in 'border'
            }
            else if ((i == 0 && (j<(b->size/2 -3) || j>(b->size/2 + 3)))|| i == b->size - 1) { // change to being based on frog size and not '5'
                b->visual_layer[i][j] = '-';
                b->gameplay_layer[i][j] = 'b';
            }
            else if ((i == 0 && (j>(b->size/2 -3) || j<(b->size/2 + 3)))){
                b->visual_layer[i][j] = ' ';
                b->gameplay_layer[i][j] = 'w'; // as in 'win'
            }
            else if (j == 0 || j == b->size - 1) {
                b->visual_layer[i][j] = '|';
                b->gameplay_layer[i][j] = 'b';
            }
            else {
                b->visual_layer[i][j] = ' ';
                b->gameplay_layer[i][j] = ' ';
            }
        }
    }
}

void allocateRowMemory(char ***art, int numRows, int numCols) {
    *art = malloc(numRows * sizeof(char *));
    if (*art == NULL) {
        perror("Error allocating art memory");
        exit(1);
    }

    for (int i = 0; i < numRows; i++) {
        (*art)[i] = malloc(numCols * sizeof(char));
        if ((*art)[i] == NULL) {
            perror("Error allocating art row memory");

            for (int j = 0; j < i; j++) free((*art)[j]);
            free(*art);
            exit(1);
        }
    }
}

void allocateArtMemory(model_t *model) {
    allocateRowMemory(&model->art, model->size.x, model->size.y);
}

void freeArtMemory(model_t *model) {
    if (model->art == NULL) return;
    for (int i = 0; i < model->size.x; i++) {
        free(model->art[i]);
    }
    free(model->art);
    model->art = NULL;
}

void readArtFromFile(const char *filename, model_t *model) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    char line[256];
    int found = 0;
    
    // Search for the model name in the file
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, model->name) != NULL) {
            found = 1;

            // Read the dimensions of the art
            if (fscanf(file, "%d %d", &model->size.x, &model->size.y) != 2) {
                perror("Error reading dimensions from file");
                fclose(file);
                exit(1);
            }

            // Allocate memory for the art
            allocateArtMemory(model);

            // Consume the newline character after the dimensions
            fgetc(file);

            // Read the art content line by line
            for (int i = 0; i < model->size.x; i++) {
                if (fgets(line, sizeof(line), file)) {
                    for (int j = 0; j < model->size.y && line[j] != '\0'; j++) {
                        model->art[i][j] = (line[j] != '\n') ? line[j] : ' ';
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

    // Set locale for Polish characters
    setlocale(LC_ALL, "");

    // Read the number_of_turns
    if (fscanf(file, "%d\n", &(s->number_of_turns)) != 1) {
        fprintf(stderr, "Failed to read number_of_turns\n");
        fclose(file);
        return;
    }

    // Read the difficulty as a single word
    char buffer[256];
    if (fscanf(file, "%255s\n", buffer) != 1) {
        fprintf(stderr, "Failed to read difficulty\n");
        fclose(file);
        return;
    }
    s->difficulty = strdup(buffer); // Allocate memory and copy the string

    // Read the name as a full line (multi-word with Polish characters)
    if (fgets(buffer, sizeof(buffer), file) == NULL) {
        fprintf(stderr, "Failed to read name\n");
        free(s->difficulty);
        fclose(file);
        return;
    }
    // Remove trailing newline character from fgets
    buffer[strcspn(buffer, "\n")] = '\0';
    s->name = strdup(buffer); // Allocate memory and copy the string

    // Read the university_id
    if (fscanf(file, "%d", &(s->university_id)) != 1) {
        fprintf(stderr, "Failed to read university_id\n");
        free(s->difficulty);
        free(s->name);
        fclose(file);
        return;
    }

    fclose(file);
}

void placeObject(board_t *b, object_t *obj) {

    const char symbol = (obj->name[0])?(obj->name[0]):(' ');
    int is_full_length = (strcmp(obj->name, "lane") == 0) ? 1 : 0;
    const int sizeY = is_full_length ? b->size : obj->model.size.y;
    const int sizeX = obj->model.size.x;
    position_t size = {sizeX,sizeY};

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

            if (posX > 0 && posX < b->size -1 && posY > 0 && posY < b->size -1) {
                b->visual_layer[posX][posY] = obj->model.art[artX][artY];
                b->gameplay_layer[posX][posY] = symbol;
            }
        }
    }
}

void createMultipleLanes(board_t *b, model_t *lane_m, int max_lanes) {
    int lanes_amount = (b->size - 2) / (lane_m->size.x * 1.5); 
    lanes_amount = lanes_amount > max_lanes ? max_lanes : lanes_amount;

    for (int i = 0; i < lanes_amount; i++) {
        position_t lane_start_pos = {(i + 1) * (1.4 * lane_m->size.x), b->size / 2};

        int alreadyPresent = 0;
        for (int j = 0; j < b->car_lanes.car_lane_amount; j++) {
            if (b->car_lanes.car_lanes[j] == (int)(lane_start_pos.x)) {
                alreadyPresent = 1;
                break;
            }
        }

        if (!alreadyPresent) {
            b->car_lanes.car_lanes[b->car_lanes.car_lane_amount] = (int)(lane_start_pos.x);
            b->car_lanes.car_lane_amount++;

            object_t lane;
            lane.name = "lane";
            lane.intent = 'n';
            lane.center = lane_start_pos;
            lane.model = *(lane_m);
            // b->car_lanes.car_lanes_array[b->car_lanes.car_lane_amount] = lane;
            placeObject(b, &lane);
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

void createCarsOnLanes(board_t *b, model_t *car_m, int cars_amount, char direction) {
    int max_cars_per_lane = (b->size - 2) / (car_m->size.y * 1.5);
    max_cars_per_lane = max_cars_per_lane > cars_amount ? cars_amount : max_cars_per_lane;

    int cars_to_place = cars_amount;
    int laneIndex = 0;
    int direction_sign = (direction == 'd') ? 1 : -1;

    while (cars_to_place > 0) {
        int cars_in_lane = (cars_amount - cars_to_place) / (b->car_lanes.car_lane_amount);
        if (cars_in_lane < max_cars_per_lane) {

            int base_y = ((cars_in_lane) * ((b->size / car_m->size.y) * (car_m->size.y / 2))) +
                         (rand() % (car_m->size.y / 2)) - (car_m->size.y / 4);
            
            if (direction_sign == -1) {
                base_y = b->size - base_y;
            }

            position_t car_start_pos = {
                fmax(
                    b->car_lanes.car_lanes[laneIndex] + (direction_sign) * (car_m->size.x) - (direction_sign),
                    0
                ),
                base_y
            };

            int found = 0;
            for (int i = 0; i < b->cars.amount; i++) {
                if (b->cars.objects[i].object.center.x == 0 && b->cars.objects[i].object.center.y == 0) {
                    object_t *car = &(b->cars.objects[i].object);
                    car->model = *car_m;
                    car->name = car_m->name;
                    car->center = car_start_pos;
                    placeObject(b, car);
                    found = 1;
                    break;
                }
            }

            if (!found) {
                break;
            }

            cars_to_place--;
        }
        laneIndex = (laneIndex + 1) % (b->car_lanes.car_lane_amount);
    }
}

void printScoreboard(const scoreboard_t *s) {
    if (!s) {
        fprintf(stderr, "Invalid scoreboard pointer\n");
        return;
    }
    printf("\n");
    printf("score: %d \n", s->score);
    printf("turn: %d/%d\n", s->current_turn, s->number_of_turns);
    printf("difficulty: %s\n", s->difficulty ? s->difficulty : "N/A");
    printf("author: %s\n", s->name ? s->name : "N/A");
    printf("university ID: %d\n", s->university_id);
}

void moveObject(board_t *b, object_t *o, char direction) {

    position_t stepBy = {1,1};

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


    int type_of_move;
    char axis;
    
    switch (direction) {
        case 'w':
                axis = 'x';
                type_of_move = -stepBy.x;
            break;
        case 's':
                axis = 'x';
                type_of_move = stepBy.x;
            break;
        case 'a':
                axis = 'y';
                type_of_move = -stepBy.y;
            break;
        case 'd':
                axis = 'y';
                type_of_move = stepBy.x;
            break;
        default:
            break;
    }

    if(o->intent == 'e' && (o->center.x + type_of_move >=0)){
        if(axis == 'x'){
            o->center.x = (o->center.x + type_of_move);
        }
        else if(axis == 'y'){
            o->center.y = (o->center.y + type_of_move);
        }
        else {
            return;
        }
        placeObject(b, o);
    }
    else {
        if(axis == 'x'){
            if ((o->center.x - (o->model.size.x / 2) > 2) || (o->center.x + o->model.size.x / 2 < b->size-3)){
                o->center.x = (o->center.x + type_of_move);
            } 
        }
        if(axis == 'y'){
            if ((o->center.y - o->model.size.y / 2 > 4)  || (o->center.y + o->model.size.y / 2 < b->size - 3)){
                o->center.x = (o->center.x + type_of_move);
            } 
        }
        placeObject(b, o);
    }
}

void refreshBoard(board_t *b){
    // if(b->car_lanes.car_lane_amount){
    //     for(int i=0; i<(b->car_lanes.car_lane_amount); i++){
    //         placeObject(b, &(b->car_lanes.car_lanes_array[i]));
    //     }
    // }
    if(b->cars.amount){
        for(int i=0; i<(b->cars.amount); i++){
            placeObject(b, &(b->cars.objects[i].object));
        }
    }
    if(b->frog.center.x){
        placeObject(b, &(b->frog));
    }
};


//testing only function
void printCarLanes(const board_t *b) {
    if (!b || !b->car_lanes.car_lane_amount) {
        printf("No car lanes to display.\n");
        return;
    }

    printf("Car Lanes (Total: %d):\n", b->car_lanes.car_lane_amount);
    for (int i = 0; i < b->car_lanes.car_lane_amount; i++) {
        printf("Lane %d: %d\n", i + 1, b->car_lanes.car_lanes[i]);
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

void printCarsPositions(const board_t *b) {
    if (!b) {
        printf("No cars to display.\n");
        return;
    }

    printf("Cars (total: %d) :\n", b->cars.amount);
    for (int i = 0; i < b->cars.amount; i++) {
        printf("Car %d: (%d, %d) %s \n", i + 1, b->cars.objects[i].object.center.x, b->cars.objects[i].object.center.y, b->cars.objects[i].object.name);
    }
}

void printBoardSpecs(board_t *board) {
    printf("Board Specifications:\n");
    printf("Size: %d\n", board->size);

    printf("\nFrog:\n");
    printf("  Name: %s\n", board->frog.name);
    printf("  Intent: %c\n", board->frog.intent);
    printf("  Center: { x: %d, y: %d }\n",
           board->frog.center.x,
           board->frog.center.y);
    printf("  Model Name: %s\n", board->frog.model.name);
    printf("  Model Size: { x: %d, y: %d }\n",
           board->frog.model.size.x,
           board->frog.model.size.y);
    if (board->frog.model.art) {
        printf("  Frog Art:\n");
        for (int i = 0; board->frog.model.art[i] != NULL; i++) {
            printf("    %s\n", board->frog.model.art[i]);
        }
    }
}


int main() {
    const int SIZE = 50;
    const int STEP = 1;
    position_t MAX_MODEL_SIZE = {15,15};
    const int MAX_LANES = 4;
    const int AMOUNT_OF_CARS = 4;
    const char defaultArt[] = "default_input.txt";
    const char defaultSettings[] = "game_setup.txt";

    scoreboard_t scoreboard;
    scoreboard.score = 0;
    scoreboard.current_turn = 1;

    board_t board;
    board.size = SIZE;
    board.car_lanes.car_lane_amount = 0;
    board.cars.amount = 0;

    board.car_lanes.car_lanes = calloc(MAX_LANES, sizeof(int)); // freed later
    board.visual_layer = malloc(SIZE * sizeof(char *)); // freed later
    board.gameplay_layer = malloc(SIZE * sizeof(char *)); // freed later
    if (!board.visual_layer || !board.gameplay_layer) {
        perror("Error allocating board layers");
        exit(1);
    }

    for (int i = 0; i < SIZE; i++) {
        board.visual_layer[i] = malloc(SIZE * sizeof(char)); // freed later
        board.gameplay_layer[i] = malloc(SIZE * sizeof(char)); // freed later
        if (!board.visual_layer[i] || !board.gameplay_layer[i]) {
            perror("Error allocating rows in board layers");
            exit(1);
        }
    }

    model_t lane_model, car_right_model, car_left_model, frog_model;
    
    lane_model.size = MAX_MODEL_SIZE;
    lane_model.name = "lane";
    allocateArtMemory(&lane_model);
    readArtFromFile(defaultArt, &lane_model);
    
    car_right_model.size = MAX_MODEL_SIZE;
    car_right_model.name = "car_right";
    allocateArtMemory(&car_right_model);
    readArtFromFile(defaultArt, &car_right_model);

    car_left_model.size = MAX_MODEL_SIZE;
    car_left_model.name = "car_left";
    allocateArtMemory(&car_left_model);
    readArtFromFile(defaultArt, &car_left_model);

    frog_model.size = MAX_MODEL_SIZE;
    frog_model.name = "frog";
    allocateArtMemory(&frog_model);
    readArtFromFile(defaultArt, &frog_model);

    board.frog.model = frog_model;
    board.frog.name = board.frog.model.name;
    board.frog.intent = 'f';
    board.frog.center = (position_t){board.size - board.frog.model.size.x, board.size/2};

    readGameSettings(defaultSettings, &scoreboard);

    initializeMovingCars(&board.cars, AMOUNT_OF_CARS);
    createBoardBorder(&board);
    createMultipleLanes(&board, &lane_model, MAX_LANES);
    createCarsOnLanes(&board, &car_right_model, AMOUNT_OF_CARS/2, 'd');
    createCarsOnLanes(&board, &car_left_model, AMOUNT_OF_CARS/2, 'a');

    placeObject(&board, &(board.frog));

    drawVisualLayer(&board);
    // drawGameplayLayer(&board);
    printScoreboard(&scoreboard);
    // printCarsPositions(&board);
    // printBoardSpecs(&board);


    int repeat = 10;
    while (repeat > 0) {
        sleep(1);
        moveObject(&board, &(board.frog), 'w');
        refreshBoard(&board);
        drawVisualLayer(&board);
        // drawGameplayLayer(&board);
        printScoreboard(&scoreboard);
        printCarLanes(&board);
        repeat--;
    }

    free(board.car_lanes.car_lanes);

    for (int i = 0; i < SIZE; i++) {
        free(board.visual_layer[i]);
        free(board.gameplay_layer[i]);
    }
    free(board.visual_layer);
    free(board.gameplay_layer);

    freeArtMemory(&lane_model);
    freeArtMemory(&car_right_model);
    freeArtMemory(&car_left_model);
    freeArtMemory(&frog_model);


    return 0;
}
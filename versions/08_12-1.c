#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <math.h>
#include <unistd.h>
#include <curses.h>

//directions
#define UP 'w'
#define DOWN 's'
#define LEFT 'a'
#define RIGHT 'd'

//map objects
#define BORDER 'b'
#define FINISH_LINE 'w'

//types of intent
#define ENEMY 'e'
#define FRIENDLY 'f'
#define NEUTRAL 'n'

//car speeds
#define MAX_SPEED 9 
#define MIN_SPEED 5

//general game settings
#define MAX_GAME_TIME 25000     // miliseconds
#define MAX_MODEL_SIZE 15       // characters
#define STEP_SIZE 1             // character
#define TIME_STEP 80            // miliseconds
#define MAX_DIFFICULTY 5        // cooldown of 5 frames
#define MIN_DIFFICULTY 1        // minimum frog cooldown of 1 frame
#define READY 0                 // meaning frog is ready to move again

//car specific constants
#define NOT_TIMED_OUT -1        // generic way to show that car is free to move
#define OUT_OF_BOARD -20        // number that describes a location to hold cars out of the visible border when timed out
#define PROXIMITY_THRESHOLD 1   // what 'close to' means - 1 character distance

//map defining constants
#define MAX_TREES 20
#define SIZE 50
#define MAX_LANES 4
#define AMOUNT_OF_CARS 6

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
} object_t;

typedef struct {
    object_t object;
    char direction;
    int speed;
    int timeout_until;
    int lane_number;
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
    model_t frog;
    model_t car_left;
    model_t car_right;
    model_t lane;
    model_t tree;
} models_set_t;

typedef struct {
    int size;
    char **visual_layer;
    char **gameplay_layer;
    car_lanes_set_t car_lanes;
    moving_cars_set_t cars;
    object_t frog;
    object_t *trees;
} board_t;

typedef struct {
    int game_time;
    int score;
    int difficulty;
    char* name;
    int university_id;
    char* game_state;
    int ready_to_jump;
    int seed;
}  scoreboard_t;

void readArtFromFile(const char *filename, model_t *model);
void readGameSettings(const char *filename, scoreboard_t *s);
void createBoardBorder(board_t *b, model_t frog_m);
void checkAndMoveCar(board_t *board, model_t car_m, position_t *car_start_pos, char direction, int laneIndex);
void getCarsOnLane(moving_object_t *cars_on_lane, board_t *board, int direction, int laneIndex, int *number_of_cars);
void getInput(char *move);
void timeOutCar(board_t *board, int id, int time);
void freeArtMemory(model_t *model);
void drawGameplayLayer(board_t *b);
void freeBoardMemory(board_t *board);
void allocateArtMemory(model_t *model); // changed a bit
void drawVisualLayer(const board_t b); // changed reference to read only copy
void createMultipleLanes(board_t *b, model_t lane_m); //changed the model thing - now not assigning it to object
void placeObject(board_t *b, object_t *obj, model_t model);
void createCarsOnLanes(board_t *b, model_t car_m, int cars_amount, char direction); // the model thing
void placeCarOnLane(board_t *b, model_t car_m, int laneIndex, int *cars_to_place, char direction); // the model thing
void processCarObject(board_t *b, model_t car_m, position_t car_start_pos, char direction, int *cars_to_place, int laneIndex); //minor model thing changes
void printScoreboard( scoreboard_t *s); //define kinda change
void moveObject(board_t *b, object_t *o, char direction, model_t model); //model thing changes
void moveCars(board_t *board, int time, model_t car_left, model_t car_right);  // some bigger model thing changes
void refreshBoard(board_t *b, models_set_t models); // had to pass models now
void checkFrogCollision(board_t *board, scoreboard_t *scoreboard, model_t frog_m); //added frog model as an argument
void checkAndAdjustCarSpeed(board_t *board, int laneIndex, moving_object_t *car, int direction);

void initializeScoreboard(scoreboard_t *scoreboard);
void prepareBoard(board_t *board);
void initializeModel(model_t *model,char *name, position_t size, const char *artPath);
void initializeModels(models_set_t *models, board_t *board, const char *defaultArt, const char *defaultSettings, scoreboard_t *scoreboard);
void initializeBoard(board_t *b);
void initializeGame(models_set_t *models, board_t *board, const char *defaultArt, const char *defaultSettings, scoreboard_t *scoreboard);
void initializeMovingCars(moving_cars_set_t *cars, int amount);



void readArtFromFile(const char *filename, model_t *model) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }
    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), file)) {
        if (strstr(line, model->name) != NULL) {
            found = 1;
            if (fscanf(file, "%d %d", &model->size.x, &model->size.y) != 2) {
                perror("Error reading dimensions from file");
                fclose(file);
                exit(1);
            }
            allocateArtMemory(model);
            fgetc(file);
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
        return;
    }

    int maxDifficulty = MAX_DIFFICULTY;
    if (fscanf(file, "%d/%d\n", &(s->difficulty), &maxDifficulty) != 2) {
        fclose(file);
        return;
    }


    if (fscanf(file, "%d\n", &(s->seed)) != 1) {
        fclose(file);
        return;
    }

    fclose(file);
}

void initializeBoard(board_t *b) {
     for (int i = 1; i < b->size-1; i++) {
        for (int j = 1; j < b->size-1; j++) {
            b->visual_layer[i][j] = ' ';
            b->gameplay_layer[i][j] = ' ';
        }
    }
}

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
        cars->objects[i].object.center.x = -MAX_MODEL_SIZE;
        cars->objects[i].object.center.y = -MAX_MODEL_SIZE;
        cars->objects[i].direction = 'd';
        cars->objects[i].speed = MIN_SPEED;
        cars->objects[i].timeout_until = NOT_TIMED_OUT;

    }
}

void createBoardBorder(board_t *b, model_t frog_m) {
    for (int i = 0; i < b->size; i++) {
        for (int j = 0; j < b->size; j++) {
            if ((i == 0 && j == 0) || (i == 0 && j == b->size - 1) || (i == b->size - 1 && j == 0) || (i == b->size - 1 && j == b->size - 1)) {
                b->visual_layer[i][j] = '+';
                b->gameplay_layer[i][j] = BORDER; // as in 'border'
            }
            else if ((i == 0 && (j<(b->size/2 +frog_m.size.y) || j>(b->size/2 + frog_m.size.y)))|| i == b->size - 1) { // change to being based on frog size and not '5'
                b->visual_layer[i][j] = '-';
                b->gameplay_layer[i][j] = BORDER;
            }
            else if (i == 0 && (j>=(b->size/2 -frog_m.size.y) || j<=(b->size/2 + frog_m.size.y))){
                if((j == (b->size/2 -frog_m.size.y)) || (j==(b->size/2 + frog_m.size.y))){
                    b->visual_layer[i][j] = ' ';
                }
                else{
                    b->visual_layer[i][j] = '!';
                }
                b->gameplay_layer[i][j] = FINISH_LINE; // as in 'win'
              
            }
            else if (j == 0 || j == b->size - 1) {
                b->visual_layer[i][j] = '|';
                b->gameplay_layer[i][j] = BORDER;
            }
        }
    }
}

int isCarCloseBy(board_t *board, model_t car_m, position_t *car_start_pos, char direction, int laneIndex, moving_object_t *close_car) {
    moving_object_t cars_on_lane[board->cars.amount];
    int direction_sign = (direction == 'd') ? 1 : -1;
    int number_of_cars = 0;

    getCarsOnLane(cars_on_lane, board, direction, laneIndex, &number_of_cars);
    if(number_of_cars <3){
        cars_on_lane[0].speed +=2;
    }
    for (int i = 0; i < number_of_cars; i++) {
        position_t car_pos = cars_on_lane[i].object.center;

        if ((direction_sign == 1 && car_pos.y > car_start_pos->y) ||
            (direction_sign == -1 && car_pos.y < car_start_pos->y)) {

            int distY = car_pos.y - car_start_pos->y;
            if (abs(distY) - car_m.size.y <= PROXIMITY_THRESHOLD) {
                *close_car = cars_on_lane[i];
                return 1;
            }
        }
    }

    return 0;
}

void checkAndMoveCar(board_t *board, model_t car_m, position_t *car_start_pos, char direction, int laneIndex) {
    moving_object_t close_car;
    if (isCarCloseBy(board, car_m, car_start_pos, direction, laneIndex, &close_car)) {
        int direction_sign = (direction == 'd') ? 1 : -1;
        car_start_pos->y -= (car_m.size.y) * direction_sign;
    }
}

void getCarsOnLane(moving_object_t *cars_on_lane, board_t *board, int direction, int laneIndex, int *number_of_cars) {
    for (int i = 0; i < board->cars.amount; i++) {
        moving_object_t *current_car = &board->cars.objects[i];
        if (current_car->timeout_until == NOT_TIMED_OUT &&
            current_car->lane_number == laneIndex &&
            current_car->direction == direction) {
            cars_on_lane[*number_of_cars] = *current_car;
            (*number_of_cars)++;
        }
    }
}

void getInput(char *move) {
    initscr();
    timeout(0);
    cbreak();
    noecho();
    keypad(stdscr, TRUE); // might not be required

    char input_char = getch();

    if (input_char != ERR) {
        *move = input_char;
    }

    endwin();
}

void timeOutCar(board_t *board, int id, int time) {
    int random_timeout_ms = time + 30*(10+ rand() % 10);
    board->cars.objects[id].timeout_until = random_timeout_ms;
    board->cars.objects[id].speed = 0;
    board->cars.objects[id].object.center.x = OUT_OF_BOARD;
    board->cars.objects[id].object.center.y = OUT_OF_BOARD;
}

void freeArtMemory(model_t *model) {
    if (model->art == NULL) return;
    for (int i = 0; i < model->size.x; i++) {
        free(model->art[i]);
    }
    free(model->art);
    model->art = NULL;
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

void freeBoardMemory(board_t *board) {
    if (board->car_lanes.car_lanes) {
        free(board->car_lanes.car_lanes);
    }
    if (board->car_lanes.car_lanes_array) {
        free(board->car_lanes.car_lanes_array);
    }
    if (board->trees[0].center.x) { // Check if tree array is non-NULL before accessing
        for (int i = 0; i < MAX_TREES; i++) {
            if (board->trees[i].center.x) { // Free the object_t name string
                free(&(board->trees[i]));
            }
        }
        free(board->trees); // Free the array itself
    }

    if (board->visual_layer) {
        for (int i = 0; i < board->size; i++) {
            if (board->visual_layer[i]) {
                free(board->visual_layer[i]);
            }
            if (board->gameplay_layer[i]) {
                free(board->gameplay_layer[i]);
            }
        }
        free(board->visual_layer);
        free(board->gameplay_layer);
    }

    if (board->cars.objects) {
        free(board->cars.objects);
    }
}

void freeModelMemory(models_set_t *models){
    freeArtMemory(&models->lane);
    freeArtMemory(&models->car_left);
    freeArtMemory(&models->car_right);
    freeArtMemory(&models->frog);
    freeArtMemory(&models->tree);
}

void printCarsPositions(const board_t b) {
    printf("Cars (total: %d) :\n", b.cars.amount);
    for (int i = 0; i < b.cars.amount; i++) {
        printf("Car %d: (%d, %d) %s %d %d %d\n", i + 1, b.cars.objects[i].object.center.x, b.cars.objects[i].object.center.y, b.cars.objects[i].object.name, b.cars.objects[i].speed, b.cars.objects[i].timeout_until,b.cars.objects[i].lane_number);
    }
}

void allocateArtMemory(model_t *model) {
    int numRows = model->size.x;
    int numCols = model->size.y;
    model->art = malloc(numRows * sizeof(char *));
    if ((model->art) == NULL) {
        perror("Error allocating art memory");
        exit(1);
    }

    for (int i = 0; i < numRows; i++) {
        ((model->art))[i] = malloc(numCols * sizeof(char));
        if ((model->art)[i] == NULL) {
            perror("Error allocating art row memory");

            for (int j = 0; j < i; j++) free((model->art)[j]);
            free(model->art);
            exit(1);
        }
    }
}

void prepareBoard(board_t *board) {
    board->size = SIZE;
    board->car_lanes.car_lane_amount = 0;
    board->cars.amount = 0;

    board->car_lanes.car_lanes = calloc(MAX_LANES, sizeof(int));
    board->car_lanes.car_lanes_array = malloc(MAX_LANES * sizeof(moving_object_t));
    board->trees = malloc(MAX_TREES * sizeof(object_t));
    board->visual_layer = malloc(SIZE * sizeof(char *));
    board->gameplay_layer = malloc(SIZE * sizeof(char *));

    if (!board->visual_layer || !board->gameplay_layer) {
        perror("Error allocating board layers");
        exit(1);
    }

    for (int i = 0; i < SIZE; i++) {
        board->visual_layer[i] = malloc(SIZE * sizeof(char));
        board->gameplay_layer[i] = malloc(SIZE * sizeof(char));
        if (!board->visual_layer[i] || !board->gameplay_layer[i]) {
            perror("Error allocating rows in board layers");
            exit(1);
        }
    }
}

void initializeScoreboard(scoreboard_t *scoreboard) {
    scoreboard->score = 0;
    scoreboard->game_time = 0;
    scoreboard->game_state = "in game";
    scoreboard->ready_to_jump = READY;
    scoreboard->name = "Emilia Łukasiuk";
    scoreboard->university_id = 203620;
}

void initializeModel(model_t *model,char *name, position_t size, const char *artPath) {
    model->size = size;
    model->name = name;
    readArtFromFile(artPath, model);
}

void initializeModels(models_set_t *models, board_t *board, const char *defaultArt, const char *defaultSettings, scoreboard_t *scoreboard) {
    position_t max_model_size_array = {MAX_MODEL_SIZE, MAX_MODEL_SIZE};
    initializeModel(&models->lane, "lane", max_model_size_array, defaultArt);
    initializeModel(&models->car_right, "car_right", max_model_size_array, defaultArt);
    initializeModel(&models->car_left, "car_left", max_model_size_array, defaultArt);
    initializeModel(&models->frog, "frog", max_model_size_array, defaultArt);
    initializeModel(&models->tree, "tree", max_model_size_array, defaultArt);

    board->frog.name = models->frog.name;
    board->frog.intent = 'f';
    board->frog.center = (position_t){ board->size - models->frog.size.x - STEP_SIZE, board->size/2};

    readGameSettings(defaultSettings, scoreboard);
}

void initializeGame(models_set_t *models, board_t *board, const char *defaultArt, const char *defaultSettings, scoreboard_t *scoreboard){
    initializeScoreboard(scoreboard);
    prepareBoard(board);
    initializeModels(models, board, defaultArt, defaultSettings, scoreboard);
    initializeBoard(board);
    initializeMovingCars(&(board->cars), AMOUNT_OF_CARS);
}

void drawVisualLayer(const board_t b) {
    printf("\033[H\033[J");
    for (int i = 0; i < b.size; i++) {
        for (int j = 0; j < b.size; j++) {
            char color_code[10] = "\033[0m";
            switch (b.gameplay_layer[i][j]) {
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
            if (b.visual_layer[i][j] != ' ') {
                printf("%s%c" "\033[0m", color_code, b.visual_layer[i][j]);
            } 
            else {
                printf("%c", b.visual_layer[i][j]);
            }
        }
        printf("\n");
    }
}

void createMultipleLanes(board_t *b, model_t lane_m) {
    int lanes_amount = (b->size - 2) / (lane_m.size.x * 1.5); 
    lanes_amount = lanes_amount > MAX_LANES ? MAX_LANES : lanes_amount;

    for (int i = 0; i < lanes_amount; i++) {
        position_t lane_start_pos = {(i + 1) * (1.4 * lane_m.size.x), b->size / 2};

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
            b->car_lanes.car_lanes_array[i] = lane;
            placeObject(b, &lane, lane_m);
        }
    }
}

void createTrees(board_t *b, model_t tree_m, model_t lane_m) {
    int car_lane_amount = b->car_lanes.car_lane_amount;
    int trees_per_lane = MAX_TREES/car_lane_amount;
    for (int i = 0; i < car_lane_amount; i++) {
        int lane_pos = b->car_lanes.car_lanes_array[i].center.x;
        int tree_pos_x = (lane_pos - lane_m.size.x / 2 - tree_m.size.x / 2 - lane_m.size.x % 2);
        for(int j = 0; j < trees_per_lane; j++) {
            int tree_pos_y = (rand() % (b->size / tree_m.size.y)) * (j + 1);
            object_t tree;
            tree.center = (position_t){tree_pos_x, tree_pos_y};
            tree.name = "tree";
            tree.intent = 'n'; // neutral
            
            // Assign the tree to the array
            b->trees[(i*trees_per_lane + j)] = tree;
            
            // Place the object (this might need to handle memory allocation for the object)
            placeObject(b, &tree, tree_m);
        }
    }
}

void placeObject(board_t *b, object_t *obj, model_t model) {
    const char symbol = (obj->name[0])?(obj->name[0]):(' ');
    int is_full_length = (strcmp(obj->name, "lane") == 0) ? 1 : 0;
    const int sizeY = is_full_length ? b->size : model.size.y;
    const int sizeX = model.size.x;
    position_t size = {sizeX,sizeY};

    for (int i = 0; i < sizeX; i++) {
        for (int j = 0; j < sizeY; j++) {

            int posX = obj->center.x - (model.size.x / 2) + i;
            int posY = obj->center.y - (model.size.y / 2) + j;

            int artY = j;
            if (is_full_length) {
                artY = j % model.size.y;
                posY = ( (model.size.y)/ 2) + j -3;
            }
            int artX = i;

            if (posX > 0 && posX < b->size -1 && posY > 0 && posY < b->size -1) {
                b->visual_layer[posX][posY] = model.art[artX][artY];
                if(symbol !='f'){
                    b->gameplay_layer[posX][posY] = (model.art[artX][artY] !=' ')?symbol:' ';
                }
                else {
                b->gameplay_layer[posX][posY] = symbol;
                }
            }
        }
    }
}

void createCarsOnLanes(board_t *b, model_t car_m, int cars_amount, char direction) {
    int max_cars_per_lane = (b->size - 2) / (car_m.size.y * 1.5);
    max_cars_per_lane = max_cars_per_lane > cars_amount ? cars_amount : max_cars_per_lane;
    int cars_to_place = cars_amount;
    int laneIndex = 0;
    while (cars_to_place > 0) {
        placeCarOnLane(b, car_m, laneIndex, &cars_to_place, direction);
        laneIndex = (laneIndex + 1) % (b->car_lanes.car_lane_amount);
    }
}

void placeCarOnLane(board_t *b, model_t car_m, int laneIndex, int *cars_to_place, char direction) { //   this was wrong - perhaps any arising issues come from this spot
    int max_cars_per_lane = (b->size - 2) / (car_m.size.y * 1.5);
    int cars_in_lane = (*cars_to_place) / (b->car_lanes.car_lane_amount);
    int direction_sign = (direction == 'd') ? 1 : -1;
    if (cars_in_lane < max_cars_per_lane) {
        int base_y = ((cars_in_lane) * ((b->size / car_m.size.y) * (car_m.size.y / 2))) + (rand() % (car_m.size.y / 4)) - (car_m.size.y / 4 - 1);
        if (direction_sign == -1) {
            base_y = b->size - base_y;
        }
        position_t car_start_pos = {
            fmax(
                b->car_lanes.car_lanes[laneIndex] + (direction_sign) * (car_m.size.x) - (direction_sign),
                0
            ),
            base_y
        };
        checkAndMoveCar(b, car_m, &car_start_pos, direction, laneIndex);
        processCarObject(b, car_m, car_start_pos, direction, cars_to_place,laneIndex);
    }
}

void processCarObject(board_t *b, model_t car_m, position_t car_start_pos, char direction, int *cars_to_place, int laneIndex) {
    for (int i = 0; i < b->cars.amount; i++) {
        if (b->cars.objects[i].object.center.x <=0 && b->cars.objects[i].object.center.y <=0) {
            object_t *car = &(b->cars.objects[i].object);
            car->name = car_m.name;
            car->center = car_start_pos;
            placeObject(b, car, car_m);
            b->cars.objects[i].lane_number = laneIndex;
            b->cars.objects[i].timeout_until = NOT_TIMED_OUT;
            b->cars.objects[i].direction = direction;
            b->cars.objects[i].speed = (MIN_SPEED + rand() % (MAX_SPEED-MIN_SPEED)); //for non zero speeds
            (*cars_to_place)--;
            break;
        }
    }
}

void printScoreboard( scoreboard_t *s) {
    if (!s) {
        return;
    }

    float game_time_seconds = s->game_time / 1000.0;
    float max_game_time_seconds = MAX_GAME_TIME / 1000.0;
    
    printf("\n");
    printf("score: %d \t", s->score);
    printf("time: %.1f/%.0fs\n", game_time_seconds, max_game_time_seconds);
    printf("difficulty: %d\n", s->difficulty);
    printf("game state: %s\t", s->game_state ? s->game_state : "N/A");
    printf("frog: %s\n", (s->ready_to_jump == READY) ? "ready" : "resting");
    printf("author: %s\n", s->name ? s->name : "N/A");
    printf("university ID: %d\n", s->university_id);
    
}

void moveObject(board_t *b, object_t *o, char direction, model_t model) {
    // Clear the object's current position
    for (int i = 0; i < model.size.x; i++) {
        for (int j = 0; j < model.size.y; j++) {
            int posX = o->center.x - model.size.x / 2 + i;
            int posY = o->center.y - model.size.y / 2 + j;
            if (posX >= 0 && posX < b->size && posY >= 0 && posY < b->size) {
                b->visual_layer[posX][posY] = ' ';
                b->gameplay_layer[posX][posY] = ' ';
            }
        }
    }

    // Determine movement direction
    position_t step = {0, 0};
    switch (direction) {
        case 'w': step.x = -STEP_SIZE; break; // Move up
        case 's': step.x = STEP_SIZE; break;  // Move down
        case 'a': step.y = -STEP_SIZE; break; // Move left
        case 'd': step.y = STEP_SIZE; break;  // Move right
        default: return; // Invalid direction, return early
    }

    // Check object type (car or frog)
    if (o->intent == 'e') {
        // Cars: unrestricted movement
        o->center.x += step.x;
        o->center.y += step.y;
    } else if (o->intent == 'f') {
        // Frogs: restricted to valid spaces
        int newX = o->center.x + step.x;
        int newY = o->center.y + step.y;

        // Validate the new position for the frog
        bool isValid = true;
        for (int i = 0; i < model.size.x; i++) {
            for (int j = 0; j < model.size.y; j++) {
                int posX = newX - model.size.x / 2 + i;
                int posY = newY - model.size.y / 2 + j;

                // Ensure the position is within board boundaries and not on invalid spaces
                if (posX < 0 || posX >= b->size || posY < 0 || posY >= b->size || 
                    b->gameplay_layer[posX][posY] == 'b' || b->gameplay_layer[posX][posY] == 't') {
                    isValid = false;
                    break;
                }
            }
            if (!isValid) break;
        }

        // If valid, update the frog's position
        if (isValid) {
            o->center.x = newX;
            o->center.y = newY;
        }
    }

    // Place the object at its new position
    placeObject(b, o, model);
}

void moveCars(board_t *board, int time, model_t car_left, model_t car_right) {
    for (int i = 0; i < board->cars.amount; i++) {
        int left_edge = board->cars.objects[i].object.center.y + car_left.size.y; // car_left and car_right's dimensions should be the same - the differrence is the ascii art 
        int right_edge = board->cars.objects[i].object.center.y - car_left.size.y;
        if(board->cars.objects[i].timeout_until == NOT_TIMED_OUT ){
            if (right_edge < board->size && left_edge > 0) {
                if(time%(MAX_SPEED - board->cars.objects[i].speed) == 0){
                    checkAndAdjustCarSpeed(board,board->cars.objects[i].lane_number, &board->cars.objects[i], board->cars.objects[i].direction);
                    if(board->cars.objects[i].direction == 'd'){
                        moveObject(board, &board->cars.objects[i].object, 'd', car_right);
                    }
                    else if(board->cars.objects[i].direction == 'a'){
                        moveObject(board, &board->cars.objects[i].object, 'a', car_left);
                    }
                }
            }
            else{
                timeOutCar(board, i, time);
            }
        }
        else if(board->cars.objects[i].timeout_until < time){
            int car_amount = 1;
            model_t model = (board->cars.objects[i].direction == 'd')?(car_right):(car_left);
            placeCarOnLane(board, model, (board->cars.objects[i].lane_number), &car_amount, board->cars.objects[i].direction);
        }
    }
}

void refreshBoard(board_t *b, models_set_t models){
    if(b->car_lanes.car_lane_amount){
        for(int i=0; i<(b->car_lanes.car_lane_amount); i++){
            placeObject(b, &(b->car_lanes.car_lanes_array[i]), models.lane);
        }
    }
    if(b->trees[0].center.x){
        for(int i=0; i<(b->car_lanes.car_lane_amount); i++){
            int trees_per_lane = (MAX_TREES/b->car_lanes.car_lane_amount);
            for(int j=0; j<trees_per_lane; j++){
                placeObject(b, &(b->trees[(i*trees_per_lane)+j]), models.tree);
            }
        }
    }
    if(b->cars.amount){
        for(int i=0; i<(b->cars.amount); i++){
            if( b->cars.objects[i].direction == 'd'){
                placeObject(b, &(b->cars.objects[i].object), models.car_right);
            }
            else {
                placeObject(b, &(b->cars.objects[i].object), models.car_left);
            }
        }
    }
    if(b->frog.center.x){
        placeObject(b, &(b->frog), models.frog);
    }
    createBoardBorder(b, models.frog);
};

void checkFrogCollision(board_t *board, scoreboard_t *scoreboard, model_t frog_m) {
    int radius = STEP_SIZE;
    int frogX = board->frog.center.x;
    int frogY = board->frog.center.y;
    int sizeX = frog_m.size.x / 2; // Half-width of the frog
    int sizeY = frog_m.size.y / 2; // Half-height of the frog

    // Bounds of the frog
    int minX = frogX - sizeX;
    int maxX = frogX + sizeX;
    int minY = frogY - sizeY;
    int maxY = frogY + sizeY;

    // Outer bounds for collision checking
    int outerMinX = minX - radius;
    int outerMaxX = maxX + radius;
    int outerMinY = minY - radius;
    int outerMaxY = maxY + radius;

    for (int i = outerMinX; i <= outerMaxX; i++) {
        for (int j = outerMinY; j <= outerMaxY; j++) {
            // Skip squares inside the frog's bounds
            if (i >= minX && i <= maxX && j >= minY && j <= maxY) {
                continue;
            }

            // Ensure indices are within board boundaries
            if (i >= 0 && i < board->size && j >= 0 && j < board->size) {
                if (board->gameplay_layer[i][j] == 'c') {
                    scoreboard->game_state = "game over";
                    return;
                }
                if (board->gameplay_layer[i][j] == 'w') {
                    scoreboard->game_state = "game won!";
                    return;
                }
            }
        }
    }
}

void printBoardSpecs(board_t *board, model_t frog_m) {
    printf("Board Specifications:\n");
    printf("Size: %d\n", board->size);

    printf("\nFrog:\n");
    printf("  Name: %s\n", board->frog.name);
    printf("  Intent: %c\n", board->frog.intent);
    printf("  Center: { x: %d, y: %d }\n",
           board->frog.center.x,
           board->frog.center.y);
    printf("  Model Name: %s\n", frog_m.name);
    printf("  Model Size: { x: %d, y: %d }\n",
           frog_m.size.x,
           frog_m.size.y);
    if (frog_m.art) {
        printf("  Frog Art:\n");
        for (int i = 0; frog_m.art[i] != NULL; i++) {
            printf("%s\n", frog_m.art[i]);
        }
    }
}

void adjustCarSpeed(moving_object_t *car, int closest_distance) { // THERE ARE STILL COLLISIONS!!!!
    if (closest_distance < PROXIMITY_THRESHOLD) {
        int adjustment = PROXIMITY_THRESHOLD - closest_distance;
        car->speed -= adjustment/4;
        if (car->speed < 1) {
            car->speed = 1;
        } else if (car->speed > 10) {
            car->speed = 10;
        }
    }
}

int getClosestCarDistance(moving_object_t *cars, int car_count, position_t current_pos, int direction) {
    int closest_distance = PROXIMITY_THRESHOLD + 1;
    int found = 0;

    for (int i = 0; i < car_count; i++) {
        int relative_pos = cars[i].object.center.y - current_pos.y;

        if ((direction == 1 && relative_pos > 0) || (direction == -1 && relative_pos < 0)) {
            int distance = abs(relative_pos);
            if (distance <= PROXIMITY_THRESHOLD && distance < closest_distance) {
                closest_distance = distance;
                found = 1;
            }
        }
    }

    return found ? closest_distance : -1;
}

void checkAndAdjustCarSpeed(board_t *board, int laneIndex, moving_object_t *car, int direction) {

    moving_object_t cars_on_lane[board->cars.amount];
    int car_count = 0;
    getCarsOnLane(cars_on_lane, board, direction, laneIndex, &car_count);

    int closest_distance = getClosestCarDistance(cars_on_lane, car_count, car->object.center, direction);

    adjustCarSpeed(car, closest_distance);
}

void createGameElements(board_t *board, models_set_t models){
    createBoardBorder(board, models.frog);
    createMultipleLanes(board, models.lane);
    createTrees(board, models.tree, models.lane);
    createCarsOnLanes(board, models.car_right, (AMOUNT_OF_CARS/2 + AMOUNT_OF_CARS%2), RIGHT);
    createCarsOnLanes(board, models.car_left, AMOUNT_OF_CARS/2, LEFT);
}

void gameLoop(scoreboard_t *scoreboard, board_t *board, models_set_t models) {
        char moveDirection=' ';
        scoreboard->game_time += TIME_STEP;
        getInput(&moveDirection);
        if(moveDirection != ' ' && scoreboard->ready_to_jump == READY){
            moveObject(board, &(board->frog), moveDirection, models.frog);
            scoreboard->ready_to_jump = (MIN_DIFFICULTY+(scoreboard->difficulty)%MAX_DIFFICULTY);
        }
        moveCars(board, scoreboard->game_time, models.car_left, models.car_right);
        refreshBoard(board, models);
        drawVisualLayer(*board);
        // drawGameplayLayer(board);
        // printCarsPositions(board);
        checkFrogCollision(board, scoreboard, models.frog);
        if(scoreboard->game_time  >= MAX_GAME_TIME){
            scoreboard->game_state = "game over";
        }
        printScoreboard(scoreboard);
        if(scoreboard->ready_to_jump != READY){
            scoreboard->ready_to_jump--;
        }
        // printBoardSpecs(&board, models.frog);
}

int main() {
    // Define main game structures
    scoreboard_t scoreboard;
    board_t board;
    models_set_t models;
    const char defaultArt[] = "art_file.txt";
    const char defaultSettings[] = "game_settings.txt";

    // Initialize elements with proper values and tie them together
    initializeGame(&models, &board, defaultArt, defaultSettings, &scoreboard);

    // Initialize the seed for random elements in the game
    srand(scoreboard.seed);

    // Put the elements into the board so they're visible and playable
    createGameElements(&board, models);

    // Draw the first frame
    drawVisualLayer(board);
    printScoreboard(&scoreboard);

    // Loop the gameLoop function if the game_state variable stays as 'in game'
    while (strcmp(scoreboard.game_state, "in game") == 0) {
        gameLoop(&scoreboard, &board, models);
        usleep(TIME_STEP * 1000); // Timeout between frames; convert from microseconds to milliseconds
    }

    // Free allocated memory
    freeModelMemory(&models);
    freeBoardMemory(&board);

    return 0;
}

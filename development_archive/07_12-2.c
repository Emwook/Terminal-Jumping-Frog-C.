#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <math.h>
#include <unistd.h>
#include <curses.h>

#define MAX_SPEED 10        // in scale 0-10
#define MAX_GAME_TIME 20000 //miliseconds
#define MAX_MODEL_SIZE 15   // characters
#define STEP_SIZE 1         // character
#define TIME_STEP 100       // miliseconds
#define NOT_TIMED_OUT -1    // generic way to show that car is free to move

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
} models_set_t;

typedef struct {
    int size;
    char **visual_layer;
    char **gameplay_layer;
    car_lanes_set_t car_lanes;
    moving_cars_set_t cars;
    object_t frog;
} board_t;

typedef struct {
    int game_time;
    int score;
    int current_turn;
    int number_of_turns;
    char* difficulty;
    char* name;
    int university_id;
    char* game_state;
}  scoreboard_t;

//pasted functions

void readArtFromFile(const char *filename, model_t *model);
void readGameSettings(const char *filename, scoreboard_t *s);
void initializeBoard(board_t *b);
void initializeMovingCars(moving_cars_set_t *cars, int amount);
void createBoardBorder(board_t *b);
void checkAndMoveCar(board_t *board, model_t car_m, position_t *car_start_pos, char direction, int laneIndex);
void getCarsOnLane(position_t *cars_on_lane, board_t *board, int direction, int laneIndex, int *number_of_cars);
void getInputAndProcess(char *move);
void timeOutCar(board_t *board, int id, int time);
void freeArtMemory(model_t *model);
void drawGameplayLayer(board_t *b);
void freeBoardMemory(board_t *board);

//pasted functions



//new functions

void allocateArtMemory(model_t *model); // changed a bit
void prepareBoard(board_t *board, int size, int max_lanes);
void initializeScoreboard(scoreboard_t *scoreboard);
void initializeModel(model_t *model,char *name, position_t size, const char *artPath);
void initializeGame(models_set_t *models, board_t *board, const char *defaultArt, const char *defaultSettings, scoreboard_t *scoreboard);
void drawVisualLayer(const board_t b); // changed reference to read only copy
void createMultipleLanes(board_t *b, model_t lane_m, int max_lanes); //changed the model thing - now not assigning it to object
void placeObject(board_t *b, object_t *obj, model_t model);
void createCarsOnLanes(board_t *b, model_t car_m, int cars_amount, char direction); // the model thing
void placeCarOnLane(board_t *b, model_t car_m, int laneIndex, int *cars_to_place, char direction); // the model thing
void processCarObject(board_t *b, model_t car_m, position_t car_start_pos, char direction, int *cars_to_place, int laneIndex); //minor model thing changes
void printScoreboard( scoreboard_t *s); //define kinda change
void moveObject(board_t *b, object_t *o, char direction, model_t model); //model thing changes
void moveCars(board_t *board, int time, model_t car_left, model_t car_right);  // some bigger model thing changes
void refreshBoard(board_t *b, models_set_t models); // had to pass models now
void checkFrogCollision(board_t *board, scoreboard_t *scoreboard, model_t frog_m); //added frog model as an argument

//new functions


// pasted stuff here

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
        cars->objects[i].speed = 1;
        cars->objects[i].timeout_until = NOT_TIMED_OUT;

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
        }
    }
}

void checkAndMoveCar(board_t *board, model_t car_m, position_t *car_start_pos, char direction, int laneIndex){
    position_t cars_on_lane[board->cars.amount];
    int direction_sign = (direction == 'd') ? 1 : -1;
    int number_of_cars = 0;
    getCarsOnLane(cars_on_lane, board, direction, laneIndex, &number_of_cars);
    for(int i = 0; i< number_of_cars; i++){
        if((direction_sign == 1)&&(cars_on_lane[i].y > car_start_pos->y) || (direction_sign == -1)&&(cars_on_lane[i].y < car_start_pos->y)){
            int distY = 0;
            distY= (cars_on_lane[i].y - car_start_pos->y);
            if(distY - car_m.size.y <= 1){
                car_start_pos->y -= (car_m.size.y)*(direction_sign);
                break;
            }
        }
    }
}

void getCarsOnLane(position_t *cars_on_lane, board_t *board, int direction, int laneIndex, int *number_of_cars) {
    for (int i = 0; i < board->cars.amount; i++) {
        if (board->cars.objects[i].timeout_until == NOT_TIMED_OUT &&
            board->cars.objects[i].lane_number == laneIndex &&
            board->cars.objects[i].direction == direction) {
            cars_on_lane[*number_of_cars] = board->cars.objects[i].object.center;
            (*number_of_cars)++;
        }
    }
}

void getInputAndProcess(char *move) {
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
    int random_timeout_ms = time + 300*(5+ rand() % 10);
    board->cars.objects[id].timeout_until = random_timeout_ms;
    board->cars.objects[id].speed = 0;
    board->cars.objects[id].object.center.x = 0;
    board->cars.objects[id].object.center.y = 0;
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

// pasted stuff here



// new stuff here

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

void prepareBoard(board_t *board, int size, int max_lanes) {
    board->size = size;
    board->car_lanes.car_lane_amount = 0;
    board->cars.amount = 0;

    board->car_lanes.car_lanes = calloc(max_lanes, sizeof(int));
    board->car_lanes.car_lanes_array = malloc(max_lanes * sizeof(moving_object_t));

    board->visual_layer = malloc(size * sizeof(char *));
    board->gameplay_layer = malloc(size * sizeof(char *));

    if (!board->visual_layer || !board->gameplay_layer) {
        perror("Error allocating board layers");
        exit(1);
    }

    for (int i = 0; i < size; i++) {
        board->visual_layer[i] = malloc(size * sizeof(char));
        board->gameplay_layer[i] = malloc(size * sizeof(char));
        if (!board->visual_layer[i] || !board->gameplay_layer[i]) {
            perror("Error allocating rows in board layers");
            exit(1);
        }
    }
}

void initializeScoreboard(scoreboard_t *scoreboard) {
    scoreboard->score = 0;
    scoreboard->current_turn = 1;
    scoreboard->game_time = 0;
    scoreboard->game_state = "in game";
}

void initializeModel(model_t *model,char *name, position_t size, const char *artPath) {
    model->size = size;
    model->name = name;
    allocateArtMemory(model);
    readArtFromFile(artPath, model);
}

void initializeGame(models_set_t *models, board_t *board, const char *defaultArt, const char *defaultSettings, scoreboard_t *scoreboard) {
    position_t max_model_size_array = {MAX_MODEL_SIZE, MAX_MODEL_SIZE};
    initializeModel(&models->lane, "lane", max_model_size_array, defaultArt);
    initializeModel(&models->car_right, "car_right", max_model_size_array, defaultArt);
    initializeModel(&models->car_left, "car_left", max_model_size_array, defaultArt);
    initializeModel(&models->frog, "frog", max_model_size_array, defaultArt);

    board->frog.name = models->frog.name;
    board->frog.intent = 'f';
    board->frog.center = (position_t){ board->size - models->frog.size.x - STEP_SIZE, board->size/2};

    readGameSettings(defaultSettings, scoreboard);
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

void createMultipleLanes(board_t *b, model_t lane_m, int max_lanes) {
    int lanes_amount = (b->size - 2) / (lane_m.size.x * 1.5); 
    lanes_amount = lanes_amount > max_lanes ? max_lanes : lanes_amount;

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
                if(symbol == 'l' ){
                    b->gameplay_layer[posX][posY] = (model.art[artX][artY] !=' ')?symbol:' ';
                }
                else{
                    b->gameplay_layer[posX][posY] = symbol;
                    // if(posX == obj->center.x && posY == obj->center.y){
                    //     b->gameplay_layer[posX][posY] = 'x';
                    // }
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
            b->cars.objects[i].speed = rand() % MAX_SPEED;
            (*cars_to_place)--;
            break;
        }
    }
}

void printScoreboard( scoreboard_t *s) {
    if (!s) {
        fprintf(stderr, "Invalid scoreboard pointer\n");
        return;
    }

    float game_time_seconds = s->game_time / 1000.0;
    float max_game_time_seconds = MAX_GAME_TIME / 1000.0;
    
    printf("\n");
    printf("score: %d \n", s->score);
    printf("turn: %d/%d\n", s->current_turn, s->number_of_turns);
    printf("difficulty: %s\n", s->difficulty ? s->difficulty : "N/A");
    printf("author: %s\n", s->name ? s->name : "N/A");
    printf("university ID: %d\n", s->university_id);
    // printf("time: %.1f/%.0fs\n", game_time_seconds, max_game_time_seconds);
    printf("time: %d \n", s->game_time);
    printf("game state: %s\n", s->game_state ? s->game_state : "N/A");
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
        // Frogs: restricted within board boundaries
        int newX = o->center.x + step.x;
        int newY = o->center.y + step.y;

        // Ensure new position is within board boundaries considering the model size
        if (newX >= (model.size.x / 2 + model.size.x%2) && newX < b->size -(model.size.x / 2 + model.size.x%2)) {
            o->center.x = newX;
        }
        if (newY >= (model.size.y / 2 + model.size.y%2) && newY < b->size - (model.size.y / 2 + model.size.y%2)) {
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
        if(board->cars.objects[i].timeout_until == NOT_TIMED_OUT){
            if ( board->cars.objects[i].direction == 'd' && right_edge < board->size) {
                moveObject(board, &board->cars.objects[i].object, 'd', car_right);
            } 
            else if(board->cars.objects[i].direction == 'a' &&  left_edge > 0){
                moveObject(board, &board->cars.objects[i].object, 'a', car_left);
            }
            else {
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
    createBoardBorder(b);
};

void checkFrogCollision(board_t *board, scoreboard_t *scoreboard, model_t frog_m) {
    int frogX = board->frog.center.x;
    int frogY = board->frog.center.y;
    int sizeX = frog_m.size.x / 2;
    int sizeY = frog_m.size.y / 2;
    int radius = STEP_SIZE;

    for (int i = -1; i <= 1; i += 2) {
        for (int j = -1; j <= 1; j += 2) {
            int to_check_x = (frogX + i * (sizeX + radius) >= 0 && frogX + i * (sizeX + radius) < board->size) 
                ? (frogX + i * (sizeX + radius)) 
                : frogX;

            int to_check_y = (frogY + j * (sizeY + radius) >= 0 && frogY + j * (sizeY + radius) < board->size) 
                ? (frogY + j * (sizeY + radius)) 
                : frogY;

            if (board->gameplay_layer[to_check_x][to_check_y] == 'c') {
                    scoreboard->game_state = "game over";
                return;
            }
            if (board->gameplay_layer[to_check_x][to_check_y] == 'w') {
                    scoreboard->game_state = "game won!";
                return;
            }
        }
    }
}

// new stuff here


// totally new things 

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

// totally new things


int main() {
    const int SIZE = 50; // move to settings 
    const int MAX_LANES = 4; // move to settings 
    const int AMOUNT_OF_CARS = 9; // move to settings 

    const char defaultArt[] = "default_input.txt";
    const char defaultSettings[] = "game_setup.txt";

    // define main game structures
    scoreboard_t scoreboard;
    board_t board;
    models_set_t models;

    // initialize elements with proper values and tie them together
    initializeScoreboard(&scoreboard);
    prepareBoard(&board, SIZE, MAX_LANES);
    initializeGame(&models, &board, defaultArt, defaultSettings, &scoreboard);
    initializeBoard(&board);
    initializeMovingCars(&board.cars, AMOUNT_OF_CARS);

    // to take put the elements into the board so they're visible and playable
    createBoardBorder(&board);
    createMultipleLanes(&board, models.lane, MAX_LANES);
    createCarsOnLanes(&board, models.car_right, (AMOUNT_OF_CARS/2 + AMOUNT_OF_CARS%2), 'd');
    createCarsOnLanes(&board, models.car_left, AMOUNT_OF_CARS/2, 'a');

    // put on the board the frog user will be controlling
    placeObject(&board, &(board.frog), models.frog);

    drawVisualLayer(board);
    printScoreboard(&scoreboard);

    while (strcmp(scoreboard.game_state, "in game") == 0) {
        char moveDirection=' ';
        scoreboard.game_time += TIME_STEP;
        getInputAndProcess(&moveDirection);
        if(moveDirection != ' '){
            moveObject(&board, &(board.frog), moveDirection, models.frog);
        }
        moveCars(&board, scoreboard.game_time, models.car_left, models.car_right);
        // checkCarFrontCollision(&board); // not like this
        refreshBoard(&board, models);
        drawVisualLayer(board);
        // drawGameplayLayer(&board);
        // printCarsPositions(&board);
        checkFrogCollision(&board, &scoreboard, models.frog);
        if(scoreboard.game_time  >= MAX_GAME_TIME){
            scoreboard.game_state = "game over";
        }
        // printScoreboard(&scoreboard);
        // printBoardSpecs(&board, models.frog);
        usleep(TIME_STEP *1000); // change from microseconds to miliseconds 
    }

    freeBoardMemory(&board);
    freeArtMemory(&models.lane);
    freeArtMemory(&models.car_left);
    freeArtMemory(&models.car_right);

    return 0;
}



// old comments
// car collision not solved still
// cars cannot be put on (0,0) cuz its still on screen
// wymaganie co do znakow ughhhh sprawdz to koniecznie 2^10

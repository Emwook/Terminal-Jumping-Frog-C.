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
} moving_object_set_t;

typedef struct {
    int size;
    char **visual_layer;
    char **gameplay_layer;
    int *car_lanes;
    int car_lane_amount;
    // object_t *models;
    moving_object_set_t cars;
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



void readArtFromFile(const char *filename, object_t *o); // reads art from file and saves it as an art attribute in a specific object
void setObjectSizeAndCenter(object_t *o, position_t pos, position_t size); // sets position and size of a specific object 

void placeObject(board_t *b, object_t *obj); // places an object on both game and visual layers, repeats the object if necessery

void initializeBoard(board_t *b); // draws the board without any objects, just the border
void drawVisualLayer(board_t *b); // takes the visual layer components and with proper coloring prints it to terminal 

void drawGameplayLayer(board_t *b); // takes the game layer components and prints it to terminal

void moveObject(board_t *b, object_t *o, char direction); // shifts the object in a given direction if it would fit afterwards
void createMultipleLanes(board_t *b, object_t *lane, int max_lanes); // checks if the given amount of lanes would fit on the board, and creates as many as possible if not, and less if given less than maximum

void readGameSettings(const char *filename, scoreboard_t *s); // basic game setup in the file

void allocateArtMemory(object_t *obj); // to manage memory of objects' art
void freeArtMemory(object_t *obj);

void initializeBoard(board_t *b) {
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
    position_t stepBy = {1,1}; // change to dynamic stuff
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
        position_t lane_start_pos = {(i + 1) * (1.4 * lane->model.size.x), b->size / 2};

        // Check if the lane's x position already exists in car_lanes
        int alreadyPresent = 0;
        for (int j = 0; j < b->car_lane_amount; j++) {
            if (b->car_lanes[j] == (int)(lane_start_pos.x)) {
                alreadyPresent = 1;
                break;
            }
        }

        // If not present, add the lane
        if (!alreadyPresent) {
            b->car_lanes[b->car_lane_amount] = (int)(lane_start_pos.x); // Add x position
            b->car_lane_amount++; // Increment the count of car lanes

            setObjectSizeAndCenter(lane, lane_start_pos, lane->model.size);
            placeObject(b, lane);
        }
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

void printcar_lanes(const board_t *b) {
    if (!b || !b->car_lanes) {
        printf("No car lanes to display.\n");
        return;
    }

    printf("Car Lanes (Total: %d):\n", b->car_lane_amount);
    for (int i = 0; i < b->car_lane_amount; i++) {
        printf("Lane %d: %d\n", i + 1, b->car_lanes[i]);
    }
}

void initializeMovingObjects(moving_object_set_t *cars, int amount) {
    if (!cars) {
        fprintf(stderr, "Invalid pointer to moving_object_set_t\n");
        return;
    }

    // Set the number of moving objects
    cars->amount = amount;

    // Allocate memory for the array of moving objects
    cars->objects = (moving_object_t *)malloc(amount * sizeof(moving_object_t));

    // Check if memory allocation was successful
    if (!cars->objects) {
        perror("Failed to allocate memory for moving objects");
        exit(EXIT_FAILURE);
    }

    // Initialize the objects (optional)
    for (int i = 0; i < amount; i++) {
        cars->objects[i].object.intent = '\0'; // Default intent
        cars->objects[i].object.name = NULL;  // No name yet
        cars->objects[i].object.center.x = 0;
        cars->objects[i].object.center.y = 0;
        cars->objects[i].direction = '\0';    // Default direction
        cars->objects[i].speed = 0;           // Default speed
    }
}

void cleanupMovingObjects(moving_object_set_t *cars) {
    if (!cars) return;

    free(cars->objects);
    cars->objects = NULL;
    cars->amount = 0;
}


void createCarsOnLanes(board_t *b, object_t *car, int cars_amount, char direction) {
    int max_cars_per_lane = (b->size - 2) / (car->model.size.y * 1.5); 
    max_cars_per_lane = max_cars_per_lane > cars_amount ? cars_amount : max_cars_per_lane;

    int cars_to_place = cars_amount;
    int laneIndex = 0; // Start with the first lane
    int direction_sign = (direction == 'r')?1:(-1);
    while (cars_to_place > 0) {
        // Compute the car's position for the current lane
        int cars_in_lane = (cars_amount - cars_to_place) / (b->car_lane_amount);
        if (cars_in_lane < max_cars_per_lane) {
            position_t car_start_pos = {b->car_lanes[laneIndex] + (direction_sign)*(car->model.size.x) - (direction_sign), ((cars_in_lane+1) * (1.25 * car->model.size.y))};
            setObjectSizeAndCenter(car, car_start_pos, car->model.size);
            placeObject(b, car);
            int current_car_amount = b->cars.amount;

            moving_object_t moving_car;
            moving_car.object = *car;
            moving_car.direction = direction;
            moving_car.speed = 1;
            b->cars.objects[current_car_amount] = moving_car;
            b->cars.amount ++;
            cars_to_place--;
        }
        // Move to the next lane in a round-robin manner
        laneIndex = (laneIndex + 1) % (b->car_lane_amount);
    }
}

int main() {
    // Data that needs to be later taken in from a file
    const int SIZE = 50;
    const int STEP = 1;
    position_t MAX_MODEL_SIZE = {10,10};
    const int MAX_LANES = 4;
    const int AMOUNT_OF_CARS = 7;
    const char defaultArt[] = "default_input.txt";

    scoreboard_t scoreboard;
    scoreboard.score = 0;
    scoreboard.current_turn = 1;

    // Define the game board and allocate memory
    board_t board;
    board.size = SIZE;
    board.car_lane_amount = 0;
    board.cars.amount = 0;
    board.car_lanes = calloc(MAX_LANES, sizeof(int));
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
    }

    initializeMovingObjects(&board.cars, AMOUNT_OF_CARS);

    // Initialize objects
    object_t frog, lane, car_right, car_left;

    frog.name = "frog";
    frog.intent = 'f'; //frog
    position_t frogStartPos = {SIZE - (STEP / 2) * 2, SIZE / 2};
    position_t defaultFrogSize = {STEP, STEP};
    setObjectSizeAndCenter(&frog, frogStartPos, MAX_MODEL_SIZE);
    allocateArtMemory(&frog);
    readArtFromFile(defaultArt, &frog);

    lane.name = "lane";
    lane.intent = 'n'; //neutral
    position_t laneStartPos = {2 * STEP, SIZE / 2};
    setObjectSizeAndCenter(&lane, laneStartPos, MAX_MODEL_SIZE); // this might be not necessary
    allocateArtMemory(&lane);
    readArtFromFile(defaultArt, &lane);

    car_right.name = "car_right";
    car_right.intent = 'e'; //enemy
    position_t carRightStartPos = {2 * STEP + 2, SIZE / 3};
    setObjectSizeAndCenter(&car_right, carRightStartPos, MAX_MODEL_SIZE); // this might be not necessary
    allocateArtMemory(&car_right);
    readArtFromFile(defaultArt, &car_right);

    car_left.name = "car_left";
    car_left.intent = 'e'; //enemy
    position_t carLeftStartPos = {6 * STEP-1, SIZE / 3};
    setObjectSizeAndCenter(&car_left, carLeftStartPos, MAX_MODEL_SIZE); // this might be not necessary
    allocateArtMemory(&car_left);
    readArtFromFile(defaultArt, &car_left);

    // Allocate memory for model.art for each object

    readGameSettings("game_setup.txt", &scoreboard);

    // Uncommented critical game logic
    initializeBoard(&board);
    placeObject(&board, &frog);
    createMultipleLanes(&board, &lane, MAX_LANES);
    createCarsOnLanes(&board, &car_right, AMOUNT_OF_CARS/2, 'r');
    createCarsOnLanes(&board, &car_left, AMOUNT_OF_CARS/2 +(AMOUNT_OF_CARS%2), 'l');
    // placeObject(&board, &car_right);
    // placeObject(&board, &car_left);
    drawVisualLayer(&board);

    // Game loop (testing for 5 iterations)
    int repeat = 10;
    while (repeat > 0) {
        sleep(1);
        createMultipleLanes(&board, &lane, MAX_LANES);
        moveObject(&board, &frog, 'w');
        drawVisualLayer(&board);
        // drawGameplayLayer(&board);
        printScoreboard(&scoreboard);
        printcar_lanes(&board);
        for(int i = 0; i< board.cars.amount; i++){
            if( board.cars.objects[i].direction == 'r'){
                moveObject(&board, &board.cars.objects[i].object, 'd');
            }
            else {
                moveObject(&board, &board.cars.objects[i].object, 'a');
            }
        }
        repeat--;
    }

    // Free allocated memory
    freeArtMemory(&frog);
    freeArtMemory(&lane);
    freeArtMemory(&car_right);

    for (int i = 0; i < SIZE; i++) {
        free(board.visual_layer[i]);
        free(board.gameplay_layer[i]);
    }
    free(board.visual_layer);
    free(board.gameplay_layer);
    free(board.car_lanes);
    cleanupMovingObjects(&board.cars);

    return 0;
}


// next step is: 
// 1. store somehow where the lanes are, so possible positions of cars [ yup ]
// 2. let the cars pass on edges ( slowly let them disappear )
// 3. time them out before generating again
// 4. generate them on the same lane or other [ i guess ]
// 5. generate multiple cars and control it [ kinda ]
// 6. think about regulating car speed - frame separation?


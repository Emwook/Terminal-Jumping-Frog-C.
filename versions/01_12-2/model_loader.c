// model_loader.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "model_loader.h"

void loadModels(const char *filename, object_t **objects, int *object_count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d", object_count); // Read the number of models
    *objects = malloc((*object_count) * sizeof(object_t));
    if (!*objects) {
        perror("Memory allocation failed");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < *object_count; i++) {
        object_t *obj = &(*objects)[i];
        char buffer[256];

        // Read the name
        fscanf(file, "%s", buffer);
        obj->name = strdup(buffer);

        // Read size
        fscanf(file, "%d %d", &obj->size.x, &obj->size.y);

        // Allocate memory for art
        obj->art = malloc(obj->size.x * sizeof(char *));
        for (int j = 0; j < obj->size.x; j++) {
            obj->art[j] = malloc(obj->size.y * sizeof(char));
        }

        // Read the art
        fgetc(file); // Consume newline
        for (int j = 0; j < obj->size.x; j++) {
            fgets(buffer, sizeof(buffer), file);
            strncpy(obj->art[j], buffer, obj->size.y);
            obj->art[j][obj->size.y] = '\0'; // Null-terminate
        }
    }

    fclose(file);
}

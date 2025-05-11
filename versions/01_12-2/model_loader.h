#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include "object.h" // Include the file defining `object_t` if separate

void loadModels(const char *filename, object_t **objects, int *object_count);

#endif

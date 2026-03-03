#ifndef OBJECT_H
#define OBJECT_H

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

#endif

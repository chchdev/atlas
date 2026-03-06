#ifndef ATLAS_RUNTIME_H
#define ATLAS_RUNTIME_H

#include <stddef.h>

#include "atlas/ast.h"

typedef struct {
    char *name;
    double value;
} Variable;

typedef struct {
    Variable *items;
    size_t count;
    size_t capacity;
} Environment;

typedef struct {
    int had_error;
    char error_message[256];
} Runtime;

void env_init(Environment *env);
void env_free(Environment *env);
int env_define(Environment *env, const char *name, double value);
int env_assign(Environment *env, const char *name, double value);
int env_get(const Environment *env, const char *name, double *out_value);

void runtime_init(Runtime *runtime);
int runtime_execute_program(Runtime *runtime, Environment *env, const Program *program);

#endif

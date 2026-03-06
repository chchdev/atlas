#ifndef ATLAS_RUNTIME_H
#define ATLAS_RUNTIME_H

#include <stddef.h>

#include "atlas/ast.h"

typedef enum {
    VALUE_NUMBER,
    VALUE_BOOL,
    VALUE_TEXT,
    VALUE_OBJECT
} ValueType;

typedef struct AtlasObject AtlasObject;

typedef struct {
    ValueType type;
    union {
        double number;
        int boolean;
        char *text;
        AtlasObject *object;
    } as;
} Value;

typedef struct {
    char *name;
    Value value;
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

void value_init_number(Value *value, double number);
void value_init_bool(Value *value, int boolean);
void value_init_text(Value *value, const char *text);
void value_copy(Value *dest, const Value *src);
void value_free(Value *value);

void env_init(Environment *env);
void env_free(Environment *env);
int env_define(Environment *env, const char *name, const Value *value);
int env_assign(Environment *env, const char *name, const Value *value);
int env_get(const Environment *env, const char *name, Value *out_value);

void runtime_init(Runtime *runtime);
int runtime_execute_program(Runtime *runtime, Environment *env, const Program *program);

#endif

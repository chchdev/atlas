#include "atlas/runtime.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char *name;
    const Stmt *stmt;
} GlobeEntry;

typedef struct {
    char *name;
    const Stmt *stmt;
} MoldEntry;

struct AtlasObject {
    int refcount;
    char *mold_name;
    Variable *fields;
    size_t field_count;
};

static int runtime_error(Runtime *runtime, const char *message) {
    runtime->had_error = 1;
    snprintf(runtime->error_message, sizeof(runtime->error_message), "%s", message);
    return 0;
}

static int runtime_errorf(Runtime *runtime, const char *prefix, const char *name) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s '%s'.", prefix, name);
    return runtime_error(runtime, buffer);
}

static void object_release(AtlasObject *object);

void value_init_number(Value *value, double number) {
    value->type = VALUE_NUMBER;
    value->as.number = number;
}

void value_init_bool(Value *value, int boolean) {
    value->type = VALUE_BOOL;
    value->as.boolean = boolean ? 1 : 0;
}

void value_init_text(Value *value, const char *text) {
    value->type = VALUE_TEXT;
    value->as.text = NULL;
    if (text != NULL) {
        size_t len = strlen(text);
        value->as.text = (char *)malloc(len + 1);
        if (value->as.text != NULL) {
            memcpy(value->as.text, text, len + 1);
        }
    }
}

static void object_retain(AtlasObject *object) {
    if (object != NULL) {
        object->refcount++;
    }
}

void value_copy(Value *dest, const Value *src) {
    dest->type = src->type;
    switch (src->type) {
        case VALUE_NUMBER:
            dest->as.number = src->as.number;
            break;
        case VALUE_BOOL:
            dest->as.boolean = src->as.boolean;
            break;
        case VALUE_TEXT:
            value_init_text(dest, src->as.text != NULL ? src->as.text : "");
            break;
        case VALUE_OBJECT:
            dest->as.object = src->as.object;
            object_retain(dest->as.object);
            break;
        default:
            break;
    }
}

void value_free(Value *value) {
    switch (value->type) {
        case VALUE_TEXT:
            free(value->as.text);
            break;
        case VALUE_OBJECT:
            object_release(value->as.object);
            break;
        default:
            break;
    }
    value->type = VALUE_NUMBER;
    value->as.number = 0.0;
}

void env_init(Environment *env) {
    env->items = NULL;
    env->count = 0;
    env->capacity = 0;
}

static void env_clear(Environment *env) {
    size_t i;
    for (i = 0; i < env->count; ++i) {
        free(env->items[i].name);
        value_free(&env->items[i].value);
    }
    free(env->items);
}

void env_free(Environment *env) {
    env_clear(env);
    env->items = NULL;
    env->count = 0;
    env->capacity = 0;
}

static int env_find_index(const Environment *env, const char *name) {
    size_t i;
    for (i = 0; i < env->count; ++i) {
        if (strcmp(env->items[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

static char *dup_text(const char *s) {
    size_t len;
    char *copy;
    if (s == NULL) {
        return NULL;
    }
    len = strlen(s);
    copy = (char *)malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }
    memcpy(copy, s, len + 1);
    return copy;
}

int env_define(Environment *env, const char *name, const Value *value) {
    Value copy;

    if (env->count == env->capacity) {
        size_t new_capacity = env->capacity == 0 ? 8 : env->capacity * 2;
        Variable *next = (Variable *)realloc(env->items, new_capacity * sizeof(Variable));
        if (next == NULL) {
            return 0;
        }
        env->items = next;
        env->capacity = new_capacity;
    }

    env->items[env->count].name = dup_text(name);
    if (env->items[env->count].name == NULL) {
        return 0;
    }

    value_copy(&copy, value);
    env->items[env->count].value = copy;
    env->count++;
    return 1;
}

int env_assign(Environment *env, const char *name, const Value *value) {
    int idx = env_find_index(env, name);
    if (idx < 0) {
        return 0;
    }
    value_free(&env->items[idx].value);
    value_copy(&env->items[idx].value, value);
    return 1;
}

int env_get(const Environment *env, const char *name, Value *out_value) {
    int idx = env_find_index(env, name);
    if (idx < 0) {
        return 0;
    }
    value_copy(out_value, &env->items[idx].value);
    return 1;
}

static AtlasObject *object_new(const char *mold_name, Variable *fields, size_t field_count) {
    AtlasObject *obj = (AtlasObject *)calloc(1, sizeof(AtlasObject));
    if (obj == NULL) {
        return NULL;
    }
    obj->refcount = 1;
    obj->mold_name = dup_text(mold_name);
    if (obj->mold_name == NULL) {
        free(obj);
        return NULL;
    }
    obj->fields = fields;
    obj->field_count = field_count;
    return obj;
}

static void object_release(AtlasObject *object) {
    size_t i;
    if (object == NULL) {
        return;
    }
    object->refcount--;
    if (object->refcount > 0) {
        return;
    }
    for (i = 0; i < object->field_count; ++i) {
        free(object->fields[i].name);
        value_free(&object->fields[i].value);
    }
    free(object->fields);
    free(object->mold_name);
    free(object);
}

static const char *value_type_name(const Value *value) {
    switch (value->type) {
        case VALUE_NUMBER:
            return "number";
        case VALUE_BOOL:
            return "boolean";
        case VALUE_TEXT:
            return "text";
        case VALUE_OBJECT:
            return "object";
        default:
            return "unknown";
    }
}

static int value_to_number(const Value *value, double *out) {
    switch (value->type) {
        case VALUE_NUMBER:
            *out = value->as.number;
            return 1;
        case VALUE_BOOL:
            *out = value->as.boolean ? 1.0 : 0.0;
            return 1;
        case VALUE_TEXT: {
            char *end = NULL;
            double n = strtod(value->as.text != NULL ? value->as.text : "", &end);
            if (end == NULL || *end != '\0') {
                return 0;
            }
            *out = n;
            return 1;
        }
        default:
            return 0;
    }
}

static int value_to_bool(const Value *value, int *out) {
    switch (value->type) {
        case VALUE_BOOL:
            *out = value->as.boolean ? 1 : 0;
            return 1;
        case VALUE_NUMBER:
            *out = fabs(value->as.number) > 1e-12 ? 1 : 0;
            return 1;
        case VALUE_TEXT:
            *out = value->as.text != NULL && value->as.text[0] != '\0';
            return 1;
        case VALUE_OBJECT:
            *out = 1;
            return 1;
        default:
            return 0;
    }
}

static int value_to_text(const Value *value, char *buffer, size_t buffer_size) {
    switch (value->type) {
        case VALUE_NUMBER:
            snprintf(buffer, buffer_size, "%.15g", value->as.number);
            return 1;
        case VALUE_BOOL:
            snprintf(buffer, buffer_size, "%s", value->as.boolean ? "true" : "false");
            return 1;
        case VALUE_TEXT:
            snprintf(buffer, buffer_size, "%s", value->as.text != NULL ? value->as.text : "");
            return 1;
        case VALUE_OBJECT:
            snprintf(buffer, buffer_size, "<%s object>", value->as.object->mold_name);
            return 1;
        default:
            return 0;
    }
}

static const GlobeEntry *find_globe(const GlobeEntry *globes, size_t count, const char *name) {
    size_t i;
    for (i = 0; i < count; ++i) {
        if (strcmp(globes[i].name, name) == 0) {
            return &globes[i];
        }
    }
    return NULL;
}

static const MoldEntry *find_mold(const MoldEntry *molds, size_t count, const char *name) {
    size_t i;
    for (i = 0; i < count; ++i) {
        if (strcmp(molds[i].name, name) == 0) {
            return &molds[i];
        }
    }
    return NULL;
}

static int eval_expr(
    Runtime *runtime,
    Environment *env,
    const GlobeEntry *globes,
    size_t globe_count,
    const MoldEntry *molds,
    size_t mold_count,
    const Expr *expr,
    Value *result
);

static int execute_ignite(
    Runtime *runtime,
    Environment *caller_env,
    const GlobeEntry *globes,
    size_t globe_count,
    const MoldEntry *molds,
    size_t mold_count,
    const char *name,
    const ExprList *args,
    int depth,
    Value *out_result
);

static int eval_builtin(
    Runtime *runtime,
    Environment *env,
    const GlobeEntry *globes,
    size_t globe_count,
    const MoldEntry *molds,
    size_t mold_count,
    const Expr *expr,
    Value *result
) {
    const char *name = expr->as.call.name;

    if (strcmp(name, "kind") == 0) {
        Value arg;
        if (expr->as.call.args.count != 1) {
            return runtime_error(runtime, "kind(...) expects exactly one argument.");
        }
        if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.call.args.items[0], &arg)) {
            return 0;
        }
        value_init_text(result, value_type_name(&arg));
        value_free(&arg);
        return 1;
    }

    if (strcmp(name, "as_number") == 0) {
        Value arg;
        double n;
        if (expr->as.call.args.count != 1) {
            return runtime_error(runtime, "as_number(...) expects exactly one argument.");
        }
        if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.call.args.items[0], &arg)) {
            return 0;
        }
        if (!value_to_number(&arg, &n)) {
            value_free(&arg);
            return runtime_error(runtime, "Cannot convert value to number.");
        }
        value_free(&arg);
        value_init_number(result, n);
        return 1;
    }

    if (strcmp(name, "as_bool") == 0) {
        Value arg;
        int b;
        if (expr->as.call.args.count != 1) {
            return runtime_error(runtime, "as_bool(...) expects exactly one argument.");
        }
        if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.call.args.items[0], &arg)) {
            return 0;
        }
        if (!value_to_bool(&arg, &b)) {
            value_free(&arg);
            return runtime_error(runtime, "Cannot convert value to boolean.");
        }
        value_free(&arg);
        value_init_bool(result, b);
        return 1;
    }

    if (strcmp(name, "as_text") == 0) {
        Value arg;
        char buffer[256];
        if (expr->as.call.args.count != 1) {
            return runtime_error(runtime, "as_text(...) expects exactly one argument.");
        }
        if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.call.args.items[0], &arg)) {
            return 0;
        }
        if (!value_to_text(&arg, buffer, sizeof(buffer))) {
            value_free(&arg);
            return runtime_error(runtime, "Cannot convert value to text.");
        }
        value_free(&arg);
        value_init_text(result, buffer);
        return 1;
    }

    return runtime_errorf(runtime, "Unknown builtin function", name);
}

static int build_object_from_mold(
    Runtime *runtime,
    Environment *env,
    const GlobeEntry *globes,
    size_t globe_count,
    const MoldEntry *molds,
    size_t mold_count,
    const char *mold_name,
    Value *result
) {
    const MoldEntry *mold = find_mold(molds, mold_count, mold_name);
    Variable *fields = NULL;
    size_t field_count;
    size_t i;

    if (mold == NULL) {
        return runtime_errorf(runtime, "Unknown mold", mold_name);
    }

    field_count = mold->stmt->as.mold_stmt.fields.count;
    if (field_count > 0) {
        fields = (Variable *)calloc(field_count, sizeof(Variable));
        if (fields == NULL) {
            return runtime_error(runtime, "Out of memory while crafting object.");
        }
    }

    for (i = 0; i < field_count; ++i) {
        const FieldInit *field = &mold->stmt->as.mold_stmt.fields.items[i];
        Value field_value;

        fields[i].name = dup_text(field->name);
        if (fields[i].name == NULL) {
            size_t j;
            for (j = 0; j < i; ++j) {
                free(fields[j].name);
                value_free(&fields[j].value);
            }
            free(fields);
            return runtime_error(runtime, "Out of memory while crafting object fields.");
        }

        if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, field->value, &field_value)) {
            size_t j;
            free(fields[i].name);
            for (j = 0; j < i; ++j) {
                free(fields[j].name);
                value_free(&fields[j].value);
            }
            free(fields);
            return 0;
        }

        fields[i].value = field_value;
    }

    {
        AtlasObject *obj = object_new(mold_name, fields, field_count);
        if (obj == NULL) {
            for (i = 0; i < field_count; ++i) {
                free(fields[i].name);
                value_free(&fields[i].value);
            }
            free(fields);
            return runtime_error(runtime, "Out of memory while finalizing crafted object.");
        }
        result->type = VALUE_OBJECT;
        result->as.object = obj;
    }

    return 1;
}

static int eval_expr(
    Runtime *runtime,
    Environment *env,
    const GlobeEntry *globes,
    size_t globe_count,
    const MoldEntry *molds,
    size_t mold_count,
    const Expr *expr,
    Value *result
) {
    switch (expr->type) {
        case EXPR_NUMBER:
            value_init_number(result, expr->as.number);
            return 1;

        case EXPR_BOOL:
            value_init_bool(result, expr->as.boolean);
            return 1;

        case EXPR_VARIABLE:
            if (env == NULL || !env_get(env, expr->as.name, result)) {
                return runtime_errorf(runtime, "Unknown seed", expr->as.name);
            }
            return 1;

        case EXPR_FIELD: {
            Value object_value;
            size_t i;
            if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.field.object, &object_value)) {
                return 0;
            }
            if (object_value.type != VALUE_OBJECT) {
                value_free(&object_value);
                return runtime_error(runtime, "Field access requires an object value.");
            }
            for (i = 0; i < object_value.as.object->field_count; ++i) {
                if (strcmp(object_value.as.object->fields[i].name, expr->as.field.field) == 0) {
                    value_copy(result, &object_value.as.object->fields[i].value);
                    value_free(&object_value);
                    return 1;
                }
            }
            value_free(&object_value);
            return runtime_errorf(runtime, "Unknown object field", expr->as.field.field);
        }

        case EXPR_CRAFT:
            return build_object_from_mold(runtime, env, globes, globe_count, molds, mold_count, expr->as.craft.mold_name, result);

        case EXPR_UNARY: {
            Value right;
            double n;
            if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.unary.right, &right)) {
                return 0;
            }
            if (expr->as.unary.op != '-' || !value_to_number(&right, &n)) {
                value_free(&right);
                return runtime_error(runtime, "Unary '-' requires a numeric-compatible value.");
            }
            value_free(&right);
            value_init_number(result, -n);
            return 1;
        }

        case EXPR_BINARY: {
            Value left;
            Value right;
            double a;
            double b;
            if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.binary.left, &left)) {
                return 0;
            }
            if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, expr->as.binary.right, &right)) {
                value_free(&left);
                return 0;
            }
            if (!value_to_number(&left, &a) || !value_to_number(&right, &b)) {
                value_free(&left);
                value_free(&right);
                return runtime_error(runtime, "Binary arithmetic requires numeric-compatible values.");
            }
            value_free(&left);
            value_free(&right);
            switch (expr->as.binary.op) {
                case '+':
                    value_init_number(result, a + b);
                    return 1;
                case '-':
                    value_init_number(result, a - b);
                    return 1;
                case '*':
                    value_init_number(result, a * b);
                    return 1;
                case '/':
                    if (fabs(b) < 1e-12) {
                        return runtime_error(runtime, "Division by zero.");
                    }
                    value_init_number(result, a / b);
                    return 1;
                default:
                    return runtime_error(runtime, "Unsupported binary operator.");
            }
        }

        case EXPR_CALL:
            if (
                strcmp(expr->as.call.name, "kind") == 0 || strcmp(expr->as.call.name, "as_number") == 0 ||
                strcmp(expr->as.call.name, "as_bool") == 0 || strcmp(expr->as.call.name, "as_text") == 0
            ) {
                return eval_builtin(runtime, env, globes, globe_count, molds, mold_count, expr, result);
            }

            return execute_ignite(
                runtime,
                env,
                globes,
                globe_count,
                molds,
                mold_count,
                expr->as.call.name,
                &expr->as.call.args,
                0,
                result
            );

        default:
            return runtime_error(runtime, "Unknown expression type.");
    }
}

static int assign_field(Runtime *runtime, Environment *env, const char *object_name, const char *field_name, const Value *value) {
    Value object_value;
    size_t i;

    if (!env_get(env, object_name, &object_value)) {
        return runtime_errorf(runtime, "Unknown object seed", object_name);
    }

    if (object_value.type != VALUE_OBJECT) {
        value_free(&object_value);
        return runtime_error(runtime, "Field assignment target must be an object.");
    }

    for (i = 0; i < object_value.as.object->field_count; ++i) {
        if (strcmp(object_value.as.object->fields[i].name, field_name) == 0) {
            value_free(&object_value.as.object->fields[i].value);
            value_copy(&object_value.as.object->fields[i].value, value);
            value_free(&object_value);
            return 1;
        }
    }

    value_free(&object_value);
    return runtime_errorf(runtime, "Unknown object field", field_name);
}

typedef enum {
    FLOW_NONE,
    FLOW_RETURN,
    FLOW_BREAK,
    FLOW_CONTINUE
} FlowSignal;

static int execute_block(
    Runtime *runtime,
    const GlobeEntry *globes,
    size_t globe_count,
    const MoldEntry *molds,
    size_t mold_count,
    const Block *block,
    Environment *env,
    int depth,
    FlowSignal *flow_signal,
    Value *return_value
) {
    size_t i;

    for (i = 0; i < block->count; ++i) {
        const Stmt *stmt = block->items[i];

        switch (stmt->type) {
            case STMT_SEED: {
                Value value;
                if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, stmt->as.seed_stmt.value, &value)) {
                    return 0;
                }
                if (!env_define(env, stmt->as.seed_stmt.name, &value)) {
                    value_free(&value);
                    return runtime_error(runtime, "Failed to create seed.");
                }
                value_free(&value);
                break;
            }

            case STMT_ASSIGN: {
                Value value;
                if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, stmt->as.assign_stmt.value, &value)) {
                    return 0;
                }
                if (!env_assign(env, stmt->as.assign_stmt.name, &value)) {
                    value_free(&value);
                    return runtime_errorf(runtime, "Cannot rebind unknown seed", stmt->as.assign_stmt.name);
                }
                value_free(&value);
                break;
            }

            case STMT_FIELD_ASSIGN: {
                Value value;
                if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, stmt->as.field_assign_stmt.value, &value)) {
                    return 0;
                }
                if (!assign_field(runtime, env, stmt->as.field_assign_stmt.object_name, stmt->as.field_assign_stmt.field_name, &value)) {
                    value_free(&value);
                    return 0;
                }
                value_free(&value);
                break;
            }

            case STMT_ECHO: {
                Value value;
                char text[256];
                if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, stmt->as.echo_stmt.value, &value)) {
                    return 0;
                }
                if (!value_to_text(&value, text, sizeof(text))) {
                    value_free(&value);
                    return runtime_error(runtime, "Cannot render value for echo.");
                }
                printf("%s\n", text);
                value_free(&value);
                break;
            }

            case STMT_EXPR: {
                Value value;
                if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, stmt->as.expr_stmt.value, &value)) {
                    return 0;
                }
                value_free(&value);
                break;
            }

            case STMT_IGNITE: {
                Value ignored;
                if (!execute_ignite(
                        runtime,
                        env,
                        globes,
                        globe_count,
                        molds,
                        mold_count,
                        stmt->as.ignite_stmt.name,
                        &stmt->as.ignite_stmt.args,
                        depth,
                        &ignored
                    )) {
                    return 0;
                }
                value_free(&ignored);
                break;
            }

            case STMT_RETURN: {
                Value value;
                if (!eval_expr(runtime, env, globes, globe_count, molds, mold_count, stmt->as.return_stmt.value, &value)) {
                    return 0;
                }
                *flow_signal = FLOW_RETURN;
                *return_value = value;
                return 1;
            }

            case STMT_ORBIT: {
                size_t safety = 0;
                for (;;) {
                    Value cond_value;
                    int cond_bool = 0;
                    FlowSignal inner_flow = FLOW_NONE;

                    if (!eval_expr(
                            runtime,
                            env,
                            globes,
                            globe_count,
                            molds,
                            mold_count,
                            stmt->as.orbit_stmt.condition,
                            &cond_value
                        )) {
                        return 0;
                    }

                    if (!value_to_bool(&cond_value, &cond_bool)) {
                        value_free(&cond_value);
                        return runtime_error(runtime, "Orbit condition could not be converted to boolean.");
                    }

                    value_free(&cond_value);
                    if (!cond_bool) {
                        break;
                    }

                    if (!execute_block(
                            runtime,
                            globes,
                            globe_count,
                            molds,
                            mold_count,
                            &stmt->as.orbit_stmt.body,
                            env,
                            depth + 1,
                            &inner_flow,
                            return_value
                        )) {
                        return 0;
                    }

                    if (inner_flow == FLOW_RETURN) {
                        *flow_signal = FLOW_RETURN;
                        return 1;
                    }

                    if (inner_flow == FLOW_BREAK) {
                        break;
                    }

                    if (inner_flow == FLOW_CONTINUE) {
                        safety++;
                        if (safety > 1000000) {
                            return runtime_error(runtime, "Orbit loop safety limit reached.");
                        }
                        continue;
                    }

                    safety++;
                    if (safety > 1000000) {
                        return runtime_error(runtime, "Orbit loop safety limit reached.");
                    }
                }
                break;
            }

            case STMT_BREAK:
                *flow_signal = FLOW_BREAK;
                return 1;

            case STMT_CONTINUE:
                *flow_signal = FLOW_CONTINUE;
                return 1;

            default:
                return runtime_error(runtime, "Unsupported statement inside globe.");
        }
    }

    return 1;
}

static int execute_ignite(
    Runtime *runtime,
    Environment *caller_env,
    const GlobeEntry *globes,
    size_t globe_count,
    const MoldEntry *molds,
    size_t mold_count,
    const char *name,
    const ExprList *args,
    int depth,
    Value *out_result
) {
    const GlobeEntry *globe = find_globe(globes, globe_count, name);
    Environment local_env;
    size_t i;
    FlowSignal flow_signal = FLOW_NONE;
    Value return_value;

    if (depth > 64) {
        return runtime_error(runtime, "Globe recursion limit reached.");
    }

    if (globe == NULL) {
        return runtime_errorf(runtime, "No globe named", name);
    }

    if (globe->stmt->as.globe_stmt.params.count != args->count) {
        return runtime_error(runtime, "Ignite argument count does not match globe parameter count.");
    }

    env_init(&local_env);
    value_init_number(&return_value, 0.0);

    for (i = 0; i < args->count; ++i) {
        Value value;
        const char *param_name = globe->stmt->as.globe_stmt.params.items[i];
        if (!eval_expr(runtime, caller_env, globes, globe_count, molds, mold_count, args->items[i], &value)) {
            env_free(&local_env);
            value_free(&return_value);
            return 0;
        }
        if (!env_define(&local_env, param_name, &value)) {
            value_free(&value);
            env_free(&local_env);
            value_free(&return_value);
            return runtime_error(runtime, "Out of memory while binding globe parameter.");
        }
        value_free(&value);
    }

    if (!execute_block(
            runtime,
            globes,
            globe_count,
            molds,
            mold_count,
            &globe->stmt->as.globe_stmt.body,
            &local_env,
            depth + 1,
            &flow_signal,
            &return_value
        )) {
        env_free(&local_env);
        value_free(&return_value);
        return 0;
    }

    env_free(&local_env);

    if (flow_signal == FLOW_BREAK || flow_signal == FLOW_CONTINUE) {
        value_free(&return_value);
        return runtime_error(runtime, "break/continue can only be used inside orbit loops.");
    }

    if (flow_signal == FLOW_RETURN) {
        *out_result = return_value;
    } else {
        value_free(&return_value);
        value_init_number(out_result, 0.0);
    }

    return 1;
}

void runtime_init(Runtime *runtime) {
    runtime->had_error = 0;
    runtime->error_message[0] = '\0';
}

int runtime_execute_program(Runtime *runtime, Environment *env, const Program *program) {
    GlobeEntry *globes = NULL;
    MoldEntry *molds = NULL;
    size_t globe_count = 0;
    size_t globe_capacity = 0;
    size_t mold_count = 0;
    size_t mold_capacity = 0;
    size_t i;

    for (i = 0; i < program->count; ++i) {
        const Stmt *stmt = program->items[i];

        if (stmt->type == STMT_GLOBE) {
            if (find_globe(globes, globe_count, stmt->as.globe_stmt.name) != NULL) {
                free(globes);
                free(molds);
                return runtime_errorf(runtime, "Duplicate globe", stmt->as.globe_stmt.name);
            }
            if (globe_count == globe_capacity) {
                size_t new_capacity = globe_capacity == 0 ? 8 : globe_capacity * 2;
                GlobeEntry *next = (GlobeEntry *)realloc(globes, new_capacity * sizeof(GlobeEntry));
                if (next == NULL) {
                    free(globes);
                    free(molds);
                    return runtime_error(runtime, "Out of memory while storing globes.");
                }
                globes = next;
                globe_capacity = new_capacity;
            }
            globes[globe_count].name = stmt->as.globe_stmt.name;
            globes[globe_count].stmt = stmt;
            globe_count++;
        } else if (stmt->type == STMT_MOLD) {
            if (find_mold(molds, mold_count, stmt->as.mold_stmt.name) != NULL) {
                free(globes);
                free(molds);
                return runtime_errorf(runtime, "Duplicate mold", stmt->as.mold_stmt.name);
            }
            if (mold_count == mold_capacity) {
                size_t new_capacity = mold_capacity == 0 ? 8 : mold_capacity * 2;
                MoldEntry *next = (MoldEntry *)realloc(molds, new_capacity * sizeof(MoldEntry));
                if (next == NULL) {
                    free(globes);
                    free(molds);
                    return runtime_error(runtime, "Out of memory while storing molds.");
                }
                molds = next;
                mold_capacity = new_capacity;
            }
            molds[mold_count].name = stmt->as.mold_stmt.name;
            molds[mold_count].stmt = stmt;
            mold_count++;
        }
    }

    for (i = 0; i < program->count; ++i) {
        const Stmt *stmt = program->items[i];
        if (stmt->type == STMT_IGNITE) {
            Value ignored;
            if (!execute_ignite(
                    runtime,
                    env,
                    globes,
                    globe_count,
                    molds,
                    mold_count,
                    stmt->as.ignite_stmt.name,
                    &stmt->as.ignite_stmt.args,
                    0,
                    &ignored
                )) {
                free(globes);
                free(molds);
                return 0;
            }
            value_free(&ignored);
        }
    }

    free(globes);
    free(molds);
    return 1;
}

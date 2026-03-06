#include "atlas/runtime.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atlas/util.h"

static int runtime_error(Runtime *runtime, const char *message) {
    runtime->had_error = 1;
    snprintf(runtime->error_message, sizeof(runtime->error_message), "%s", message);
    return 0;
}

void env_init(Environment *env) {
    env->items = NULL;
    env->count = 0;
    env->capacity = 0;
}

void env_free(Environment *env) {
    size_t i;
    for (i = 0; i < env->count; ++i) {
        free(env->items[i].name);
    }
    free(env->items);
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

int env_define(Environment *env, const char *name, double value) {
    int idx = env_find_index(env, name);
    if (idx >= 0) {
        env->items[idx].value = value;
        return 1;
    }

    if (env->count == env->capacity) {
        size_t new_capacity = env->capacity == 0 ? 8 : env->capacity * 2;
        Variable *new_items = (Variable *)realloc(env->items, new_capacity * sizeof(Variable));
        if (new_items == NULL) {
            return 0;
        }
        env->items = new_items;
        env->capacity = new_capacity;
    }

    env->items[env->count].name = atlas_strdup(name);
    if (env->items[env->count].name == NULL) {
        return 0;
    }
    env->items[env->count].value = value;
    env->count++;
    return 1;
}

int env_assign(Environment *env, const char *name, double value) {
    int idx = env_find_index(env, name);
    if (idx < 0) {
        return 0;
    }
    env->items[idx].value = value;
    return 1;
}

int env_get(const Environment *env, const char *name, double *out_value) {
    int idx = env_find_index(env, name);
    if (idx < 0) {
        return 0;
    }
    *out_value = env->items[idx].value;
    return 1;
}

void runtime_init(Runtime *runtime) {
    runtime->had_error = 0;
    runtime->error_message[0] = '\0';
}

static int eval_expr(Runtime *runtime, Environment *env, const Expr *expr, double *result) {
    if (expr == NULL) {
        return runtime_error(runtime, "Internal error: null expression.");
    }

    switch (expr->type) {
        case EXPR_NUMBER:
            *result = expr->as.number;
            return 1;

        case EXPR_VARIABLE:
            if (!env_get(env, expr->as.name, result)) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer), "Undefined variable '%s'.", expr->as.name);
                return runtime_error(runtime, buffer);
            }
            return 1;

        case EXPR_UNARY: {
            double right;
            if (!eval_expr(runtime, env, expr->as.unary.right, &right)) {
                return 0;
            }
            switch (expr->as.unary.op) {
                case '-':
                    *result = -right;
                    return 1;
                default:
                    return runtime_error(runtime, "Unsupported unary operator.");
            }
        }

        case EXPR_BINARY: {
            double left;
            double right;
            if (!eval_expr(runtime, env, expr->as.binary.left, &left)) {
                return 0;
            }
            if (!eval_expr(runtime, env, expr->as.binary.right, &right)) {
                return 0;
            }
            switch (expr->as.binary.op) {
                case '+':
                    *result = left + right;
                    return 1;
                case '-':
                    *result = left - right;
                    return 1;
                case '*':
                    *result = left * right;
                    return 1;
                case '/':
                    if (fabs(right) < 1e-12) {
                        return runtime_error(runtime, "Division by zero.");
                    }
                    *result = left / right;
                    return 1;
                default:
                    return runtime_error(runtime, "Unsupported binary operator.");
            }
        }

        default:
            return runtime_error(runtime, "Unknown expression type.");
    }
}

int runtime_execute_program(Runtime *runtime, Environment *env, const Program *program) {
    size_t i;
    for (i = 0; i < program->count; ++i) {
        const Stmt *stmt = program->items[i];
        double value = 0.0;

        switch (stmt->type) {
            case STMT_LET:
                if (!eval_expr(runtime, env, stmt->as.let_stmt.value, &value)) {
                    return 0;
                }
                if (!env_define(env, stmt->as.let_stmt.name, value)) {
                    return runtime_error(runtime, "Failed to define variable.");
                }
                break;

            case STMT_ASSIGN:
                if (!eval_expr(runtime, env, stmt->as.assign_stmt.value, &value)) {
                    return 0;
                }
                if (!env_assign(env, stmt->as.assign_stmt.name, value)) {
                    char buffer[256];
                    snprintf(
                        buffer,
                        sizeof(buffer),
                        "Cannot assign to undefined variable '%s'.",
                        stmt->as.assign_stmt.name
                    );
                    return runtime_error(runtime, buffer);
                }
                break;

            case STMT_PRINT:
                if (!eval_expr(runtime, env, stmt->as.print_stmt.value, &value)) {
                    return 0;
                }
                printf("%.15g\n", value);
                break;

            case STMT_EXPR:
                if (!eval_expr(runtime, env, stmt->as.expr_stmt.value, &value)) {
                    return 0;
                }
                break;

            default:
                return runtime_error(runtime, "Unknown statement type.");
        }
    }

    return 1;
}

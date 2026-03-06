#include "atlas/ast.h"

#include <stdlib.h>

#include "atlas/util.h"

static void *grow_array(void *ptr, size_t *capacity, size_t item_size) {
    size_t new_capacity = *capacity == 0 ? 8 : (*capacity * 2);
    void *next = realloc(ptr, new_capacity * item_size);
    if (next == NULL) {
        return NULL;
    }
    *capacity = new_capacity;
    return next;
}

void expr_list_init(ExprList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int expr_list_push(ExprList *list, Expr *expr) {
    if (list->count == list->capacity) {
        Expr **next = (Expr **)grow_array(list->items, &list->capacity, sizeof(Expr *));
        if (next == NULL) {
            return 0;
        }
        list->items = next;
    }
    list->items[list->count++] = expr;
    return 1;
}

void expr_list_free(ExprList *list) {
    size_t i;
    for (i = 0; i < list->count; ++i) {
        expr_free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void name_list_init(NameList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int name_list_push(NameList *list, const char *name) {
    char *copy;
    if (list->count == list->capacity) {
        char **next = (char **)grow_array(list->items, &list->capacity, sizeof(char *));
        if (next == NULL) {
            return 0;
        }
        list->items = next;
    }
    copy = atlas_strdup(name);
    if (copy == NULL) {
        return 0;
    }
    list->items[list->count++] = copy;
    return 1;
}

void name_list_free(NameList *list) {
    size_t i;
    for (i = 0; i < list->count; ++i) {
        free(list->items[i]);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

void field_init_list_init(FieldInitList *list) {
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

int field_init_list_push(FieldInitList *list, const char *name, Expr *value) {
    FieldInit *next_items;
    char *copy;

    if (list->count == list->capacity) {
        next_items = (FieldInit *)grow_array(list->items, &list->capacity, sizeof(FieldInit));
        if (next_items == NULL) {
            return 0;
        }
        list->items = next_items;
    }

    copy = atlas_strdup(name);
    if (copy == NULL) {
        return 0;
    }

    list->items[list->count].name = copy;
    list->items[list->count].value = value;
    list->count++;
    return 1;
}

void field_init_list_free(FieldInitList *list) {
    size_t i;
    for (i = 0; i < list->count; ++i) {
        free(list->items[i].name);
        expr_free(list->items[i].value);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    list->capacity = 0;
}

Expr *expr_number_new(double value) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_NUMBER;
    expr->as.number = value;
    return expr;
}

Expr *expr_bool_new(int value) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_BOOL;
    expr->as.boolean = value ? 1 : 0;
    return expr;
}

Expr *expr_variable_new(const char *name) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_VARIABLE;
    expr->as.name = atlas_strdup(name);
    if (expr->as.name == NULL) {
        free(expr);
        return NULL;
    }
    return expr;
}

Expr *expr_binary_new(char op, Expr *left, Expr *right) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_BINARY;
    expr->as.binary.op = op;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    return expr;
}

Expr *expr_unary_new(char op, Expr *right) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_UNARY;
    expr->as.unary.op = op;
    expr->as.unary.right = right;
    return expr;
}

Expr *expr_call_new(const char *name, ExprList *args) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_CALL;
    expr->as.call.name = atlas_strdup(name);
    if (expr->as.call.name == NULL) {
        free(expr);
        return NULL;
    }
    expr->as.call.args = *args;
    args->items = NULL;
    args->count = 0;
    args->capacity = 0;
    return expr;
}

Expr *expr_field_new(Expr *object, const char *field) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_FIELD;
    expr->as.field.object = object;
    expr->as.field.field = atlas_strdup(field);
    if (expr->as.field.field == NULL) {
        expr_free(object);
        free(expr);
        return NULL;
    }
    return expr;
}

Expr *expr_craft_new(const char *mold_name) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_CRAFT;
    expr->as.craft.mold_name = atlas_strdup(mold_name);
    if (expr->as.craft.mold_name == NULL) {
        free(expr);
        return NULL;
    }
    return expr;
}

void expr_free(Expr *expr) {
    size_t i;

    if (expr == NULL) {
        return;
    }

    switch (expr->type) {
        case EXPR_VARIABLE:
            free(expr->as.name);
            break;
        case EXPR_BINARY:
            expr_free(expr->as.binary.left);
            expr_free(expr->as.binary.right);
            break;
        case EXPR_UNARY:
            expr_free(expr->as.unary.right);
            break;
        case EXPR_CALL:
            free(expr->as.call.name);
            for (i = 0; i < expr->as.call.args.count; ++i) {
                expr_free(expr->as.call.args.items[i]);
            }
            free(expr->as.call.args.items);
            break;
        case EXPR_FIELD:
            expr_free(expr->as.field.object);
            free(expr->as.field.field);
            break;
        case EXPR_CRAFT:
            free(expr->as.craft.mold_name);
            break;
        case EXPR_NUMBER:
        case EXPR_BOOL:
        default:
            break;
    }

    free(expr);
}

void block_init(Block *block) {
    block->items = NULL;
    block->count = 0;
    block->capacity = 0;
}

int block_push(Block *block, Stmt *stmt) {
    if (block->count == block->capacity) {
        Stmt **next = (Stmt **)grow_array(block->items, &block->capacity, sizeof(Stmt *));
        if (next == NULL) {
            return 0;
        }
        block->items = next;
    }
    block->items[block->count++] = stmt;
    return 1;
}

void block_free(Block *block) {
    size_t i;
    for (i = 0; i < block->count; ++i) {
        stmt_free(block->items[i]);
    }
    free(block->items);
    block->items = NULL;
    block->count = 0;
    block->capacity = 0;
}

Stmt *stmt_globe_new(const char *name, NameList *params, Block *body) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }

    stmt->type = STMT_GLOBE;
    stmt->as.globe_stmt.name = atlas_strdup(name);
    if (stmt->as.globe_stmt.name == NULL) {
        free(stmt);
        return NULL;
    }

    stmt->as.globe_stmt.params = *params;
    params->items = NULL;
    params->count = 0;
    params->capacity = 0;

    stmt->as.globe_stmt.body = *body;
    body->items = NULL;
    body->count = 0;
    body->capacity = 0;

    return stmt;
}

Stmt *stmt_mold_new(const char *name, FieldInitList *fields) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }

    stmt->type = STMT_MOLD;
    stmt->as.mold_stmt.name = atlas_strdup(name);
    if (stmt->as.mold_stmt.name == NULL) {
        free(stmt);
        return NULL;
    }

    stmt->as.mold_stmt.fields = *fields;
    fields->items = NULL;
    fields->count = 0;
    fields->capacity = 0;

    return stmt;
}

Stmt *stmt_ignite_new(const char *name, ExprList *args) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }

    stmt->type = STMT_IGNITE;
    stmt->as.ignite_stmt.name = atlas_strdup(name);
    if (stmt->as.ignite_stmt.name == NULL) {
        free(stmt);
        return NULL;
    }

    stmt->as.ignite_stmt.args = *args;
    args->items = NULL;
    args->count = 0;
    args->capacity = 0;

    return stmt;
}

Stmt *stmt_seed_new(const char *name, Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }
    stmt->type = STMT_SEED;
    stmt->as.seed_stmt.name = atlas_strdup(name);
    stmt->as.seed_stmt.value = value;
    if (stmt->as.seed_stmt.name == NULL) {
        expr_free(value);
        free(stmt);
        return NULL;
    }
    return stmt;
}

Stmt *stmt_assign_new(const char *name, Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }
    stmt->type = STMT_ASSIGN;
    stmt->as.assign_stmt.name = atlas_strdup(name);
    stmt->as.assign_stmt.value = value;
    if (stmt->as.assign_stmt.name == NULL) {
        expr_free(value);
        free(stmt);
        return NULL;
    }
    return stmt;
}

Stmt *stmt_field_assign_new(const char *object_name, const char *field_name, Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }

    stmt->type = STMT_FIELD_ASSIGN;
    stmt->as.field_assign_stmt.object_name = atlas_strdup(object_name);
    stmt->as.field_assign_stmt.field_name = atlas_strdup(field_name);
    stmt->as.field_assign_stmt.value = value;

    if (stmt->as.field_assign_stmt.object_name == NULL || stmt->as.field_assign_stmt.field_name == NULL) {
        free(stmt->as.field_assign_stmt.object_name);
        free(stmt->as.field_assign_stmt.field_name);
        expr_free(value);
        free(stmt);
        return NULL;
    }

    return stmt;
}

Stmt *stmt_echo_new(Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }
    stmt->type = STMT_ECHO;
    stmt->as.echo_stmt.value = value;
    return stmt;
}

Stmt *stmt_expr_new(Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }
    stmt->type = STMT_EXPR;
    stmt->as.expr_stmt.value = value;
    return stmt;
}

Stmt *stmt_return_new(Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }
    stmt->type = STMT_RETURN;
    stmt->as.return_stmt.value = value;
    return stmt;
}

void stmt_free(Stmt *stmt) {
    if (stmt == NULL) {
        return;
    }

    switch (stmt->type) {
        case STMT_GLOBE:
            free(stmt->as.globe_stmt.name);
            name_list_free(&stmt->as.globe_stmt.params);
            block_free(&stmt->as.globe_stmt.body);
            break;
        case STMT_MOLD:
            free(stmt->as.mold_stmt.name);
            field_init_list_free(&stmt->as.mold_stmt.fields);
            break;
        case STMT_IGNITE:
            free(stmt->as.ignite_stmt.name);
            expr_list_free(&stmt->as.ignite_stmt.args);
            break;
        case STMT_SEED:
            free(stmt->as.seed_stmt.name);
            expr_free(stmt->as.seed_stmt.value);
            break;
        case STMT_ASSIGN:
            free(stmt->as.assign_stmt.name);
            expr_free(stmt->as.assign_stmt.value);
            break;
        case STMT_FIELD_ASSIGN:
            free(stmt->as.field_assign_stmt.object_name);
            free(stmt->as.field_assign_stmt.field_name);
            expr_free(stmt->as.field_assign_stmt.value);
            break;
        case STMT_ECHO:
            expr_free(stmt->as.echo_stmt.value);
            break;
        case STMT_EXPR:
            expr_free(stmt->as.expr_stmt.value);
            break;
        case STMT_RETURN:
            expr_free(stmt->as.return_stmt.value);
            break;
        default:
            break;
    }

    free(stmt);
}

void program_init(Program *program) {
    block_init(program);
}

int program_push(Program *program, Stmt *stmt) {
    return block_push(program, stmt);
}

void program_free(Program *program) {
    block_free(program);
}

#include "atlas/ast.h"

#include <stdlib.h>

#include "atlas/util.h"

Expr *expr_number_new(double value) {
    Expr *expr = (Expr *)calloc(1, sizeof(Expr));
    if (expr == NULL) {
        return NULL;
    }
    expr->type = EXPR_NUMBER;
    expr->as.number = value;
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

void expr_free(Expr *expr) {
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
        case EXPR_NUMBER:
        default:
            break;
    }

    free(expr);
}

Stmt *stmt_globe_new(const char *name, Block *body) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));

    if (stmt == NULL) {
        return NULL;
    }

    stmt->type = STMT_GLOBE;
    stmt->as.globe_stmt.name = atlas_strdup(name);
    block_init(&stmt->as.globe_stmt.body);

    if (stmt->as.globe_stmt.name == NULL) {
        free(stmt);
        return NULL;
    }

    stmt->as.globe_stmt.body = *body;
    body->items = NULL;
    body->count = 0;
    body->capacity = 0;

    return stmt;
}

Stmt *stmt_ignite_new(const char *name) {
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

void stmt_free(Stmt *stmt) {
    if (stmt == NULL) {
        return;
    }

    switch (stmt->type) {
        case STMT_GLOBE:
            free(stmt->as.globe_stmt.name);
            block_free(&stmt->as.globe_stmt.body);
            break;
        case STMT_IGNITE:
            free(stmt->as.ignite_stmt.name);
            break;
        case STMT_SEED:
            free(stmt->as.seed_stmt.name);
            expr_free(stmt->as.seed_stmt.value);
            break;
        case STMT_ASSIGN:
            free(stmt->as.assign_stmt.name);
            expr_free(stmt->as.assign_stmt.value);
            break;
        case STMT_ECHO:
            expr_free(stmt->as.echo_stmt.value);
            break;
        case STMT_EXPR:
            expr_free(stmt->as.expr_stmt.value);
            break;
        default:
            break;
    }

    free(stmt);
}

void block_init(Block *block) {
    block->items = NULL;
    block->count = 0;
    block->capacity = 0;
}

int block_push(Block *block, Stmt *stmt) {
    if (block->count == block->capacity) {
        size_t new_capacity = block->capacity == 0 ? 8 : block->capacity * 2;
        Stmt **new_items = (Stmt **)realloc(block->items, new_capacity * sizeof(Stmt *));
        if (new_items == NULL) {
            return 0;
        }
        block->items = new_items;
        block->capacity = new_capacity;
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

void program_init(Program *program) {
    block_init(program);
}

int program_push(Program *program, Stmt *stmt) {
    return block_push(program, stmt);
}

void program_free(Program *program) {
    block_free(program);
}

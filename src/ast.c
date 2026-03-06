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

Stmt *stmt_let_new(const char *name, Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }
    stmt->type = STMT_LET;
    stmt->as.let_stmt.name = atlas_strdup(name);
    stmt->as.let_stmt.value = value;
    if (stmt->as.let_stmt.name == NULL) {
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

Stmt *stmt_print_new(Expr *value) {
    Stmt *stmt = (Stmt *)calloc(1, sizeof(Stmt));
    if (stmt == NULL) {
        return NULL;
    }
    stmt->type = STMT_PRINT;
    stmt->as.print_stmt.value = value;
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
        case STMT_LET:
            free(stmt->as.let_stmt.name);
            expr_free(stmt->as.let_stmt.value);
            break;
        case STMT_ASSIGN:
            free(stmt->as.assign_stmt.name);
            expr_free(stmt->as.assign_stmt.value);
            break;
        case STMT_PRINT:
            expr_free(stmt->as.print_stmt.value);
            break;
        case STMT_EXPR:
            expr_free(stmt->as.expr_stmt.value);
            break;
        default:
            break;
    }

    free(stmt);
}

void program_init(Program *program) {
    program->items = NULL;
    program->count = 0;
    program->capacity = 0;
}

int program_push(Program *program, Stmt *stmt) {
    if (program->count == program->capacity) {
        size_t new_capacity = program->capacity == 0 ? 8 : program->capacity * 2;
        Stmt **new_items = (Stmt **)realloc(program->items, new_capacity * sizeof(Stmt *));
        if (new_items == NULL) {
            return 0;
        }
        program->items = new_items;
        program->capacity = new_capacity;
    }

    program->items[program->count++] = stmt;
    return 1;
}

void program_free(Program *program) {
    size_t i;

    for (i = 0; i < program->count; ++i) {
        stmt_free(program->items[i]);
    }

    free(program->items);
    program->items = NULL;
    program->count = 0;
    program->capacity = 0;
}

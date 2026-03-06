#ifndef ATLAS_AST_H
#define ATLAS_AST_H

#include <stddef.h>

typedef enum {
    EXPR_NUMBER,
    EXPR_VARIABLE,
    EXPR_BINARY,
    EXPR_UNARY
} ExprType;

typedef struct Expr Expr;

struct Expr {
    ExprType type;
    union {
        double number;
        char *name;
        struct {
            char op;
            Expr *left;
            Expr *right;
        } binary;
        struct {
            char op;
            Expr *right;
        } unary;
    } as;
};

typedef enum {
    STMT_LET,
    STMT_ASSIGN,
    STMT_PRINT,
    STMT_EXPR
} StmtType;

typedef struct Stmt Stmt;

struct Stmt {
    StmtType type;
    union {
        struct {
            char *name;
            Expr *value;
        } let_stmt;
        struct {
            char *name;
            Expr *value;
        } assign_stmt;
        struct {
            Expr *value;
        } print_stmt;
        struct {
            Expr *value;
        } expr_stmt;
    } as;
};

typedef struct {
    Stmt **items;
    size_t count;
    size_t capacity;
} Program;

Expr *expr_number_new(double value);
Expr *expr_variable_new(const char *name);
Expr *expr_binary_new(char op, Expr *left, Expr *right);
Expr *expr_unary_new(char op, Expr *right);
void expr_free(Expr *expr);

Stmt *stmt_let_new(const char *name, Expr *value);
Stmt *stmt_assign_new(const char *name, Expr *value);
Stmt *stmt_print_new(Expr *value);
Stmt *stmt_expr_new(Expr *value);
void stmt_free(Stmt *stmt);

void program_init(Program *program);
int program_push(Program *program, Stmt *stmt);
void program_free(Program *program);

#endif

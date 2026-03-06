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
    STMT_GLOBE,
    STMT_IGNITE,
    STMT_SEED,
    STMT_ASSIGN,
    STMT_ECHO,
    STMT_EXPR
} StmtType;

typedef struct Stmt Stmt;
typedef struct Block Block;

struct Block {
    Stmt **items;
    size_t count;
    size_t capacity;
};

struct Stmt {
    StmtType type;
    union {
        struct {
            char *name;
            Block body;
        } globe_stmt;
        struct {
            char *name;
        } ignite_stmt;
        struct {
            char *name;
            Expr *value;
        } seed_stmt;
        struct {
            char *name;
            Expr *value;
        } assign_stmt;
        struct {
            Expr *value;
        } echo_stmt;
        struct {
            Expr *value;
        } expr_stmt;
    } as;
};

typedef Block Program;

Expr *expr_number_new(double value);
Expr *expr_variable_new(const char *name);
Expr *expr_binary_new(char op, Expr *left, Expr *right);
Expr *expr_unary_new(char op, Expr *right);
void expr_free(Expr *expr);

Stmt *stmt_globe_new(const char *name, Block *body);
Stmt *stmt_ignite_new(const char *name);
Stmt *stmt_seed_new(const char *name, Expr *value);
Stmt *stmt_assign_new(const char *name, Expr *value);
Stmt *stmt_echo_new(Expr *value);
Stmt *stmt_expr_new(Expr *value);
void stmt_free(Stmt *stmt);

void block_init(Block *block);
int block_push(Block *block, Stmt *stmt);
void block_free(Block *block);

void program_init(Program *program);
int program_push(Program *program, Stmt *stmt);
void program_free(Program *program);

#endif

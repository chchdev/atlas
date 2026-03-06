#ifndef ATLAS_AST_H
#define ATLAS_AST_H

#include <stddef.h>

typedef struct Expr Expr;
typedef struct Stmt Stmt;
typedef struct Block Block;

typedef struct {
    Expr **items;
    size_t count;
    size_t capacity;
} ExprList;

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} NameList;

typedef enum {
    EXPR_NUMBER,
    EXPR_BOOL,
    EXPR_VARIABLE,
    EXPR_BINARY,
    EXPR_UNARY,
    EXPR_CALL,
    EXPR_FIELD,
    EXPR_CRAFT
} ExprType;

struct Expr {
    ExprType type;
    union {
        double number;
        int boolean;
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
        struct {
            char *name;
            ExprList args;
        } call;
        struct {
            Expr *object;
            char *field;
        } field;
        struct {
            char *mold_name;
        } craft;
    } as;
};

typedef struct {
    char *name;
    Expr *value;
} FieldInit;

typedef struct {
    FieldInit *items;
    size_t count;
    size_t capacity;
} FieldInitList;

struct Block {
    Stmt **items;
    size_t count;
    size_t capacity;
};

typedef enum {
    STMT_GLOBE,
    STMT_MOLD,
    STMT_IGNITE,
    STMT_SEED,
    STMT_ASSIGN,
    STMT_FIELD_ASSIGN,
    STMT_ECHO,
    STMT_EXPR,
    STMT_RETURN,
    STMT_ORBIT,
    STMT_BREAK,
    STMT_CONTINUE
} StmtType;

struct Stmt {
    StmtType type;
    union {
        struct {
            char *name;
            NameList params;
            Block body;
        } globe_stmt;
        struct {
            char *name;
            FieldInitList fields;
        } mold_stmt;
        struct {
            char *name;
            ExprList args;
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
            char *object_name;
            char *field_name;
            Expr *value;
        } field_assign_stmt;
        struct {
            Expr *value;
        } echo_stmt;
        struct {
            Expr *value;
        } expr_stmt;
        struct {
            Expr *value;
        } return_stmt;
        struct {
            Expr *condition;
            Block body;
        } orbit_stmt;
    } as;
};

typedef Block Program;

Expr *expr_number_new(double value);
Expr *expr_bool_new(int value);
Expr *expr_variable_new(const char *name);
Expr *expr_binary_new(char op, Expr *left, Expr *right);
Expr *expr_unary_new(char op, Expr *right);
Expr *expr_call_new(const char *name, ExprList *args);
Expr *expr_field_new(Expr *object, const char *field);
Expr *expr_craft_new(const char *mold_name);
void expr_free(Expr *expr);

Stmt *stmt_globe_new(const char *name, NameList *params, Block *body);
Stmt *stmt_mold_new(const char *name, FieldInitList *fields);
Stmt *stmt_ignite_new(const char *name, ExprList *args);
Stmt *stmt_seed_new(const char *name, Expr *value);
Stmt *stmt_assign_new(const char *name, Expr *value);
Stmt *stmt_field_assign_new(const char *object_name, const char *field_name, Expr *value);
Stmt *stmt_echo_new(Expr *value);
Stmt *stmt_expr_new(Expr *value);
Stmt *stmt_return_new(Expr *value);
Stmt *stmt_orbit_new(Expr *condition, Block *body);
Stmt *stmt_break_new(void);
Stmt *stmt_continue_new(void);
void stmt_free(Stmt *stmt);

void expr_list_init(ExprList *list);
int expr_list_push(ExprList *list, Expr *expr);
void expr_list_free(ExprList *list);

void name_list_init(NameList *list);
int name_list_push(NameList *list, const char *name);
void name_list_free(NameList *list);

void field_init_list_init(FieldInitList *list);
int field_init_list_push(FieldInitList *list, const char *name, Expr *value);
void field_init_list_free(FieldInitList *list);

void block_init(Block *block);
int block_push(Block *block, Stmt *stmt);
void block_free(Block *block);

void program_init(Program *program);
int program_push(Program *program, Stmt *stmt);
void program_free(Program *program);

#endif

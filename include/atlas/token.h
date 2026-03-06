#ifndef ATLAS_TOKEN_H
#define ATLAS_TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_LET,
    TOKEN_PRINT,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_EQUAL,
    TOKEN_SEMICOLON,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_INVALID
} TokenType;

typedef struct {
    TokenType type;
    double number;
    char *lexeme;
    size_t length;
    size_t line;
    size_t column;
} Token;

#endif

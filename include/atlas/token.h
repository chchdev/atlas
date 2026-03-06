#ifndef ATLAS_TOKEN_H
#define ATLAS_TOKEN_H

#include <stddef.h>

typedef enum {
    TOKEN_EOF,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_GLOBE,
    TOKEN_IGNITE,
    TOKEN_SEED,
    TOKEN_ECHO,
    TOKEN_MOLD,
    TOKEN_CRAFT,
    TOKEN_RETURN,
    TOKEN_ORBIT,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_SLASH,
    TOKEN_ARROW,
    TOKEN_SEMICOLON,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
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

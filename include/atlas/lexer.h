#ifndef ATLAS_LEXER_H
#define ATLAS_LEXER_H

#include <stddef.h>

#include "atlas/token.h"

typedef struct {
    const char *source;
    size_t source_length;
    size_t start;
    size_t current;
    size_t line;
    size_t column;
    size_t token_column;
} Lexer;

void lexer_init(Lexer *lexer, const char *source);
Token lexer_next_token(Lexer *lexer);
void token_free(Token *token);

#endif

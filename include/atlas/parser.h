#ifndef ATLAS_PARSER_H
#define ATLAS_PARSER_H

#include "atlas/ast.h"
#include "atlas/lexer.h"

typedef struct {
    Lexer lexer;
    Token current;
    Token previous;
    int had_error;
    char error_message[256];
} Parser;

void parser_init(Parser *parser, const char *source);
int parser_parse(Parser *parser, Program *program);
void parser_destroy(Parser *parser);

#endif

#include "atlas/lexer.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static int is_at_end(const Lexer *lexer) {
    return lexer->current >= lexer->source_length;
}

static char peek(const Lexer *lexer) {
    if (is_at_end(lexer)) {
        return '\0';
    }
    return lexer->source[lexer->current];
}

static char advance(Lexer *lexer) {
    char c;

    if (is_at_end(lexer)) {
        return '\0';
    }

    c = lexer->source[lexer->current++];
    if (c == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }

    return c;
}

static Token make_token(Lexer *lexer, TokenType type) {
    Token token;
    size_t length;

    token.type = type;
    token.number = 0.0;
    token.lexeme = NULL;
    token.length = 0;
    token.line = lexer->line;
    token.column = lexer->token_column;

    if (type == TOKEN_EOF) {
        return token;
    }

    length = lexer->current - lexer->start;
    token.length = length;

    if (length > 0) {
        char *temp = (char *)malloc(length + 1);
        if (temp != NULL) {
            memcpy(temp, lexer->source + lexer->start, length);
            temp[length] = '\0';
            token.lexeme = temp;
        }
    }

    return token;
}

static void skip_whitespace(Lexer *lexer) {
    for (;;) {
        char c = peek(lexer);
        if (c == ' ' || c == '\r' || c == '\t' || c == '\n') {
            advance(lexer);
        } else {
            break;
        }
    }
}

static Token identifier(Lexer *lexer) {
    while (isalnum((unsigned char)peek(lexer)) || peek(lexer) == '_') {
        advance(lexer);
    }

    {
        Token token = make_token(lexer, TOKEN_IDENTIFIER);
        if (token.lexeme != NULL) {
            if (strcmp(token.lexeme, "true") == 0) {
                token.type = TOKEN_TRUE;
            } else if (strcmp(token.lexeme, "false") == 0) {
                token.type = TOKEN_FALSE;
            } else if (strcmp(token.lexeme, "globe") == 0) {
                token.type = TOKEN_GLOBE;
            } else if (strcmp(token.lexeme, "ignite") == 0) {
                token.type = TOKEN_IGNITE;
            } else if (strcmp(token.lexeme, "seed") == 0) {
                token.type = TOKEN_SEED;
            } else if (strcmp(token.lexeme, "echo") == 0) {
                token.type = TOKEN_ECHO;
            } else if (strcmp(token.lexeme, "mold") == 0) {
                token.type = TOKEN_MOLD;
            } else if (strcmp(token.lexeme, "craft") == 0) {
                token.type = TOKEN_CRAFT;
            } else if (strcmp(token.lexeme, "return") == 0) {
                token.type = TOKEN_RETURN;
            } else if (strcmp(token.lexeme, "orbit") == 0) {
                token.type = TOKEN_ORBIT;
            } else if (strcmp(token.lexeme, "break") == 0) {
                token.type = TOKEN_BREAK;
            } else if (strcmp(token.lexeme, "continue") == 0) {
                token.type = TOKEN_CONTINUE;
            }
        }
        return token;
    }
}

static Token number(Lexer *lexer) {
    Token token;

    while (isdigit((unsigned char)peek(lexer))) {
        advance(lexer);
    }

    if (peek(lexer) == '.') {
        advance(lexer);
        while (isdigit((unsigned char)peek(lexer))) {
            advance(lexer);
        }
    }

    token = make_token(lexer, TOKEN_NUMBER);
    if (token.lexeme != NULL) {
        token.number = strtod(token.lexeme, NULL);
    }

    return token;
}

void lexer_init(Lexer *lexer, const char *source) {
    size_t len = strlen(source);

    lexer->source = source;
    lexer->source_length = len;
    lexer->start = 0;
    lexer->current = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->token_column = 1;
}

Token lexer_next_token(Lexer *lexer) {
    char c;

    skip_whitespace(lexer);
    lexer->start = lexer->current;
    lexer->token_column = lexer->column;

    if (is_at_end(lexer)) {
        return make_token(lexer, TOKEN_EOF);
    }

    c = advance(lexer);

    if (isalpha((unsigned char)c) || c == '_') {
        return identifier(lexer);
    }

    if (isdigit((unsigned char)c)) {
        return number(lexer);
    }

    switch (c) {
        case '+':
            return make_token(lexer, TOKEN_PLUS);
        case '-':
            return make_token(lexer, TOKEN_MINUS);
        case '*':
            return make_token(lexer, TOKEN_STAR);
        case '/':
            return make_token(lexer, TOKEN_SLASH);
        case '<':
            if (peek(lexer) == '-') {
                advance(lexer);
                return make_token(lexer, TOKEN_ARROW);
            }
            return make_token(lexer, TOKEN_INVALID);
        case ';':
            return make_token(lexer, TOKEN_SEMICOLON);
        case ',':
            return make_token(lexer, TOKEN_COMMA);
        case '.':
            return make_token(lexer, TOKEN_DOT);
        case '(':
            return make_token(lexer, TOKEN_LPAREN);
        case ')':
            return make_token(lexer, TOKEN_RPAREN);
        case '{':
            return make_token(lexer, TOKEN_LBRACE);
        case '}':
            return make_token(lexer, TOKEN_RBRACE);
        default:
            return make_token(lexer, TOKEN_INVALID);
    }
}

void token_free(Token *token) {
    free(token->lexeme);
    token->lexeme = NULL;
}

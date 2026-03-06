#include "atlas/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atlas/util.h"

static void parser_error(Parser *parser, const char *message) {
    if (!parser->had_error) {
        parser->had_error = 1;
        snprintf(
            parser->error_message,
            sizeof(parser->error_message),
            "[line %u, col %u] %s",
            (unsigned int)parser->current.line,
            (unsigned int)parser->current.column,
            message
        );
    }
}

static void advance_token(Parser *parser) {
    token_free(&parser->previous);
    parser->previous = parser->current;
    parser->current = lexer_next_token(&parser->lexer);
}

static int check(const Parser *parser, TokenType type) {
    return parser->current.type == type;
}

static int match(Parser *parser, TokenType type) {
    if (!check(parser, type)) {
        return 0;
    }
    advance_token(parser);
    return 1;
}

static int consume(Parser *parser, TokenType type, const char *message) {
    if (check(parser, type)) {
        advance_token(parser);
        return 1;
    }
    parser_error(parser, message);
    return 0;
}

static Expr *expression(Parser *parser);
static Stmt *inner_statement(Parser *parser);

static Expr *primary(Parser *parser) {
    if (match(parser, TOKEN_NUMBER)) {
        return expr_number_new(parser->previous.number);
    }

    if (match(parser, TOKEN_IDENTIFIER)) {
        return expr_variable_new(parser->previous.lexeme);
    }

    if (match(parser, TOKEN_LPAREN)) {
        Expr *expr = expression(parser);
        if (!consume(parser, TOKEN_RPAREN, "Expected ')' after expression.")) {
            expr_free(expr);
            return NULL;
        }
        return expr;
    }

    parser_error(parser, "Expected expression.");
    return NULL;
}

static Expr *unary(Parser *parser) {
    if (match(parser, TOKEN_MINUS)) {
        Expr *right = unary(parser);
        if (right == NULL) {
            return NULL;
        }
        return expr_unary_new('-', right);
    }

    return primary(parser);
}

static Expr *factor(Parser *parser) {
    Expr *expr = unary(parser);
    while (expr != NULL && (match(parser, TOKEN_STAR) || match(parser, TOKEN_SLASH))) {
        char op = parser->previous.type == TOKEN_STAR ? '*' : '/';
        Expr *right = unary(parser);
        if (right == NULL) {
            expr_free(expr);
            return NULL;
        }
        expr = expr_binary_new(op, expr, right);
        if (expr == NULL) {
            expr_free(right);
            return NULL;
        }
    }
    return expr;
}

static Expr *term(Parser *parser) {
    Expr *expr = factor(parser);
    while (expr != NULL && (match(parser, TOKEN_PLUS) || match(parser, TOKEN_MINUS))) {
        char op = parser->previous.type == TOKEN_PLUS ? '+' : '-';
        Expr *right = factor(parser);
        if (right == NULL) {
            expr_free(expr);
            return NULL;
        }
        expr = expr_binary_new(op, expr, right);
        if (expr == NULL) {
            expr_free(right);
            return NULL;
        }
    }
    return expr;
}

static Expr *expression(Parser *parser) {
    return term(parser);
}

static Stmt *seed_statement(Parser *parser) {
    char *name;
    Expr *value;
    Stmt *stmt;

    if (!consume(parser, TOKEN_IDENTIFIER, "Expected seed name after 'seed'.")) {
        return NULL;
    }

    name = atlas_strdup(parser->previous.lexeme);
    if (name == NULL) {
        parser_error(parser, "Out of memory while reading seed name.");
        return NULL;
    }

    if (!consume(parser, TOKEN_ARROW, "Expected '<-' after seed name.")) {
        free(name);
        return NULL;
    }

    value = expression(parser);
    if (value == NULL) {
        free(name);
        return NULL;
    }

    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after seed declaration.")) {
        expr_free(value);
        free(name);
        return NULL;
    }

    stmt = stmt_seed_new(name, value);
    free(name);
    return stmt;
}

static Stmt *ignite_statement(Parser *parser) {
    char *name;
    Stmt *stmt;

    if (!consume(parser, TOKEN_IDENTIFIER, "Expected globe name after 'ignite'.")) {
        return NULL;
    }

    name = atlas_strdup(parser->previous.lexeme);
    if (name == NULL) {
        parser_error(parser, "Out of memory while reading globe name.");
        return NULL;
    }

    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after ignite statement.")) {
        free(name);
        return NULL;
    }

    stmt = stmt_ignite_new(name);
    free(name);
    return stmt;
}

static Stmt *assignment_or_expression(Parser *parser) {
    if (match(parser, TOKEN_IDENTIFIER)) {
        char *name = atlas_strdup(parser->previous.lexeme);
        if (name == NULL) {
            parser_error(parser, "Out of memory while reading identifier.");
            return NULL;
        }

        if (match(parser, TOKEN_ARROW)) {
            Expr *value = expression(parser);
            Stmt *stmt;
            if (value == NULL) {
                free(name);
                return NULL;
            }
            if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after assignment.")) {
                expr_free(value);
                free(name);
                return NULL;
            }
            stmt = stmt_assign_new(name, value);
            free(name);
            return stmt;
        }

        free(name);
        parser_error(parser, "Expected '<-' after identifier.");
        return NULL;
    }

    {
        Expr *value = expression(parser);
        Stmt *stmt;
        if (value == NULL) {
            return NULL;
        }
        if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after expression.")) {
            expr_free(value);
            return NULL;
        }
        stmt = stmt_expr_new(value);
        return stmt;
    }
}

static Stmt *inner_statement(Parser *parser) {
    if (match(parser, TOKEN_SEED)) {
        return seed_statement(parser);
    }

    if (match(parser, TOKEN_ECHO)) {
        Expr *value = expression(parser);
        Stmt *stmt;
        if (value == NULL) {
            return NULL;
        }
        if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after echo statement.")) {
            expr_free(value);
            return NULL;
        }
        stmt = stmt_echo_new(value);
        return stmt;
    }

    if (match(parser, TOKEN_IGNITE)) {
        return ignite_statement(parser);
    }

    return assignment_or_expression(parser);
}

static Stmt *globe_declaration(Parser *parser) {
    char *name;
    Block body;
    Stmt *stmt;

    if (!consume(parser, TOKEN_IDENTIFIER, "Expected globe name after 'globe'.")) {
        return NULL;
    }

    name = atlas_strdup(parser->previous.lexeme);
    if (name == NULL) {
        parser_error(parser, "Out of memory while reading globe name.");
        return NULL;
    }

    if (!consume(parser, TOKEN_LBRACE, "Expected '{' to start globe body.")) {
        free(name);
        return NULL;
    }

    block_init(&body);

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        Stmt *inner = inner_statement(parser);
        if (inner == NULL) {
            block_free(&body);
            free(name);
            return NULL;
        }
        if (!block_push(&body, inner)) {
            stmt_free(inner);
            block_free(&body);
            parser_error(parser, "Out of memory while building globe body.");
            free(name);
            return NULL;
        }
    }

    if (!consume(parser, TOKEN_RBRACE, "Expected '}' after globe body.")) {
        block_free(&body);
        free(name);
        return NULL;
    }

    stmt = stmt_globe_new(name, &body);
    free(name);
    if (stmt == NULL) {
        block_free(&body);
        parser_error(parser, "Out of memory while building globe declaration.");
        return NULL;
    }

    return stmt;
}

static Stmt *top_level_statement(Parser *parser) {
    if (match(parser, TOKEN_GLOBE)) {
        return globe_declaration(parser);
    }

    if (match(parser, TOKEN_IGNITE)) {
        return ignite_statement(parser);
    }

    parser_error(parser, "Top-level only allows 'globe' and 'ignite'.");
    return NULL;
}

void parser_init(Parser *parser, const char *source) {
    memset(parser, 0, sizeof(*parser));
    lexer_init(&parser->lexer, source);
    parser->current = lexer_next_token(&parser->lexer);
    parser->previous.type = TOKEN_EOF;
    parser->previous.lexeme = NULL;
    parser->previous.length = 0;
    parser->previous.number = 0.0;
    parser->previous.line = 1;
    parser->previous.column = 1;
}

int parser_parse(Parser *parser, Program *program) {
    program_init(program);

    while (!check(parser, TOKEN_EOF)) {
        Stmt *stmt = top_level_statement(parser);
        if (stmt == NULL) {
            program_free(program);
            return 0;
        }
        if (!program_push(program, stmt)) {
            stmt_free(stmt);
            parser_error(parser, "Out of memory while building program.");
            program_free(program);
            return 0;
        }
    }

    if (parser->had_error) {
        program_free(program);
        return 0;
    }

    return 1;
}

void parser_destroy(Parser *parser) {
    token_free(&parser->current);
    token_free(&parser->previous);
}

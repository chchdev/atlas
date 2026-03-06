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

static TokenType peek_next_type(const Parser *parser) {
    Lexer snapshot = parser->lexer;
    Token lookahead = lexer_next_token(&snapshot);
    TokenType type = lookahead.type;
    token_free(&lookahead);
    return type;
}

static Expr *expression(Parser *parser);
static Stmt *inner_statement(Parser *parser);

static int parse_identifier_list(Parser *parser, NameList *params) {
    name_list_init(params);

    if (check(parser, TOKEN_RPAREN)) {
        return 1;
    }

    do {
        if (!consume(parser, TOKEN_IDENTIFIER, "Expected parameter name.")) {
            name_list_free(params);
            return 0;
        }
        if (!name_list_push(params, parser->previous.lexeme)) {
            name_list_free(params);
            parser_error(parser, "Out of memory while storing parameter.");
            return 0;
        }
    } while (match(parser, TOKEN_COMMA));

    return 1;
}

static int parse_expression_list(Parser *parser, ExprList *args) {
    expr_list_init(args);

    if (check(parser, TOKEN_RPAREN)) {
        return 1;
    }

    do {
        Expr *arg = expression(parser);
        if (arg == NULL) {
            expr_list_free(args);
            return 0;
        }
        if (!expr_list_push(args, arg)) {
            expr_free(arg);
            expr_list_free(args);
            parser_error(parser, "Out of memory while storing argument.");
            return 0;
        }
    } while (match(parser, TOKEN_COMMA));

    return 1;
}

static Expr *primary(Parser *parser) {
    if (match(parser, TOKEN_NUMBER)) {
        return expr_number_new(parser->previous.number);
    }

    if (match(parser, TOKEN_TRUE)) {
        return expr_bool_new(1);
    }

    if (match(parser, TOKEN_FALSE)) {
        return expr_bool_new(0);
    }

    if (match(parser, TOKEN_IDENTIFIER)) {
        return expr_variable_new(parser->previous.lexeme);
    }

    if (match(parser, TOKEN_CRAFT)) {
        char *mold_name;
        Expr *expr;
        ExprList args;

        if (!consume(parser, TOKEN_LPAREN, "Expected '(' after 'craft'.")) {
            return NULL;
        }
        if (!consume(parser, TOKEN_IDENTIFIER, "Expected mold name in craft(...).")) {
            return NULL;
        }

        mold_name = atlas_strdup(parser->previous.lexeme);
        if (mold_name == NULL) {
            parser_error(parser, "Out of memory while reading mold name.");
            return NULL;
        }

        expr_list_init(&args);
        if (match(parser, TOKEN_COMMA)) {
            if (!parse_expression_list(parser, &args)) {
                free(mold_name);
                return NULL;
            }
        }

        if (!consume(parser, TOKEN_RPAREN, "Expected ')' after craft arguments.")) {
            free(mold_name);
            expr_list_free(&args);
            return NULL;
        }

        expr = expr_craft_new(mold_name, &args);
        free(mold_name);
        return expr;
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

static Expr *postfix(Parser *parser) {
    Expr *expr = primary(parser);

    while (expr != NULL) {
        if (match(parser, TOKEN_DOT)) {
            char *member;
            Expr *next;

            if (!consume(parser, TOKEN_IDENTIFIER, "Expected field name after '.'.")) {
                expr_free(expr);
                return NULL;
            }

            member = atlas_strdup(parser->previous.lexeme);
            if (member == NULL) {
                expr_free(expr);
                parser_error(parser, "Out of memory while reading member name.");
                return NULL;
            }

            if (match(parser, TOKEN_LPAREN)) {
                ExprList args;

                if (!parse_expression_list(parser, &args)) {
                    expr_free(expr);
                    free(member);
                    return NULL;
                }

                if (!consume(parser, TOKEN_RPAREN, "Expected ')' after method arguments.")) {
                    expr_free(expr);
                    expr_list_free(&args);
                    free(member);
                    return NULL;
                }

                next = expr_method_call_new(expr, member, &args);
                free(member);
                if (next == NULL) {
                    expr_list_free(&args);
                    return NULL;
                }
                expr = next;
                continue;
            }

            next = expr_field_new(expr, member);
            free(member);
            if (next == NULL) {
                return NULL;
            }
            expr = next;
            continue;
        }

        if (match(parser, TOKEN_LPAREN)) {
            ExprList args;
            Expr *next;

            if (expr->type != EXPR_VARIABLE) {
                expr_free(expr);
                parser_error(parser, "Only named call targets are supported.");
                return NULL;
            }

            if (!parse_expression_list(parser, &args)) {
                expr_free(expr);
                return NULL;
            }

            if (!consume(parser, TOKEN_RPAREN, "Expected ')' after arguments.")) {
                expr_free(expr);
                expr_list_free(&args);
                return NULL;
            }

            next = expr_call_new(expr->as.name, &args);
            expr_free(expr);
            if (next == NULL) {
                expr_list_free(&args);
                parser_error(parser, "Out of memory while building call expression.");
                return NULL;
            }
            expr = next;
            continue;
        }

        break;
    }

    return expr;
}

static Expr *unary(Parser *parser) {
    if (match(parser, TOKEN_MINUS)) {
        Expr *right = unary(parser);
        if (right == NULL) {
            return NULL;
        }
        return expr_unary_new('-', right);
    }

    return postfix(parser);
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
    ExprList args;
    Stmt *stmt;

    if (!consume(parser, TOKEN_IDENTIFIER, "Expected globe name after 'ignite'.")) {
        return NULL;
    }

    name = atlas_strdup(parser->previous.lexeme);
    if (name == NULL) {
        parser_error(parser, "Out of memory while reading globe name.");
        return NULL;
    }

    if (!consume(parser, TOKEN_LPAREN, "Expected '(' after globe name in ignite statement.")) {
        free(name);
        return NULL;
    }

    if (!parse_expression_list(parser, &args)) {
        free(name);
        return NULL;
    }

    if (!consume(parser, TOKEN_RPAREN, "Expected ')' after ignite arguments.")) {
        expr_list_free(&args);
        free(name);
        return NULL;
    }

    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after ignite statement.")) {
        expr_list_free(&args);
        free(name);
        return NULL;
    }

    stmt = stmt_ignite_new(name, &args);
    free(name);
    return stmt;
}

static Stmt *return_statement(Parser *parser) {
    Expr *value = expression(parser);
    Stmt *stmt;

    if (value == NULL) {
        return NULL;
    }

    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after return value.")) {
        expr_free(value);
        return NULL;
    }

    stmt = stmt_return_new(value);
    return stmt;
}

static Stmt *break_statement(Parser *parser) {
    Stmt *stmt;
    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after break.")) {
        return NULL;
    }
    stmt = stmt_break_new();
    if (stmt == NULL) {
        parser_error(parser, "Out of memory while creating break statement.");
        return NULL;
    }
    return stmt;
}

static Stmt *continue_statement(Parser *parser) {
    Stmt *stmt;
    if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after continue.")) {
        return NULL;
    }
    stmt = stmt_continue_new();
    if (stmt == NULL) {
        parser_error(parser, "Out of memory while creating continue statement.");
        return NULL;
    }
    return stmt;
}

static Stmt *orbit_statement(Parser *parser) {
    Expr *condition;
    Block body;
    Stmt *stmt;

    if (!consume(parser, TOKEN_LPAREN, "Expected '(' after 'orbit'.")) {
        return NULL;
    }

    condition = expression(parser);
    if (condition == NULL) {
        return NULL;
    }

    if (!consume(parser, TOKEN_RPAREN, "Expected ')' after orbit condition.")) {
        expr_free(condition);
        return NULL;
    }

    if (!consume(parser, TOKEN_LBRACE, "Expected '{' to start orbit body.")) {
        expr_free(condition);
        return NULL;
    }

    block_init(&body);
    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        Stmt *inner = inner_statement(parser);
        if (inner == NULL) {
            expr_free(condition);
            block_free(&body);
            return NULL;
        }
        if (!block_push(&body, inner)) {
            stmt_free(inner);
            expr_free(condition);
            block_free(&body);
            parser_error(parser, "Out of memory while building orbit body.");
            return NULL;
        }
    }

    if (!consume(parser, TOKEN_RBRACE, "Expected '}' after orbit body.")) {
        expr_free(condition);
        block_free(&body);
        return NULL;
    }

    stmt = stmt_orbit_new(condition, &body);
    if (stmt == NULL) {
        expr_free(condition);
        block_free(&body);
        parser_error(parser, "Out of memory while creating orbit statement.");
        return NULL;
    }

    return stmt;
}

static Stmt *assignment_or_expression(Parser *parser) {
    TokenType next_type = peek_next_type(parser);

    if (check(parser, TOKEN_IDENTIFIER) && (next_type == TOKEN_ARROW || next_type == TOKEN_DOT)) {
        match(parser, TOKEN_IDENTIFIER);
        char *name = atlas_strdup(parser->previous.lexeme);

        if (name == NULL) {
            parser_error(parser, "Out of memory while reading identifier.");
            return NULL;
        }

        if (match(parser, TOKEN_DOT)) {
            char *field_name;
            Expr *value;
            Stmt *stmt;

            if (!consume(parser, TOKEN_IDENTIFIER, "Expected field name after '.'.")) {
                free(name);
                return NULL;
            }

            field_name = atlas_strdup(parser->previous.lexeme);
            if (field_name == NULL) {
                free(name);
                parser_error(parser, "Out of memory while reading field name.");
                return NULL;
            }

            if (!consume(parser, TOKEN_ARROW, "Expected '<-' after field target.")) {
                free(name);
                free(field_name);
                return NULL;
            }

            value = expression(parser);
            if (value == NULL) {
                free(name);
                free(field_name);
                return NULL;
            }

            if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after field assignment.")) {
                expr_free(value);
                free(name);
                free(field_name);
                return NULL;
            }

            stmt = stmt_field_assign_new(name, field_name, value);
            free(name);
            free(field_name);
            return stmt;
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
        parser_error(parser, "Expected '<-' or '.field <-' after identifier.");
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

    if (match(parser, TOKEN_RETURN)) {
        return return_statement(parser);
    }

    if (match(parser, TOKEN_BREAK)) {
        return break_statement(parser);
    }

    if (match(parser, TOKEN_CONTINUE)) {
        return continue_statement(parser);
    }

    if (match(parser, TOKEN_ORBIT)) {
        return orbit_statement(parser);
    }

    return assignment_or_expression(parser);
}

static Stmt *globe_declaration(Parser *parser) {
    char *name;
    NameList params;
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

    if (!consume(parser, TOKEN_LPAREN, "Expected '(' after globe name.")) {
        free(name);
        return NULL;
    }

    if (!parse_identifier_list(parser, &params)) {
        free(name);
        return NULL;
    }

    if (!consume(parser, TOKEN_RPAREN, "Expected ')' after parameter list.")) {
        name_list_free(&params);
        free(name);
        return NULL;
    }

    if (!consume(parser, TOKEN_LBRACE, "Expected '{' to start globe body.")) {
        name_list_free(&params);
        free(name);
        return NULL;
    }

    block_init(&body);

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        Stmt *inner = inner_statement(parser);
        if (inner == NULL) {
            block_free(&body);
            name_list_free(&params);
            free(name);
            return NULL;
        }
        if (!block_push(&body, inner)) {
            stmt_free(inner);
            block_free(&body);
            name_list_free(&params);
            parser_error(parser, "Out of memory while building globe body.");
            free(name);
            return NULL;
        }
    }

    if (!consume(parser, TOKEN_RBRACE, "Expected '}' after globe body.")) {
        block_free(&body);
        name_list_free(&params);
        free(name);
        return NULL;
    }

    stmt = stmt_globe_new(name, &params, &body);
    free(name);
    if (stmt == NULL) {
        block_free(&body);
        name_list_free(&params);
        parser_error(parser, "Out of memory while building globe declaration.");
        return NULL;
    }

    return stmt;
}

static Stmt *mold_declaration(Parser *parser) {
    char *name;
    char *parent_name = NULL;
    FieldInitList fields;
    Stmt *stmt;

    if (!consume(parser, TOKEN_IDENTIFIER, "Expected mold name after 'mold'.")) {
        return NULL;
    }

    name = atlas_strdup(parser->previous.lexeme);
    if (name == NULL) {
        parser_error(parser, "Out of memory while reading mold name.");
        return NULL;
    }

    if (match(parser, TOKEN_ARROW)) {
        if (!consume(parser, TOKEN_IDENTIFIER, "Expected parent mold name after '<-'.")) {
            free(name);
            return NULL;
        }
        parent_name = atlas_strdup(parser->previous.lexeme);
        if (parent_name == NULL) {
            free(name);
            parser_error(parser, "Out of memory while reading parent mold name.");
            return NULL;
        }
    }

    if (!consume(parser, TOKEN_LBRACE, "Expected '{' to start mold body.")) {
        free(parent_name);
        free(name);
        return NULL;
    }

    field_init_list_init(&fields);

    while (!check(parser, TOKEN_RBRACE) && !check(parser, TOKEN_EOF)) {
        Expr *value;

        if (!consume(parser, TOKEN_SEED, "Mold fields must use 'seed'.")) {
            field_init_list_free(&fields);
            free(parent_name);
            free(name);
            return NULL;
        }

        if (!consume(parser, TOKEN_IDENTIFIER, "Expected mold field name.")) {
            field_init_list_free(&fields);
            free(parent_name);
            free(name);
            return NULL;
        }

        {
            char *field_name = atlas_strdup(parser->previous.lexeme);
            if (field_name == NULL) {
                field_init_list_free(&fields);
                free(parent_name);
                free(name);
                parser_error(parser, "Out of memory while reading mold field name.");
                return NULL;
            }

            if (!consume(parser, TOKEN_ARROW, "Expected '<-' after mold field name.")) {
                free(field_name);
                field_init_list_free(&fields);
                free(parent_name);
                free(name);
                return NULL;
            }

            value = expression(parser);
            if (value == NULL) {
                free(field_name);
                field_init_list_free(&fields);
                free(parent_name);
                free(name);
                return NULL;
            }

            if (!consume(parser, TOKEN_SEMICOLON, "Expected ';' after mold field declaration.")) {
                expr_free(value);
                free(field_name);
                field_init_list_free(&fields);
                free(parent_name);
                free(name);
                return NULL;
            }

            if (!field_init_list_push(&fields, field_name, value)) {
                expr_free(value);
                free(field_name);
                field_init_list_free(&fields);
                free(parent_name);
                free(name);
                parser_error(parser, "Out of memory while storing mold field.");
                return NULL;
            }

            free(field_name);
        }
    }

    if (!consume(parser, TOKEN_RBRACE, "Expected '}' after mold body.")) {
        field_init_list_free(&fields);
        free(parent_name);
        free(name);
        return NULL;
    }

    stmt = stmt_mold_new(name, parent_name, &fields);
    free(parent_name);
    free(name);
    if (stmt == NULL) {
        field_init_list_free(&fields);
        parser_error(parser, "Out of memory while building mold declaration.");
        return NULL;
    }

    return stmt;
}

static Stmt *top_level_statement(Parser *parser) {
    if (match(parser, TOKEN_GLOBE)) {
        return globe_declaration(parser);
    }

    if (match(parser, TOKEN_MOLD)) {
        return mold_declaration(parser);
    }

    if (match(parser, TOKEN_IGNITE)) {
        return ignite_statement(parser);
    }

    parser_error(parser, "Top-level only allows 'globe', 'mold', and 'ignite'.");
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

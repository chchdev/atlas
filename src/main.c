#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "atlas/ast.h"
#include "atlas/parser.h"
#include "atlas/runtime.h"

static char *read_entire_file(const char *path) {
    FILE *f = fopen(path, "rb");
    long size;
    char *buffer;
    size_t read_size;

    if (f == NULL) {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }

    size = ftell(f);
    if (size < 0) {
        fclose(f);
        return NULL;
    }

    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }

    buffer = (char *)malloc((size_t)size + 1);
    if (buffer == NULL) {
        fclose(f);
        return NULL;
    }

    read_size = fread(buffer, 1, (size_t)size, f);
    if (read_size != (size_t)size) {
        free(buffer);
        fclose(f);
        return NULL;
    }

    buffer[size] = '\0';
    fclose(f);
    return buffer;
}

static int execute_source(Environment *env, const char *source) {
    Parser parser;
    Program program;
    Runtime runtime;
    int ok;

    parser_init(&parser, source);
    ok = parser_parse(&parser, &program);
    if (!ok) {
        fprintf(stderr, "Parse error: %s\n", parser.error_message);
        parser_destroy(&parser);
        return 0;
    }

    runtime_init(&runtime);
    ok = runtime_execute_program(&runtime, env, &program);
    if (!ok) {
        fprintf(stderr, "Runtime error: %s\n", runtime.error_message);
    }

    program_free(&program);
    parser_destroy(&parser);
    return ok;
}

static int run_file(Environment *env, const char *path) {
    char *source = read_entire_file(path);
    int ok;

    if (source == NULL) {
        fprintf(stderr, "Failed to read '%s': %s\n", path, strerror(errno));
        return 0;
    }

    ok = execute_source(env, source);
    free(source);
    return ok;
}

static void run_repl(Environment *env) {
    char line[2048];

    puts("Atlas REPL");
    puts("Type statements ending in ';'. Use Ctrl+C to exit.");

    while (fgets(line, sizeof(line), stdin) != NULL) {
        if (line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        (void)execute_source(env, line);
    }
}

int main(int argc, char **argv) {
    Environment env;
    int ok = 1;

    env_init(&env);

    if (argc > 2) {
        fprintf(stderr, "Usage: atlas [script.atlas]\n");
        env_free(&env);
        return 64;
    }

    if (argc == 2) {
        ok = run_file(&env, argv[1]);
    } else {
        run_repl(&env);
    }

    env_free(&env);
    return ok ? 0 : 1;
}

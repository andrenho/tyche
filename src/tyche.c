#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tyche.h"
#include "contrib/c-args-parser.h"

typedef struct Options {
    bool repl;
    bool print_bytecode;
    bool decompile_assembly;
    bool debug_assembly;
} Options;

//
// MAIN
//

static int execute(int argc, char **argv, void *user)
{
    assert(argc == 1);

    // load file

    FILE *f = fopen(argv[0], "rb");
    if (!f) {
        fprintf(stderr, "Could not open file '%s'.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END); long n = ftell(f); rewind(f);
    char *src = calloc(1, n + 1);
    if (fread(src, 1, n, f) != n) {
        fprintf(stderr, "Error reading source file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(f);

    // TODO - idenitfy file type
    // TODO - execute file (apply options)

    free(src);
    return CARGS_OK;
}

//
// COMMAND LINE OPTIONS
//

int cmd_print_bytecode(const char *value, void *user) {
    (void) value;
    ((Options *) user)->print_bytecode = true;
    return CARGS_OK;
}

int cmd_decompile_assembly(const char *value, void *user) {
    (void) value;
    ((Options *) user)->decompile_assembly = true;
    return CARGS_OK;
}

int cmd_debug_assembly(const char *value, void *user) {
    (void) value;
    ((Options *) user)->debug_assembly = true;
    return CARGS_OK;
}

int main(int argc, char* argv[])
{
    static const cargs_opt opts[] = {
            { "print-bytecode", 'B', CARGS_ARG_NONE, NULL, "Print bytecode", cmd_print_bytecode, NULL, NULL, 0, CARGS_ARG_NONE },
            { "decompile", 'D', CARGS_ARG_NONE, NULL, "Decompile assembly", cmd_decompile_assembly, NULL, NULL, 0, CARGS_ARG_NONE },
            { "debug-assembly", 'a', CARGS_ARG_NONE, NULL, "Print assembly commands as it runs", cmd_debug_assembly, NULL, NULL, 0, CARGS_ARG_NONE },
    };

    static const cargs_pos pos_schema[] = {
            { "source", "Source file (tyche source, assembly or bytecode)", 1, 1 }
    };

    static const cargs_cmd root = {
            NULL,
            "tyche: a programming language",
            .opts = opts,
            .opt_count = sizeof opts / sizeof opts[0],
            .subs = NULL,
            .sub_count = 0,
            .aliases = NULL,
            .alias_count = 0,
            .pos = pos_schema,
            .pos_count = sizeof pos_schema / sizeof pos_schema[0],
            .run = execute,
    };

    char version[10]; snprintf(version, 10, "%d.%d", VERSION_MAJOR, VERSION_MINOR);
    cargs_env env = {
            .prog         = "tyche",
            .version      = version,
            .author       = "",
            .auto_help    = true,
            .auto_version = true,
            .auto_author  = false,
            .wrap_cols    = 80,
            .color        = true,
            .out          = stdout,
            .err          = stderr
    };

    Options options;
    return cargs_dispatch(&env, &root, argc, argv, &options) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
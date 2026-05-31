#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tyche.h"
#include "c-args-parser.h"

typedef enum FileType {
    FT_SOURCE, FT_ASSEMBLY, FT_BINARY,
} FileType;

typedef struct Options {
    bool print_bytecode;
    bool decompile_assembly;
    bool debug_assembly;
    const char* output_file;
} Options;

#define TRY(cmd) { if ((cmd) != T_OK) { fprintf(stderr, "error: %s\n", tyc_last_error()); exit(EXIT_FAILURE); } }

//
// MAIN
//

static FileType identify_file_type(const char* src, size_t len)
{
    if (strlen(src) < 24)
        return FT_SOURCE;

    uint8_t const* bin = (uint8_t const *) src;
    if (bin[0] == 0xb1 && bin[1] == 0xe9 && bin[2] == 0xd6 && bin[3] == 0xa7)
        return FT_BINARY;

    size_t i = 0;
    while (i < len && isspace(src[i]))
        ++i;
    if (len > i + 9 && memcmp(&src[i], ".assembly", 9) == 0)
        return FT_ASSEMBLY;

    return FT_SOURCE;
}

static int execute(int argc, char **argv, void *user)
{
    assert(argc == 1);
    Options* opt = user;

    // load file

    FILE *f = fopen(argv[0], "rb");
    if (!f) {
        fprintf(stderr, "Could not open file '%s'.\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END); long n = ftell(f); rewind(f);
    char *src = calloc(1, (size_t) (n + 1));
    if (!src)
        abort();
    if (fread(src, 1, (size_t) n, f) != (size_t) n) {
        fprintf(stderr, "Error reading source file: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(f);

    FileType file_type = identify_file_type(src, (size_t) n);

    // TODO - possibly generate output

    TycheVM* T = tyc_new();

    if (file_type == FT_BINARY) {
        TRY(tyc_load_bytecode(T, (uint8_t const *) src, (size_t) n))
    } else if (file_type == FT_ASSEMBLY) {
        TRY(tyc_load_assembly(T, src))
    } else {
        fprintf(stderr, "Sorry, tyche code not supported yet.\n");
        exit(EXIT_FAILURE);
    }

    if (opt->print_bytecode)
        tyc_print_bytecode(T);
    if (opt->decompile_assembly)
        tyc_assembly_decompile(T);
    if (opt->debug_assembly)
        tyc_debug_to_console(T, true);

    TRY(tyc_call(T, 0))

    tyc_destroy(T);
    free(src);

    return CARGS_OK;
}

//
// COMMAND LINE OPTIONS
//

static int cmd_print_bytecode(const char *value, void *user) {
    (void) value;
    ((Options *) user)->print_bytecode = true;
    return CARGS_OK;
}

static int cmd_decompile_assembly(const char *value, void *user) {
    (void) value;
    ((Options *) user)->decompile_assembly = true;
    return CARGS_OK;
}

static int cmd_debug_assembly(const char *value, void *user) {
    (void) value;
    ((Options *) user)->debug_assembly = true;
    return CARGS_OK;
}

static int cmd_gen_bytecode(const char* value, void* user) {
    ((Options *) user)->output_file = value;
    return CARGS_OK;
}

int main(int argc, char* argv[])
{
    // TODO - option to execute vs compile

    static const cargs_opt opts[] = {
            { "output", 'o', CARGS_ARG_NONE, NULL, "Generate output file (bytecode)", cmd_gen_bytecode, NULL, NULL, 0, CARGS_ARG_OPTIONAL },
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

    Options options = {
            .debug_assembly = false,
            .decompile_assembly = false,
            .print_bytecode = false,
            .output_file = NULL,
    };
    return cargs_dispatch(&env, &root, argc, argv, &options) < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
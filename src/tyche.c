#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tyche.h"
#include "contrib/c-args-parser.h"
#include "usage.h"

typedef struct Options {
} Options;

//
// MAIN
//

static int execute(int argc, char **argv, void *user)
{
    return CARGS_OK;
}

//
// COMMAND LINE OPTIONS
//

int main(int argc, char* argv[])
{
    static const cargs_opt opts[] = {
            // { "help", 'h', CARGS_ARG_NONE, NULL, "Print this help", cmd_print_help, NULL, NULL, 0, CARGS_ARG_NONE },
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
            .pos = NULL,
            .pos_count = 0,
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
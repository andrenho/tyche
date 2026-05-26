#include "compiler_priv.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef enum { TS_NONE, TS_CONST, TS_FUNC } Section;

TYC_RESULT code_assemble(const char* code, uint8_t** bytecode, size_t* bytecode_sz)
{
    return T_ERR;
}

char* remove_comments_and_trim(char* line)
{
    /* Cut off the comment: terminate at the first ';'. */
    char *semi = strchr(line, ';');
    if (semi)
        *semi = '\0';

    /* Find first non-whitespace. */
    char *start = line;
    while (*start && isspace((unsigned char)*start))
        start++;

    /* Find end, trimming trailing whitespace. */
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)(end[-1])))
        end--;

    size_t len = (size_t)(end - start);

    /* Shift back to the base of the buffer if needed. */
    if (start != line)
        memmove(line, start, len);
    line[len] = '\0';

    return line;
}

TYC_RESULT assemble(const char* code, Assembly* assembly)
{
    Section section = TS_NONE;

    // for each line
    for (const char* start = code, *p = code; ; ++p) {
        if (*p == '\n' || *p == '\0') {
            char* line = xcalloc(1, p - start + 1); memcpy(line, start, p - start);
            // printf("--> %s\n", line);

            // remove comments and trim
            line = remove_comments_and_trim(line);

            // skip empty line
            if (line[0] == '\0')
                goto skip;

            // if .const
            if (strcmp(line, ".const") == 0) {
                section = TS_CONST;
            }

            // if .func
            else if (1 == 0 /* TODO - check for .func */) {
                // TODO
            }

            // constants
            else if (section == TS_CONST) {
                // TODO
            }

            // functions
            else if (section == TS_FUNC) {
                // TODO
            }

            // end of loop
skip:
            free(line);
            if (!*p) break;
            start = p + 1;
        }
    }

    return T_OK;
}
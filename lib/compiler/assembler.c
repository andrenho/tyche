#include "compiler_priv.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

typedef enum { TS_NONE, TS_CONST, TS_FUNC } Section;

typedef enum { TA_DIRECTIVE, TA_NUMBER, TA_COLON, TA_FLOAT, TA_STRING, TA_INSTRUCTION } TokenType;

typedef struct {
    TokenType type;
    union {
        char*   s;
        int32_t i;
        double  d;
    } v;
} Token;

static TYC_RESULT assembler_tokenize_line(const char* line, size_t line_no, Token* tokens, size_t* n_tokens)
{
    TYC_RESULT r = T_OK;
    size_t i = 0;
    const char* c = line;

    while (*c != '\0' && i < *n_tokens) {
        while (*c && isspace(*c)) ++c;    // skip spaces

        // if comment, return
        if (*c == ';')
            break;

        if (*c == '.') {
            const char* start = c; while (*c && (*c == '.' || isalpha(*c))) ++c;
            char* token = xcalloc(1, c - start + 1); memcpy(token, start, c - start);
            tokens[i++] = (Token) { .type = TA_DIRECTIVE, .v.s = token };
        }

        else if (*c == ':') {
            tokens[i++] = (Token) { .type = TA_COLON };
            ++c;
        }

        else if (*c == '"') {
            const char* start = ++c; while (*c && *c != '"') ++c;
            ++c;
            char* token = xcalloc(1, c - start); memcpy(token, start, c - start - 1);
            tokens[i++] = (Token) { .type = TA_STRING, .v.s = token };
        }

        else if (isdigit(*c)) {
            TokenType type = TA_NUMBER;
            const char* start = c;
            while (*c && (*c == '.' || isdigit(*c))) {
                if (*c == '.')
                    type = TA_FLOAT;
                ++c;
            }
            char token[c - start + 1]; memcpy(token, start, c - start); token[c - start] = '\0';
            if (type == TA_NUMBER)
                tokens[i++] = (Token) { .type = TA_NUMBER, .v.i = (int32_t) strtol(token, NULL, 10) };
            else
                tokens[i++] = (Token) { .type = TA_FLOAT, .v.d = strtod(token, NULL) };
        }

        else if (isalpha(*c)) {
            const char* start = c; while (*c && isalpha(*c)) ++c;
            char* token = xcalloc(1, c - start + 1); memcpy(token, start, c - start);
            tokens[i++] = (Token) { .type = TA_INSTRUCTION, .v.s = token };
        }

        else {
            ERROR("Assembler error on line %zu.", line_no)
        }
    }

    *n_tokens = i;
    return r;
}

TYC_RESULT assemble(const char* code, Assembly* assembly)
{
    Section section = TS_NONE;

    // for each line
    size_t line_no = 1;
    for (const char* start = code, *p = code; ; ++p) {
        if (*p == '\n' || *p == '\0') {
            size_t n_tokens = 5;
            Token tokens[n_tokens];

            char* line = xcalloc(1, p - start + 1); memcpy(line, start, p - start);
            assembler_tokenize_line(line, line_no, tokens, &n_tokens);

            // printf("--> %s\n", line);

            // remove comments and trim
            // line = remove_comments_and_trim(line);

            // skip empty line
            if (line[0] == '\0')
                goto skip;

            // if .const
            /*
            if (strcmp(line, ".const") == 0) {
                section = TS_CONST;
            }

            // if .func
            else if (1 == 0 / TODO - check for .func /) {
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
             */

            // end of loop
skip:
            free(line);
            if (!*p) break;
            start = p + 1;
        }

        ++line_no;
    }

    return T_OK;
}
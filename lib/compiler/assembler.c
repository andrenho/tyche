#include "compiler_priv.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"

typedef enum { TS_NONE, TS_CONST, TS_FUNC } Section;

typedef enum { TA_DIRECTIVE, TA_NUMBER, TA_COLON, TA_REAL, TA_STRING, TA_INSTRUCTION, TA_LABEL } TokenType;

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

        else if (*c == '@') {
            const char* start = c; while (*c && (*c == '@' || *c == ':' || isalpha(*c))) ++c;
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

        else if (isdigit(*c) || *c == '-') {
            TokenType type = TA_NUMBER;
            const char* start = c;
            while (*c && (*c == '.' || *c == '-' || isdigit(*c))) {
                if (*c == '.')
                    type = TA_REAL;
                ++c;
            }
            char token[c - start + 1]; memcpy(token, start, c - start); token[c - start] = '\0';
            if (type == TA_NUMBER)
                tokens[i++] = (Token) { .type = TA_NUMBER, .v.i = (int32_t) strtol(token, NULL, 10) };
            else
                tokens[i++] = (Token) { .type = TA_REAL, .v.d = strtod(token, NULL) };
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

TYC_RESULT assemble(const char* code, Assembly* as)
{
    Section section = TS_NONE;
    int f_id = -1;

    // for each line
    size_t line_no = 0;
    for (const char* start = code, *p = code; ; ++p) {
        if (*p == '\n' || *p == '\0') {
            ++line_no;
            size_t n_tokens = 5;
            Token tokens[n_tokens];

            char* line = xcalloc(1, p - start + 1); memcpy(line, start, (p - start));
            assembler_tokenize_line(line, line_no, tokens, &n_tokens);

            // skip empty line
            if (n_tokens == 0)
                goto skip;

            // if .const
            if (n_tokens == 1 && tokens[0].type == TA_DIRECTIVE && strcmp(tokens[0].v.s, ".const") == 0) {
                section = TS_CONST;
            }

            // if .func
            else if (n_tokens == 2 && tokens[0].type == TA_DIRECTIVE && strcmp(tokens[0].v.s, ".func") == 0 && tokens[1].type == TA_NUMBER) {
                f_id = tokens[1].v.i;
                if (f_id < 0)
                    ERROR("Function must be a positive number")
                assembly_add_function(as, (uint32_t) f_id);
                section = TS_FUNC;
            }

            // constants
            else if (section == TS_CONST && n_tokens == 3 && tokens[0].type == TA_NUMBER && tokens[1].type == TA_COLON && (tokens[2].type == TA_REAL || tokens[2].type == TA_STRING)) {
                if (tokens[0].v.i < 0)
                    ERROR("Const must be a positive number")
                if (tokens[2].type == TA_REAL)
                    assembly_add_const_real(as, (uint32_t) tokens[0].v.i, tokens[2].v.d);
                else
                    assembly_add_const_str(as, (uint32_t) tokens[0].v.i, tokens[2].v.s);
            }

            // functions
            else if (section == TS_FUNC && tokens[0].type == TA_INSTRUCTION) {
                TYC_INST inst = instruction_from_name(tokens[0].v.s);
                if (inst == TO_UNKNOWN)
                    ERROR("Invalid instruction '%s' in line %zu.", tokens[0].v.s, line_no)
                if (inst < PARAMETER_INST) {
                    assembly_add_inst(as, (uint32_t) f_id, inst);
                } else {
                    if (n_tokens != 2 || tokens[1].type != TA_NUMBER)
                        ERROR("Parameter invalid or missing on line %zu", line_no)
                    assembly_add_inst_p(as, (uint32_t) f_id, inst, tokens[1].v.i);
                }

                // TODO - labels
            }

            else {
                ERROR("Syntax error in line %zu", line_no)
            }

            // end of loop
skip:
            for (size_t i = 0; i < n_tokens; ++i)
                if (tokens[i].type == TA_DIRECTIVE || tokens[i].type == TA_INSTRUCTION || tokens[i].type == TA_LABEL)
                    free(tokens[i].v.s);
            free(line);
            if (!*p) break;
            start = p + 1;
        }
    }

    return T_OK;
}

#pragma GCC diagnostic pop

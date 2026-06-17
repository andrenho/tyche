#include "../lib/priv.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "toml-c.h"

static void assert_result(toml_table_t* item, TycheVM* T)
{
    toml_value_t t;

    if ((t = toml_table_int(item, "expected_stack_size")), t.ok) {
        assert(tyc_stack_size(T) == (size_t) t.u.i);
    }

    const char* SS = "expected_stack_top";
    if ((t = toml_table_int(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_INTEGER);
        int32_t v; assert(tyc_tointeger(T, -1, &v) == TYC_OK); assert(v == t.u.i);
    } else if ((t = toml_table_double(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_REAL);
        double v; assert(tyc_toreal(T, -1, &v) == TYC_OK); assert(fabs(v - t.u.d) < 0.0001);
    } else if ((t = toml_table_string(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_STRING);
        const char* str; assert(tyc_tostring(T, -1, &str) == TYC_OK); assert(strcmp(str, t.u.s) == 0);
        free(t.u.s);
    } else if ((t = toml_table_bool(item, SS)), t.ok) {
        TYC_TYPE type; assert(tyc_type(T, -1, &type) == TYC_OK); assert(type == TYC_BOOLEAN);
        bool v; assert(tyc_toboolean(T, -1, &v) == TYC_OK); assert(v == t.u.b);
    }
}

static void run_test_code(toml_table_t* item, const char* code, bool debug, bool decompile, bool debug_bytecode)
{
    TycheVM* T = tyc_new();
    tyc_debug_to_console(T, debug);

    // load code
    uint8_t* bytecode; size_t bytecode_sz;
    assert(code_assemble(code, &bytecode, &bytecode_sz) == TYC_OK);

    // run code
    assert(tyc_load_bytecode(T, bytecode, bytecode_sz) == TYC_OK);
    if (debug_bytecode)
        tyc_print_bytecode(T);
    if (decompile)
        tyc_assembly_decompile(T);
    assert(tyc_call(T, 0) == TYC_OK);

    // assert
    assert_result(item, T);

    // cleanup
    free(bytecode);
    tyc_destroy(T);
}

static char* transform_code(const char* template, toml_array_t* pars)
{
    toml_value_t t;
    char* r = CALLOC(1, strlen(template) + 100);
    const char* pt = template;
    char* pr = r;
    int i = 0;

    while (*pt) {
        if (*pt == '%') {
            int j = 0;
            if ((t = toml_array_int(pars, i)), t.ok) {
                j = sprintf(pr, "%ld", t.u.i);
            } else if ((t = toml_array_double(pars, i)), t.ok) {
                j = sprintf(pr, "%f", t.u.d);
            } else if ((t = toml_array_string(pars, i)), t.ok) {
                j = sprintf(pr, "%s", t.u.s);
                free(t.u.s);
            }
            ++i;
            pr += j;
        } else {
            *pr = *pt;
            ++pr;
        }
        ++pt;
    }

    return r;
}

static void run_test_template(toml_table_t* item, const char* template, bool debug, bool decompile, bool debug_bytecode)
{
    toml_array_t* a;

    if ((a = toml_table_array(item, "scenarios")), !a)
        abort();

    for (int i = 0; i < toml_array_len(a); ++i) {
        toml_table_t* scenario = toml_array_table(a, i);

        toml_value_t t;
        if ((t = toml_table_string(scenario, "name")), t.ok) {
            printf("  - %s\n", t.u.s);
            free(t.u.s);
        }

        toml_array_t* parameters = toml_table_array(scenario, "parameters");
        assert(parameters);
        char* code = transform_code(template, parameters);

        run_test_code(scenario, code, debug, decompile, debug_bytecode);

        free(code);
    }
}

static void run_assembly_test(const char* key, toml_table_t* item)
{
    toml_value_t t;
    printf("## %s\n", key);

    bool debug = false;
    if ((t = toml_table_bool(item, "debug")), t.ok)
        debug = t.u.b;

    bool decompile = false;
    if ((t = toml_table_bool(item, "decompile")), t.ok)
        decompile = t.u.b;

    bool debug_bytecode = false;
    if ((t = toml_table_bool(item, "debug_bytecode")), t.ok)
        debug_bytecode = t.u.b;

    if ((t = toml_table_string(item, "code")), t.ok) {
        run_test_code(item, t.u.s, debug, decompile, debug_bytecode);
        free(t.u.s);
    }

    if ((t = toml_table_string(item, "template")), t.ok) {
        run_test_template(item, t.u.s, debug, decompile, debug_bytecode);
        free(t.u.s);
    }
}

int main()
{
    FILE* f = fopen("./test/tests-op.toml", "r");
    if (!f) {
        fprintf(stderr, "Can't read TOML file.\n");
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char* buf = CALLOC(1, (size_t) size+1);
    (void) fread(buf, (size_t) size, 1, f);
    fclose(f);

    char errbuf[200];
    toml_table_t *tbl = toml_parse(buf, errbuf, sizeof(errbuf));
    if (!tbl) {
        fprintf(stderr, "ERROR: %s\n", errbuf);
        exit(1);
    }

    for (int i = 0; i < toml_table_len(tbl); i++) {
        int keylen;
        const char *key = toml_table_key(tbl, i, &keylen);
        toml_table_t *subtbl = toml_table_table(tbl, key);
        run_assembly_test(key, subtbl);
    }

    toml_free(tbl);
    free(buf);
}
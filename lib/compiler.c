#include "priv.h"

#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "compiler/compiler.lua.h"

TYC_RESULT code_assemble(const char* code, uint8_t** bytecode, size_t* bytecode_sz)
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);

    int r = luaL_loadbufferx(L, (const char *) lib_compiler_compiler_out, lib_compiler_compiler_out_len, "compiler", "b");
    if (r == LUA_ERRSYNTAX)
        abort();
    else if (r == LUA_ERRMEM)
        out_of_memory();

    lua_call(L, 0, 1);

    lua_pushstring(L, code);
    r = lua_pcall(L, 1, 1, 0);
    if (r == LUA_ERRMEM) {
        out_of_memory();
    } else if (r == LUA_ERRERR) {
        abort();
    } else if (r == LUA_ERRRUN) {
        fprintf(stderr, "%s\n", lua_tostring(L, -1));
        ERROR("Assembler: syntax error")
    }

    if (!lua_isstring(L, -1))
        abort();
    *bytecode_sz = (size_t) luaL_len(L, -1);
    *bytecode = malloc(*bytecode_sz);
    memcpy(*bytecode, lua_tostring(L, -1), *bytecode_sz);

    lua_close(L);
    return T_OK;
}

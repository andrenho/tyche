#!/usr/bin/env lua

local instructions = {
    -- stack operations
    pushi   = 0xa0,
    pushc   = 0xa1,
    pushf   = 0xa2,
    pushn   = 0x00,
    pushz   = 0x01,
    pusht   = 0x02,
    newa    = 0x03,
    newt    = 0x04,
    pop     = 0x05,
    dup     = 0x06,

    -- variables
    pushv   = 0xa3,
    set     = 0xae,
    dupv    = 0xa4,
    glbl    = 0x07,

    -- function operations
    call    = 0xa7,
    ret     = 0x10,
    retn    = 0x11,

    -- table and array operations
    getkv   = 0x16,
    setkv   = 0x17,
    geti    = 0xa8,
    seti    = 0xa9,
    appnd   = 0x18,
    next    = 0x19,
    sptb    = 0x1a,

    -- logical/arithmetic
    sum     = 0x20,
    sub     = 0x21,
    mul     = 0x22,
    div     = 0x23,
    idiv    = 0x24,
    mod     = 0x25,
    eq      = 0x26,
    neq     = 0x27,
    lt      = 0x28,
    lte     = 0x29,
    gt      = 0x2a,
    gte     = 0x2b,
    ['and'] = 0x2c,
    ['or']  = 0x2d,
    xor     = 0x2e,
    pow     = 0x2f,
    shl     = 0x30,
    shr     = 0x31,
    ['not'] = 0x32,
    neg     = 0x33,

    -- other value operations
    len     = 0x40,
    type    = 0x41,
    ver     = 0x42,

    -- external code
    -- TODO

    -- control flow
    bz      = 0xca,
    bnz     = 0xcb,
    bnil    = 0xcf,
    jmp     = 0xcc,

    -- memory management
    gc      = 0x4b,

    -- error management
    pushe   = 0xcd,
    pope    = 0x5a,
    thrw    = 0x5b,
}

-----------------------------------------------------------------------------------

--
-- prepare
--

local sorted_opcodes = {}
for _, v in pairs(instructions) do table.insert(sorted_opcodes, v) end
table.sort(sorted_opcodes)

local instructions_inv = {}
for k, v in pairs(instructions) do instructions_inv[v] = k end

--
-- generate header
--

local header = {}
local function H(txt) table.insert(header, txt) end

H [[#ifndef TYCHEVM_INSTRUCTIONS_H_
#define TYCHEVM_INSTRUCTIONS_H_

#include <stddef.h>
#include <stdint.h>

#define PARAMETER_INST 0xa0

#define TO_16BIT 0x20
#define TO_32BIT 0x40

typedef enum TYC_INST {]]

for _, opcode in ipairs(sorted_opcodes) do
    H(string.format('    TO_%-6s = 0x%02x,', instructions_inv[opcode]:upper(), string.format('0x%02x', opcode)))
end

H [[    TO_UNKNOWN = -1,
} TYC_INST;

const char* instruction_name(TYC_INST inst);
TYC_INST    instruction_from_name(const char* name);
size_t      instruction_size(TYC_INST inst, int32_t operand);

#endif  // TYCHEVM_INSTRUCTIONS_H_
]]

-- print(table.concat(header, '\n'))

--
-- generate source
--

local source = {}
local function S(txt) table.insert(source, txt) end

S [[#include "instructions.h"

#include <string.h>

const char* instruction_name(TYC_INST inst)
{
    switch (inst) { ]]

for _, opcode in ipairs(sorted_opcodes) do
    S(string.format('        case TO_%-5s: return "%-8s";', instructions_inv[opcode]:upper(), instructions_inv[opcode]))
end

S [[        case TO_UNKNOWN: default      : return "????    ";
    };
}

TYC_INST instruction_from_name(const char* name)
{ ]]

for _, opcode in ipairs(sorted_opcodes) do
    S(string.format([[    if (strcmp(name, "%s") == 0) return TO_%s;]], instructions_inv[opcode], instructions_inv[opcode]:upper()))
end

S [[    return TO_UNKNOWN;
}

size_t instruction_size(TYC_INST inst, int32_t operand)
{
    if (inst >= PARAMETER_INST + TO_32BIT)
        return 5;
    if (inst >= PARAMETER_INST + TO_16BIT)
        return 3;
    if (inst >= PARAMETER_INST)
        return 2;
    return 1;
}

]]

--
-- OUTPUT FILES
--

local f = assert(io.open('instructions.h', 'w')); f:write(table.concat(header, '\n')); f:close()
local f = assert(io.open('instructions.c', 'w')); f:write(table.concat(source, '\n')); f:close()

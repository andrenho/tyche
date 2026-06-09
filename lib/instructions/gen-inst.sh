#!/bin/sh
# Generates instructions.h and instructions.c for TycheVM.

set -eu

# ---------------------------------------------------------------------------
# instruction table: "name opcode" pairs
# ---------------------------------------------------------------------------
INSTRUCTIONS='
pushi a0
pushc a1
pushf a2
pushn 00
pushz 01
pusht 02
newa 03
newt 04
pop 05
dup 06
pushv a3
set ae
dupv a4
glbl 07
call a7
ret 10
retn 11
getkv 16
setkv 17
geti a8
seti a9
appnd 18
next 19
sptb 1a
sum 20
sub 21
mul 22
div 23
idiv 24
mod 25
eq 26
neq 27
lt 28
lte 29
gt 2a
gte 2b
and 2c
or 2d
xor 2e
pow 2f
shl 30
shr 31
not 32
neg 33
hash 34
len 40
type 41
ver 42
bz ca
bnz cb
bnil cf
jmp cc
gc 4b
pushe cd
pope 5a
thrw 5b
'

JUMP_INSTRUCTIONS='bz bnz bnil jmp pushe'

# ---------------------------------------------------------------------------
# prepare: build a list sorted by opcode value -> "DEC HEX NAME"
# ---------------------------------------------------------------------------
SORTED=$(
    echo "$INSTRUCTIONS" | while read -r name hex; do
        [ -z "$name" ] && continue
        dec=$(printf '%d' "0x$hex")
        printf '%d %s %s\n' "$dec" "$hex" "$name"
    done | sort -n
)

upper() { printf '%s' "$1" | tr '[:lower:]' '[:upper:]'; }

# ---------------------------------------------------------------------------
# generate header
# ---------------------------------------------------------------------------
{
cat <<'EOF'
#ifndef TYCHEVM_INSTRUCTIONS_H_
#define TYCHEVM_INSTRUCTIONS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PARAMETER_INST 0xa0

#define TO_16BIT 0x20
#define TO_32BIT 0x40

typedef enum TYC_INST {
EOF

echo "$SORTED" | while read -r dec hex name; do
    printf '    TO_%-6s = 0x%s,\n' "$(upper "$name")" "$hex"
done

cat <<'EOF'
    TO_UNKNOWN = -1,
} TYC_INST;

const char* instruction_name(TYC_INST inst);
TYC_INST    instruction_from_name(const char* name);
size_t      instruction_size(TYC_INST inst);
bool        is_16bit_instruction(TYC_INST inst);

#endif  // TYCHEVM_INSTRUCTIONS_H_
EOF
} > instructions.h

# ---------------------------------------------------------------------------
# generate source
# ---------------------------------------------------------------------------
{
cat <<'EOF'
#include "instructions.h"

#include <string.h>

const char* instruction_name(TYC_INST inst)
{
    switch (inst) {
EOF

echo "$SORTED" | while read -r dec hex name; do
    printf '        case TO_%-5s: return "%-8s";\n' "$(upper "$name")" "$name"
done

cat <<'EOF'
        case TO_UNKNOWN: default      : return "????    ";
    };
}

TYC_INST instruction_from_name(const char* name)
{
EOF

echo "$SORTED" | while read -r dec hex name; do
    printf '    if (strcmp(name, "%s") == 0) return TO_%s;\n' "$name" "$(upper "$name")"
done

cat <<'EOF'
    return TO_UNKNOWN;
}

size_t instruction_size(TYC_INST inst)
{
    if (inst >= PARAMETER_INST + TO_32BIT)
        return 5;
    if (inst >= PARAMETER_INST + TO_16BIT)
        return 3;
    if (inst >= PARAMETER_INST)
        return 2;
    return 1;
}

bool is_16bit_instruction(TYC_INST inst)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wswitch-enum"
    switch (inst) {
EOF

for inst in $JUMP_INSTRUCTIONS; do
    printf '        case TO_%s: return true;\n' "$(upper "$inst")"
done

cat <<'EOF'
        default: return false;
    }
#pragma GCC diagnostic pop
}
EOF
} > instructions.c
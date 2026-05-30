#include "assembler_priv.h"

typedef struct Binary {
    uint8_t* data;
    size_t   sz;
    size_t   cap;
} Binary;

static size_t extend_bin(Binary* bin, size_t n_bytes)
{
    size_t sz = bin->sz;
    if (bin->sz + n_bytes >= bin->cap) {
        bin->cap *= 2;
        bin->data = xrealloc(bin->data, bin->cap);
    }
    bin->sz += n_bytes;
    return sz;
}

static void add_8(Binary* bin, uint8_t value)
{
    size_t i = extend_bin(bin, 1);
    bin->data[i] = value;
}

static void add_16(Binary* bin, uint16_t value)
{
    size_t i = extend_bin(bin, 2);
    bin->data[i] = (uint8_t) value;
    bin->data[i+1] = (uint8_t) (value >> 8);
}

static void add_32(Binary* bin, uint32_t value)
{
    size_t i = extend_bin(bin, 4);
    bin->data[i] = (uint8_t) value;
    bin->data[i+1] = (uint8_t) (value >> 8);
    bin->data[i+2] = (uint8_t) (value >> 16);
    bin->data[i+3] = (uint8_t) (value >> 24);
}

static void set_32(Binary* bin, size_t pos, uint32_t value)
{
    bin->data[pos] = (uint8_t) value;
    bin->data[pos+1] = (uint8_t) (value >> 8);
    bin->data[pos+2] = (uint8_t) (value >> 16);
    bin->data[pos+3] = (uint8_t) (value >> 24);
}

static void add_64(Binary* bin, uint64_t value)
{
    size_t i = extend_bin(bin, 8);
    bin->data[i] = (uint8_t) value;
    bin->data[i+1] = (uint8_t) (value >> 8);
    bin->data[i+2] = (uint8_t) (value >> 16);
    bin->data[i+3] = (uint8_t) (value >> 24);
    bin->data[i+4] = (uint8_t) (value >> 32);
    bin->data[i+5] = (uint8_t) (value >> 40);
    bin->data[i+6] = (uint8_t) (value >> 48);
    bin->data[i+7] = (uint8_t) (value >> 56);
}

static void add_str(Binary* bin, const char* str)
{
    size_t len = strlen(str);
    size_t i = extend_bin(bin, len + 1);
    memcpy(&bin->data[i], str, len + 1);
}

TYC_RESULT bytecode_gen(Assembly const* as, uint8_t** bytecode, size_t* bytecode_sz)
{
    Binary bin = {
        .data = xmalloc(1024),
        .sz = 0,
        .cap = 1024,
    };

    // header
    add_32(&bin, MAGIC);
    add_8(&bin, VERSION_MINOR);
    add_8(&bin, VERSION_MAJOR);
    add_16(&bin, 0);

    // constants
    size_t CODE_START_ADDR = bin.sz;
    add_32(&bin, 0);  // placeholder for code start
    add_32(&bin, (uint32_t) as->consts_n);
    for (size_t i = 0; i < as->consts_n; ++i) {
        if (as->consts[i].type == TC_REAL) {
            uint64_t bytes;
            memcpy(&bytes, &as->consts[i].value.real, 8);  // TODO - this will break if big endian
            add_8(&bin, TC_REAL);
            add_64(&bin, bytes);
        } else {
            add_8(&bin, TC_STRING);
            add_str(&bin, as->consts[i].value.string);
        }
    }
    set_32(&bin, CODE_START_ADDR, (uint32_t) bin.sz); // update code start

    // code
    add_32(&bin, 0);  // placeholder for debugging info
    add_32(&bin, (uint32_t) as->functions_n);
    for (size_t i = 0; i < as->functions_n; ++i) {
        size_t NEXT_FUNCTION_ADDR = bin.sz;
        add_32(&bin, 0);   // placeholder for next function addr
        for (size_t j = 0; j < as->functions[i].n_instructions; ++j) {
            AssemblyInstruction* inst = &as->functions[i].instructions[j];
            if (inst->operator.type == OP_LABEL)
                ERROR("Label '%s' was left over in assembly. This is a tyche bug.", inst->operator.v.label)
            if (inst->operator.type == OP_NONE) {
                add_8(&bin, inst->instruction);
            } else if (inst->operator.type == OP_INT) {
                if (is_16bit_instruction(inst->instruction)) {
                    add_8(&bin, inst->instruction);
                    add_16(&bin, (uint16_t) (inst->operator.v.i));
                } else if (inst->operator.v.i >= -128 && inst->operator.v.i <= 127) {
                    add_8(&bin, inst->instruction);
                    add_8(&bin, (uint8_t) inst->operator.v.i);
                } else if (inst->operator.v.i >= -32768 && inst->operator.v.i <= 32767) {
                    add_8(&bin, (uint8_t) (inst->instruction + TO_16BIT));
                    add_16(&bin, (uint16_t) (inst->operator.v.i));
                } else {
                    add_8(&bin, (uint8_t) (inst->instruction + TO_32BIT));
                    add_32(&bin, (uint32_t) (inst->operator.v.i));
                }
            }
        }

        set_32(&bin, NEXT_FUNCTION_ADDR, (uint32_t) bin.sz); // update next function
    }

    bin.data = xrealloc(bin.data, bin.sz);
    *bytecode = bin.data;
    *bytecode_sz = bin.sz;

    return T_OK;
}
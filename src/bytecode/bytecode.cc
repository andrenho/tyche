#include "bytecode.hh"

#include "bc_exceptions.hh"
#include "../common/overloaded.hh"

namespace tyche::bc {

Bytecode::Bytecode(ByteArray ba)
    : byte_array_(std::move(ba))
{
    // check file size
    if (byte_array_.size() < (TOC_START + TOC_SZ))
        throw BytecodeParsingError("Invalid bytecode format (file too short)");

    // check magic number and version
    if (byte_array_.get_uint32(0) != MAGIC_NUMBER)
        throw BytecodeParsingError("Invalid bytecode format (magic number not matching)");
    if (byte_array_.get_uint32(4) != BYTECODE_VERSION)
        throw BytecodeParsingError("Unexpected bytecode format version");

    // load cache
    cache_.constants_idx_addr = byte_array_.get_uint32(TOC_START);
    cache_.n_constants = byte_array_.get_uint16(TOC_START + 4);
    cache_.functions_idx_addr = byte_array_.get_uint32(TOC_START + (1 * TOC_RECORD_SZ));
    cache_.n_functions = byte_array_.get_uint16(TOC_START + (1 * TOC_RECORD_SZ) + 4);
    cache_.constants_start_addr = byte_array_.get_uint32(TOC_START + (2 * TOC_RECORD_SZ));
    uint32_t code_start = byte_array_.get_uint32(TOC_START + (3 * TOC_RECORD_SZ));
    for (uint32_t i = 0; i < cache_.n_functions; ++i) {
        cache_.function_addr.emplace_back(code_start + byte_array_.get_uint32(cache_.functions_idx_addr + (i * FUNCTION_RECORD_SZ)));
        cache_.function_sz.emplace_back(byte_array_.get_uint32(cache_.functions_idx_addr + (i * FUNCTION_RECORD_SZ) + 8));
    }
}

uint32_t Bytecode::n_constants() const
{
    return cache_.n_constants;
}

uint32_t Bytecode::n_functions() const
{
    return cache_.n_functions;
}

ConstantValue Bytecode::get_constant(uint32_t idx) const
{
    uint32_t constant_idx = byte_array_.get_uint32(cache_.constants_idx_addr + (idx * CONST_RECORD_SZ));
    switch ((ConstantType) byte_array_.get_byte(cache_.constants_start_addr + constant_idx)) {
        case CONST_TYPE_FLOAT:
            return byte_array_.get_float(cache_.constants_start_addr + constant_idx + 1);
        case CONST_TYPE_STRING:
            return byte_array_.get_string_ptr(cache_.constants_start_addr + constant_idx + 1).first;
        default:
            throw BytecodeParsingError("Invalid bytecode format (invalid constant type)");
    }
}

Bytecode::FunctionDef Bytecode::get_function_def(uint32_t function_id) const
{
    uint32_t idx = cache_.functions_idx_addr + (function_id * FUNCTION_RECORD_SZ);
    return {
        .n_params = byte_array_.get_uint16(idx + 4),
        .locals = byte_array_.get_uint16(idx + 6),
    };
}

uint32_t Bytecode::get_function_sz(uint32_t function_id) const
{
    return cache_.function_sz.at(function_id);
}

uint8_t Bytecode::get_code_byte(uint32_t function_id, uint32_t idx) const
{
    return byte_array_.get_byte(cache_.function_addr.at(function_id) + idx);
}

int8_t Bytecode::get_code_int8(uint32_t function_id, uint32_t idx) const
{
    return byte_array_.get_int8(cache_.function_addr.at(function_id) + idx);
}

int16_t Bytecode::get_code_int16(uint32_t function_id, uint32_t idx) const
{
    return byte_array_.get_int16(cache_.function_addr.at(function_id) + idx);
}

int32_t Bytecode::get_code_int32(uint32_t function_id, uint32_t idx) const
{
    return byte_array_.get_int32(cache_.function_addr.at(function_id) + idx);
}

ByteArray Bytecode::generate(BytecodePrototype const& bp)
{
    // header section
    ByteArray header;
    header.set_uint32(0, MAGIC_NUMBER);
    header.set_byte(4, BYTECODE_VERSION);

    // constants
    ByteArray constant_indexes;
    ByteArray raw_constants;

    uint32_t idx = 0;
    for (auto const& constant: bp.constants) {
        constant_indexes.append_uint32(idx);
        std::visit(overloaded {
                [&](float f) {
                    raw_constants.append_byte(CONST_TYPE_FLOAT);
                    raw_constants.append_float(f);
                },
                [&](std::string const& s) {
                    raw_constants.append_byte(CONST_TYPE_STRING);
                    raw_constants.append_string(s);
                },
        }, constant);
        idx = raw_constants.size();
    }

    // functions
    ByteArray functions_indexes;
    ByteArray raw_code;

    uint32_t idx_idx = 0, code_idx = 0;
    for (auto const& f: bp.functions) {
        functions_indexes.set_uint32(idx_idx, code_idx);
        functions_indexes.set_uint16(idx_idx + 4, f.n_pars);
        functions_indexes.set_uint16(idx_idx + 6, f.n_locals);
        functions_indexes.set_uint32(idx_idx + 8, f.code.size());
        raw_code.append_bytearray(f.code);
        code_idx = raw_code.size();
        idx_idx += FUNCTION_RECORD_SZ;
    }

    // table of contents
    uint32_t function_idx_start = CONST_IDX_START + constant_indexes.size();
    uint32_t raw_constant_start = function_idx_start + functions_indexes.size();
    uint32_t raw_code_start = raw_constant_start + raw_constants.size();

    ByteArray toc;
    if (!bp.constants.empty()) {
        toc.set_uint32(SEC_CONST_IDX * TOC_RECORD_SZ, CONST_IDX_START);
        toc.set_uint32(SEC_CONST_IDX * TOC_RECORD_SZ + 4, constant_indexes.size() / CONST_RECORD_SZ);
        toc.set_uint32(SEC_CONST_DATA * TOC_RECORD_SZ, raw_constant_start);
        toc.set_uint32(SEC_CONST_DATA * TOC_RECORD_SZ + 4, raw_constants.size());
    }
    if (!bp.functions.empty()) {
        toc.set_uint32(SEC_FUNC_IDX * TOC_RECORD_SZ, function_idx_start);
        toc.set_uint32(SEC_FUNC_IDX * TOC_RECORD_SZ + 4, functions_indexes.size() / FUNCTION_RECORD_SZ);
        toc.set_uint32(SEC_CODE * TOC_RECORD_SZ, raw_code_start);
        toc.set_uint32(SEC_CODE * TOC_RECORD_SZ + 4, raw_code.size());
    }

    //
    // assemble bytecode
    //

    ByteArray ba;
    ba.set_bytearray(0, header);
    ba.set_bytearray(TOC_START, toc);
    ba.set_bytearray(CONST_IDX_START, constant_indexes);
    ba.set_bytearray(function_idx_start, functions_indexes);
    ba.set_bytearray(raw_constant_start, raw_constants);
    ba.set_bytearray(raw_code_start, raw_code);
    return ba;
}

}
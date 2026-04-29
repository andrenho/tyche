#ifndef TYCHE_BYTECODE_HH
#define TYCHE_BYTECODE_HH

#include "bytearray.hh"
#include "bytecodeprototype.hh"

namespace tyche {

class Bytecode {
public:
    Bytecode() = default;
    explicit Bytecode(ByteArray ba);

    [[nodiscard]] uint32_t n_constants() const;
    [[nodiscard]] uint32_t n_functions() const;

    [[nodiscard]] ConstantValue get_constant(uint32_t idx) const;

    struct FunctionDef { uint16_t n_params, locals; };
    [[nodiscard]] FunctionDef get_function_def(uint32_t function_id) const;
    [[nodiscard]] uint32_t    get_function_sz(uint32_t function_id) const;

    [[nodiscard]] uint8_t get_code_byte(uint32_t function_id, uint32_t idx) const;
    [[nodiscard]] int8_t  get_code_int8(uint32_t function_id, uint32_t idx) const;
    [[nodiscard]] int16_t get_code_int16(uint32_t function_id, uint32_t idx) const;
    [[nodiscard]] int32_t get_code_int32(uint32_t function_id, uint32_t idx) const;

    // TODO - debugging info

    [[nodiscard]] static ByteArray generate(BytecodePrototype const& bp);

private:
    ByteArray byte_array_;      // the actual data

    static constexpr uint8_t  BYTECODE_VERSION = 1;
    static constexpr uint32_t MAGIC_NUMBER = 0x74b3c138;
    static constexpr uint32_t TOC_START = 16,
                              TOC_N_RECORDS = 8,
                              TOC_RECORD_SZ = 8,
                              TOC_SZ = TOC_N_RECORDS * TOC_RECORD_SZ;
    static constexpr uint32_t CONST_IDX_START = TOC_START + TOC_SZ,
                              CONST_RECORD_SZ = 4;
    static constexpr uint32_t FUNCTION_RECORD_SZ = 12;

    enum Sections { SEC_CONST_IDX = 0, SEC_FUNC_IDX = 1, SEC_CONST_DATA = 2, SEC_CODE = 3 };

    // caching for faster reading of data
    struct Cache {
        uint32_t constants_idx_addr;
        uint16_t n_constants;
        uint32_t constants_start_addr;
        uint32_t functions_idx_addr;
        uint32_t n_functions;
        std::vector<uint32_t> function_addr;
        std::vector<uint32_t> function_sz;
    };
    Cache cache_ {};
};

}

#endif //TYCHE_BYTECODE_HH

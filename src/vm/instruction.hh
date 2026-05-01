#ifndef TYCHE_INSTRUCTION_HH
#define TYCHE_INSTRUCTION_HH

#include <cstdint>
#include <optional>
#include <string>
#include <utility>

#include "../bytecode/bytecode.hh"

namespace tyche::vm {

constexpr uint8_t OPCODE_NEXT_SIZE = 0x20;

enum class Instruction : uint8_t {

    // stack operations
    PushInt8        = 0xa0,
    PushInt16       = 0xc0,
    PushInt32       = 0xe0,
    PushConstant8   = 0xa1,
    PushConstant16  = 0xc1,
    PushConstant32  = 0xe1,
    PushZero        = 0x00,
    PushTrue        = 0x01,
    NewArray        = 0x02,
    NewTable        = 0x03,
    Pop             = 0x04,
    Duplicate       = 0x05,

    // local variables
    SetLocal8       = 0xa3,
    SetLocal16      = 0xc3,
    SetLocal32      = 0xe3,
    GetLocal8       = 0xa4,
    GetLocal16      = 0xc4,
    GetLocal32      = 0xe4,
    SetGlobal8      = 0xa5,
    SetGlobal16     = 0xc5,
    SetGlobal32     = 0xe5,
    GetGlobal8      = 0xa6,
    GetGlobal16     = 0xc6,
    GetGlobal32     = 0xe6,

    // function operations
    Call8           = 0xa7,
    Call16          = 0xc7,
    Call32          = 0xe7,
    Return          = 0x10,
    ReturnNil       = 0x11,

    // table and array operations
    GetKeyValue     = 0x16,
    SetKeyValue     = 0x17,
    GetArrayItem    = 0x18,
    SetArrayItem    = 0x19,
    Append          = 0x1a,
    Next            = 0x1b,
    SetMetatable    = 0x1c,
    GetMetatable    = 0x1d,

    // logical/arithmetic
    Sum             = 0x20,
    Subtract        = 0x21,
    Multiply        = 0x22,
    Divide          = 0x23,
    DivideInt       = 0x24,
    Equals          = 0x25,
    NotEquals       = 0x26,
    LessThan        = 0x27,
    LessThanEq      = 0x28,
    GreaterThan     = 0x29,
    GreaterThanEq   = 0x2a,
    And             = 0x2b,
    Or              = 0x2c,
    Xor             = 0x2d,

    // other value operations
    Len             = 0x30,
    Type            = 0x31,
    Cast            = 0x32,
    Version         = 0x33,

    // control flow
    BranchIfZero8     = 0xa8,
    BranchIfZero16    = 0xc8,
    BranchIfZero32    = 0xe8,
    BranchIfNotZero8  = 0xa9,
    BranchIfNotZero16 = 0xc9,
    BranchIfNotZero32 = 0xe9,
    Jump8             = 0xaa,
    Jump16            = 0xca,
    Jump32            = 0xea,

    // external code
    Compile         = 0x38,
    Assemble        = 0x39,
    Load            = 0x3a,
};

std::pair<std::string, size_t> debug_instruction(Instruction inst, int oper=0);
std::pair<std::string, size_t> debug_instruction(bc::Bytecode const& bt, uint32_t function_id, uint32_t addr);

enum class OperandType { NoOperand, Int8, Int16, Int32 };
OperandType instruction_operand_type(Instruction instruction);

std::optional<Instruction> translate_instruction(std::string const& txt, std::optional<int> op);

}

#endif //TYCHE_INSTRUCTION_HH

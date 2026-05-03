#include "instruction.hh"

#include <limits>
#include <unordered_map>

namespace tyche {

const std::unordered_map<std::string, Instruction> instruction_names = {
    { "pushi", Instruction::PushInt8 },
    { "pushc", Instruction::PushConstant8 },
    { "pushz", Instruction::PushZero },
    { "pusht", Instruction::PushTrue },
    { "pushf", Instruction::PushFunction8 },
    { "newa",  Instruction::NewArray },
    { "newt",  Instruction::NewTable },
    { "pop",   Instruction::Pop },
    { "dup",   Instruction::Duplicate },
    { "pushv", Instruction::PushValues8 },
    { "set",   Instruction::SetValue8 },
    { "dupv",  Instruction::DuplicateValue8 },
    { "setg",  Instruction::SetGlobal8 },
    { "getl",  Instruction::GetGlobal8 },
    { "call8", Instruction::Call8 },
    { "ret",   Instruction::Return },
    { "retn",  Instruction::ReturnNil },
    { "getkv", Instruction::GetKeyValue },
    { "setkv", Instruction::SetKeyValue },
    { "geta",  Instruction::GetArrayItem },
    { "seta",  Instruction::SetArrayItem },
    { "appnd", Instruction::Append },
    { "next",  Instruction::Next },
    { "smt",   Instruction::SetMetatable },
    { "mt",    Instruction::GetMetatable },
    { "sum",   Instruction::Sum },
    { "sub",   Instruction::Subtract },
    { "mul",   Instruction::Multiply },
    { "div",   Instruction::Divide },
    { "idiv",  Instruction::DivideInt },
    { "eq",    Instruction::Equals },
    { "neq",   Instruction::NotEquals },
    { "lt",    Instruction::LessThan },
    { "lte",   Instruction::LessThanEq },
    { "gt",    Instruction::GreaterThan },
    { "gte",   Instruction::GreaterThanEq },
    { "and",   Instruction::And },
    { "or",    Instruction::Or },
    { "xor",   Instruction::Xor },
    { "pow",   Instruction::Power },
    { "shl",   Instruction::ShiftLeft },
    { "shr",   Instruction::ShiftRight },
    { "mod",   Instruction::Modulo },
    { "len",   Instruction::Len },
    { "type",  Instruction::Type },
    { "cast",  Instruction::Cast },
    { "ver",   Instruction::Version },
    { "bz",    Instruction::BranchIfZero8 },
    { "bnz",   Instruction::BranchIfNotZero8 },
    { "jmp",   Instruction::Jump8 },
    { "cmpl",  Instruction::Compile },
    { "asmbl", Instruction::Assemble },
    { "load",  Instruction::Load },
};



std::pair<std::string, size_t> debug_instruction(Instruction inst, int oper)
{
    std::string out;
    switch (inst) {

        case Instruction::PushInt8:
        case Instruction::PushInt16:
        case Instruction::PushInt32:
            out = "pushi";
            break;
        case Instruction::PushConstant8:
        case Instruction::PushConstant16:
        case Instruction::PushConstant32:
            out = "pushc";
            break;
        case Instruction::PushFunction8:
        case Instruction::PushFunction16:
        case Instruction::PushFunction32:
            out = "pushf";
            break;
        case Instruction::PushZero:         out = "pushz"; break;
        case Instruction::PushTrue:         out = "pusht"; break;
        case Instruction::NewArray:         out = "newa"; break;
        case Instruction::NewTable:         out = "newt"; break;
        case Instruction::Pop:              out = "pop"; break;
        case Instruction::Duplicate:        out = "dup"; break;
        case Instruction::PushValues8:
        case Instruction::PushValues16:
        case Instruction::PushValues32:
            out = "pushv";
            break;
        case Instruction::SetValue8:
        case Instruction::SetValue16:
        case Instruction::SetValue32:
            out = "set";
            break;
        case Instruction::DuplicateValue8:
        case Instruction::DuplicateValue16:
        case Instruction::DuplicateValue32:
            out = "dupv";
            break;
        case Instruction::SetGlobal8:
        case Instruction::SetGlobal16:
        case Instruction::SetGlobal32:
            out = "setg";
            break;
        case Instruction::GetGlobal8:
        case Instruction::GetGlobal16:
        case Instruction::GetGlobal32:
            out = "getg";
            break;
        case Instruction::Call8:
        case Instruction::Call16:
        case Instruction::Call32:
            out = "call";
            break;
        case Instruction::Return:           out = "ret"; break;
        case Instruction::ReturnNil:        out = "retn"; break;
        case Instruction::GetKeyValue:      out = "getkv"; break;
        case Instruction::SetKeyValue:      out = "setkv"; break;
        case Instruction::GetArrayItem:     out = "geta"; break;
        case Instruction::SetArrayItem:     out = "seta"; break;
        case Instruction::Append:           out = "appnd"; break;
        case Instruction::Next:             out = "next"; break;
        case Instruction::SetMetatable:     out = "smt"; break;
        case Instruction::GetMetatable:     out = "mt"; break;
        case Instruction::Sum:              out = "sum"; break;
        case Instruction::Subtract:         out = "sub"; break;
        case Instruction::Multiply:         out = "mul"; break;
        case Instruction::Divide:           out = "div"; break;
        case Instruction::DivideInt:        out = "idiv"; break;
        case Instruction::Equals:           out = "eq"; break;
        case Instruction::NotEquals:        out = "neq"; break;
        case Instruction::LessThan:         out = "lt"; break;
        case Instruction::LessThanEq:       out = "lte"; break;
        case Instruction::GreaterThan:      out = "gt"; break;
        case Instruction::GreaterThanEq:    out = "gte"; break;
        case Instruction::And:              out = "and"; break;
        case Instruction::Or:               out = "or"; break;
        case Instruction::Xor:              out = "xor"; break;
        case Instruction::Power:            out = "pow"; break;
        case Instruction::ShiftLeft:        out = "shl"; break;
        case Instruction::ShiftRight:       out = "shr"; break;
        case Instruction::Modulo:           out = "mod"; break;
        case Instruction::Len:              out = "len"; break;
        case Instruction::Type:             out = "type"; break;
        case Instruction::Cast:             out = "cast"; break;
        case Instruction::Version:          out = "ver"; break;
        case Instruction::BranchIfZero8:
        case Instruction::BranchIfZero16:
        case Instruction::BranchIfZero32:
            out = "bz";
            break;
        case Instruction::BranchIfNotZero8:
        case Instruction::BranchIfNotZero16:
        case Instruction::BranchIfNotZero32:
            out = "bnz";
            break;
        case Instruction::Jump8:
        case Instruction::Jump16:
        case Instruction::Jump32:
            out = "jmp";
            break;
        case Instruction::Compile:          out = "cmpl"; break;
        case Instruction::Assemble:         out = "asmbl"; break;
        case Instruction::Load:             out = "load"; break;
        default:
            out = "???";
    }

    OperandType operands = instruction_operand_type(inst);

    if (operands == OperandType::NoOperand)
        return { out, 1 };

    out += " " + std::to_string(oper);
    if (operands == OperandType::Int32)
        return { out, 5 };
    if (operands == OperandType::Int16)
        return { out, 3 };

    return { out, 2 };
}

std::pair<std::string, size_t> debug_instruction(bc::Bytecode const& bt, uint32_t function_id, uint32_t addr)
{
    auto inst = (Instruction) bt.get_code_byte(function_id, addr);

    switch (instruction_operand_type(inst)) {
        case OperandType::NoOperand:
            return debug_instruction(inst);
        case OperandType::Int8:
            return debug_instruction(inst, bt.get_code_int8(function_id, addr + 1));
        case OperandType::Int16:
            return debug_instruction(inst, bt.get_code_int16(function_id, addr + 1));
        case OperandType::Int32:
            return debug_instruction(inst, bt.get_code_int32(function_id, addr + 1));
        default:
            break;
    }

    return { "???", 1 };
}

OperandType instruction_operand_type(Instruction inst)
{
    if ((uint8_t) inst >= 0xe0)
        return OperandType::Int32;
    if ((uint8_t) inst >= 0xc0)
        return OperandType::Int16;
    if ((uint8_t) inst >= 0xa0)
        return OperandType::Int8;
    return OperandType::NoOperand;
}

std::optional<Instruction> translate_instruction(std::string const& txt, std::optional<int> op)
{
    auto it = instruction_names.find(txt);
    if (it == instruction_names.end())
        return {};
    Instruction inst = it->second;
    OperandType optype = instruction_operand_type(inst);

    if (optype == OperandType::NoOperand && op)
        return {};
    if (optype != OperandType::NoOperand && !op)
        return {};

    if (optype == OperandType::NoOperand)
        return inst;

    if (op >= std::numeric_limits<int8_t>::min() && op <= std::numeric_limits<int8_t>::max())
        return inst;
    if (op >= std::numeric_limits<int16_t>::min() && op <= std::numeric_limits<int16_t>::max())
        return (Instruction) ((uint8_t) inst + OPCODE_NEXT_SIZE);
    return (Instruction) ((uint8_t) inst + (OPCODE_NEXT_SIZE * 2));
}

}
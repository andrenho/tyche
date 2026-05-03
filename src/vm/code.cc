#include "code.hh"
#include "../common/overloaded.hh"
#include "../instructions/instruction.hh"

namespace tyche::vm {

FunctionId Code::import_bytecode(ByteArray incoming)
{
    bc::Bytecode bc(std::move(incoming));
    // TODO - adjust function calls, constants

    bytecode_ = std::move(bc);

    return 0;  // TODO
}

Operation Code::operation(Location const& location) const
{
    Instruction inst = (Instruction) bytecode_.get_code_byte(location.function_id, location.pc);
    OperandType opet = instruction_operand_type(inst);

    switch (opet) {
        case OperandType::NoOperand:
            return {
                .instruction = inst,
                .operator_ = 0,
                .next_location = { .function_id = location.function_id, .pc = location.pc + 1 },
            };
        case OperandType::Int8:
            return {
                .instruction = inst,
                .operator_ = bytecode_.get_code_int8(location.function_id, location.pc + 1),
                .next_location = { .function_id = location.function_id, .pc = location.pc + 2 },
            };
        case OperandType::Int16:
            return {
                .instruction = inst,
                .operator_ = bytecode_.get_code_int16(location.function_id, location.pc + 1),
                .next_location = { .function_id = location.function_id, .pc = location.pc + 3 },
            };
        case OperandType::Int32:
            return {
                .instruction = inst,
                .operator_ = bytecode_.get_code_int32(location.function_id, location.pc + 1),
                .next_location = { .function_id = location.function_id, .pc = location.pc + 5 },
            };
        default:
            break;
    }

    throw std::logic_error("Should not get here");
}

std::string Code::disassemble() const
{
    std::string out;

    out += ".const\n";
    for (size_t i = 0; i < bytecode_.n_constants(); ++i) {
        out += "\t" + std::to_string(i) + ": ";
        std::visit(overloaded {
            [&out](float f)                { out += std::to_string(f); },
            [&out](std::string const& str) { out += "\"" + str + "\""; },
        }, bytecode_.get_constant(i));
        out += "\n";
    }
    out += "\n";

    for (size_t i = 0; i < bytecode_.n_functions(); ++i) {
        out += ".func " + std::to_string(i) + "\n";
        uint32_t addr = 0;
        while (addr < bytecode_.get_function_sz(i)) {
            auto [op, sz] = debug_instruction(bytecode_, i, addr);
            out += "\t" + op + "\n";
            addr += sz;
        }
    }

    return out;
}

} // tyche

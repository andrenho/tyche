#include "code.hh"
#include "../common/overloaded.hh"
#include "instruction.hh"

namespace tyche {

void Code::import_bytecode(ByteArray incoming)
{
    Bytecode bc(std::move(incoming));
    // TODO - adjust function calls, constants

    bytecode_ = std::move(bc);
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

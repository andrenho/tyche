#ifndef TYCHE_CODE_HH
#define TYCHE_CODE_HH

#include "../instructions/instruction.hh"
#include "location.hh"
#include "value.hh"
#include "../bytecode/bytecode.hh"

namespace tyche::vm {

struct Operation
{
    Instruction instruction;
    int32_t     operator_;
    Location    next_location;
};

class Code {
public:
    FunctionId import_bytecode(StaticByteArray const* incoming);

    [[nodiscard]] std::string disassemble() const;

    [[nodiscard]] Operation operation(Location const& location) const;

    [[nodiscard]] bc::Bytecode const& bytecode() const { return bytecode_; }

private:
    bc::Bytecode bytecode_;
};

} // tyche

#endif //TYCHE_CODE_HH

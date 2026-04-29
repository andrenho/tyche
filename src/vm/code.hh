#ifndef TYCHE_CODE_HH
#define TYCHE_CODE_HH

#include "../bytecode/bytecode.hh"

namespace tyche {

class Code {
public:
    void import_bytecode(ByteArray incoming);

    [[nodiscard]] std::string disassemble() const;

private:
    Bytecode bytecode_;
};

} // tyche

#endif //TYCHE_CODE_HH

#ifndef TYCHE_ASSEMBLER_HH
#define TYCHE_ASSEMBLER_HH

#include <string>

#include "lexer.hh"
#include "../common/bytearray.hh"
#include "../bytecode/bytecodeprototype.hh"

namespace tyche::as {

class Assembler {
public:
    explicit Assembler(std::string source) : lexer_(std::move(source)) {}

    [[nodiscard]] ByteArray assemble();

private:
    Lexer lexer_;

    TokenValue expect_token(TokenType type);
    void       emit_instruction(uint32_t function_id, std::string const& inst, int oper);
};

} // tyche

#endif //TYCHE_ASSEMBLER_HH

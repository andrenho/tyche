#include "assembler.hh"

#include "as_exceptions.hh"
#include "../bytecode/bytecode.hh"

namespace tyche::as {

ByteArray Assembler::assemble()
{
    lexer_.reset();
    bp_ = {};

    enum class Section { Const, Function } section;
    uint32_t function_id = 0;

    for (;;) {
        Token t = lexer_.ingest();

        if (t.type == TokenType::Directive) {
        } else if (section == Section::Const && t.type == TokenType::Integer) {
        } else if (section == Section::Function && t.type == TokenType::Instruction) {
        } else if (t.type == TokenType::EOF_) {
            break;
        } else if (t.type != TokenType::Enter) {
            throw AssemblyError("Unexpected token " + t.token, t.line, t.column);
        }
    }

    return bc::Bytecode::generate(bp_);
}

} // tyche

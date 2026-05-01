#include "assembler.hh"

#include "as_exceptions.hh"
#include "../bytecode/bytecode.hh"

using namespace std::string_literals;

namespace tyche::as {

ByteArray Assembler::assemble()
{
    bc::BytecodePrototype bp;

    lexer_.reset();

    enum class Section { Const, Function } section;
    uint32_t function_id = 0;

    for (;;) {
        Token t = lexer_.ingest();
        if (t.type == TokenType::Enter)
            continue;

        if (t.type == TokenType::Directive) {
            if (std::get<std::string>(t.token) == ".const") {
                section = Section::Const;
                expect_token(TokenType::Enter);
            } else if (std::get<std::string>(t.token) == ".func") {
                section = Section::Function;
                function_id = std::get<int>(expect_token(TokenType::Integer));
                expect_token(TokenType::Enter);
            } else {
                throw AssemblyError("Invalid directive " + std::get<std::string>(t.token), t.line, t.column);
            }

        } else if (section == Section::Const && t.type == TokenType::Integer) {
            int index = std::get<int>(t.token);
            if ((size_t) index >= bp.constants.size())
                bp.constants.resize(index + 1);
            expect_token(TokenType::Colon);
            Token tt = lexer_.ingest();
            if (tt.type == TokenType::Float)
                bp.constants[index] = std::get<float>(tt.token);
            else if (tt.type == TokenType::String)
                bp.constants[index] = std::get<std::string>(tt.token);
            else
                throw AssemblyError("Expected float or string as constant", tt.line, tt.column);
            expect_token(TokenType::Enter);

        } else if (section == Section::Function && t.type == TokenType::Instruction) {
            std::string instruction = std::get<std::string>(t.token);
            int oper = 0;
            Token tt = lexer_.ingest();
            if (tt.type == TokenType::Integer) {
                oper = std::get<int>(tt.token);
                tt = lexer_.ingest();
            }
            emit_instruction(function_id, instruction, oper);
            if (tt.type != TokenType::Enter)
                throw AssemblyError("Expected enter", tt.line, tt.column);

        } else if (t.type == TokenType::EOF_) {
            break;

        } else if (t.type != TokenType::Enter) {
            throw AssemblyError("Unexpected token of type " + token_type_name(t.type) + ")", t.line, t.column);
        }
    }

    return bc::Bytecode::generate(bp);
}

void Assembler::emit_instruction(uint32_t function_id, std::string const& inst, int oper)
{

}

TokenValue Assembler::expect_token(TokenType type)
{
    Token t = lexer_.ingest();
    if (t.type != type)
        throw AssemblyError("Expected " + token_type_name(t.type), t.line, t.column);
    return t.token;
}

} // tyche

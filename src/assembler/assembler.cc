#include "assembler.hh"

#include <unordered_map>

#include "as_exceptions.hh"
#include "../bytecode/bytecode.hh"
#include "../instructions/instruction.hh"

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
                if (function_id >= bp.functions.size())
                    bp.functions.resize(function_id + 1, { 0, 0 });
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
            std::optional<int> oper = {};
            Token tt = lexer_.ingest();
            if (tt.type == TokenType::Integer) {
                oper = std::get<int>(tt.token);
                tt = lexer_.ingest();
            }

            auto oinst = translate_instruction(instruction, oper);
            if (!oinst)
                throw AssemblyError("Invalid or misused instruction '" + instruction + "'", tt.line, tt.column);

            bp.functions.at(function_id).code.append_byte((uint8_t) *oinst);
            switch (instruction_operand_type(*oinst)) {
                case OperandType::Int8:  bp.functions.at(function_id).code.append_int8((int8_t) *oper); break;
                case OperandType::Int16: bp.functions.at(function_id).code.append_int16((int16_t) *oper); break;
                case OperandType::Int32: bp.functions.at(function_id).code.append_int32(*oper); break;
                case OperandType::NoOperand: default: break;
            }

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

TokenValue Assembler::expect_token(TokenType type)
{
    Token t = lexer_.ingest();
    if (t.type != type)
        throw AssemblyError("Expected " + token_type_name(t.type), t.line, t.column);
    return t.token;
}

} // tyche

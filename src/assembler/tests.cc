#include "assembler.hh"
#include "gtest/gtest.h"

#include "../bytecode/bytecodeprototype.hh"
#include "../bytecode/bytecode.hh"
#include "../instructions/instruction.hh"

using namespace tyche;
using namespace tyche::as;
using namespace tyche::bc;

TEST(Lexer, Lexer)
{
    Token t;
    Lexer lexer(".dir push 382 -12 3.14 -12.8 \"Hello\" \"Hel\\\"lo\"\n");

    ASSERT_EQ(lexer.ingest(), (Token { TokenType::Directive, ".dir" }));
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::Instruction, "push" }));
    t = lexer.ingest(); ASSERT_EQ(t.type, TokenType::Integer); ASSERT_EQ(std::get<int>(t.token), 382);
    t = lexer.ingest(); ASSERT_EQ(t.type, TokenType::Integer); ASSERT_EQ(std::get<int>(t.token), -12);
    t = lexer.ingest(); ASSERT_EQ(t.type, TokenType::Float); ASSERT_FLOAT_EQ(std::get<float>(t.token), 3.14f);
    t = lexer.ingest(); ASSERT_EQ(t.type, TokenType::Float); ASSERT_FLOAT_EQ(std::get<float>(t.token), -12.8f);
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::String, "Hello" }));
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::String, "Hel\"lo" }));
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::Enter, "\n" }));
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::EOF_ }));
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::EOF_ }));
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::EOF_ }));

    lexer.reset();
    ASSERT_EQ(lexer.ingest(), (Token { TokenType::Directive, ".dir" }));
}

TEST(Assember, Assembler)
{
    BytecodePrototype bp;
    bp.constants.emplace_back(3.14f);
    bp.constants.emplace_back("Hello world");
    bp.functions.emplace_back(0, 0);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::PushInt8);
    bp.functions.at(0).code.append_int8(2);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::PushInt8);
    bp.functions.at(0).code.append_int8(3);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::Sum);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::Return);
    bp.functions.emplace_back(0, 0);
    bp.functions.at(1).code.append_byte((uint8_t) Instruction::PushInt16);
    bp.functions.at(1).code.append_int16(5000);
    bp.functions.at(1).code.append_byte((uint8_t) Instruction::Return);
    StaticByteArray expected = Bytecode::generate(bp);

    std::string src = R"(
.const
    0: 3.14
    1: "Hello world"

.func 0
    pushi   2   ; this is a comment
    pushi   3
    sum
    ret
.func 1
    pushi   5000
    ret
)";

    StaticByteArray actual = Assembler(src).assemble();
    ASSERT_EQ(expected, actual);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

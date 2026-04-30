#include "assembler.hh"
#include "gtest/gtest.h"

#include "../bytecode/bytecodeprototype.hh"
#include "../bytecode/bytecode.hh"
#include "../vm/instruction.hh"

using namespace tyche;
using namespace tyche::as;
using namespace tyche::bc;
using namespace tyche::vm;

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
    ByteArray expected = Bytecode::generate(bp);

    std::string src = R"(
.const
    0: 3.14
    1: "Hello world"

.function 0
    pushi   2
    pushi   3
    sum
    ret
.function 1
    pushi   5000
    ret
)";

    ByteArray actual = Assembler(src).assemble();
    ASSERT_EQ(expected, actual);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

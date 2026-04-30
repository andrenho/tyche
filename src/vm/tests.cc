#include "gtest/gtest.h"

#include "../bytecode/bytecodeprototype.hh"
#include "../common/bytearray.hh"
#include "../bytecode/bytecode.hh"
#include "code.hh"
#include "stack.hh"
#include "vm.hh"

using namespace tyche;
using namespace tyche::bc;
using namespace tyche::vm;

TEST(Code, ImportSingleAndDebug)
{
    BytecodePrototype bp;

    bp.constants.emplace_back(3.14f);
    bp.constants.emplace_back("HELLO");

    bp.functions.emplace_back(0, 0);
    bp.functions.at(0).code.append_byte(0xa0);       // pushi
    bp.functions.at(0).code.append_int8(42);

    bp.functions.emplace_back(2, 1);
    bp.functions.at(1).code.append_byte(0x1a);      // appnd

    ByteArray ba = Bytecode::generate(bp);

    Code code;
    code.import_bytecode(std::move(ba));
    printf("%s\n", code.disassemble().c_str());
}

TEST(Stack, PushPullGet)
{
    Stack stack;
    stack.push(Value::CreateInteger(10));
    stack.push(Value::CreateInteger(20));
    stack.push(Value::CreateInteger(30));

    ASSERT_EQ(stack.size(), 3);
    ASSERT_EQ(stack.at(0).as_integer(), 10);
    ASSERT_EQ(stack.at(1).as_integer(), 20);
    ASSERT_EQ(stack.at(-1).as_integer(), 30);
    ASSERT_EQ(stack.at(-2).as_integer(), 20);
}

TEST(Stack, FramePointer)
{
    Stack stack;
    stack.push(Value::CreateInteger(10));
    stack.push(Value::CreateInteger(20));
    stack.push_fp();
    stack.push(Value::CreateInteger(30));
    stack.push(Value::CreateInteger(40));
    stack.push(Value::CreateInteger(50));

    ASSERT_EQ(stack.size(), 3);
    ASSERT_EQ(stack.at(0).as_integer(), 30);
    ASSERT_EQ(stack.at(1).as_integer(), 40);
    ASSERT_EQ(stack.at(-1).as_integer(), 50);
    ASSERT_EQ(stack.at(-2).as_integer(), 40);

    stack.pop_fp();

    ASSERT_EQ(stack.size(), 2);
    ASSERT_EQ(stack.at(0).as_integer(), 10);
    ASSERT_EQ(stack.at(1).as_integer(), 20);
    ASSERT_EQ(stack.at(-1).as_integer(), 20);
    ASSERT_EQ(stack.at(-2).as_integer(), 10);
}

TEST(VM, BasicCode)
{
    // code (2+3)
    BytecodePrototype bp;
    bp.functions.emplace_back(0, 0);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::PushInt8);
    bp.functions.at(0).code.append_int8(2);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::PushInt8);
    bp.functions.at(0).code.append_int8(3);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::Sum);
    bp.functions.at(0).code.append_byte((uint8_t) Instruction::Return);
    ByteArray ba = Bytecode::generate(bp);

    VM vm;
    vm.load_bytecode(std::move(ba));
    vm.call(0);

    int32_t result = vm.to_integer(-1);
    ASSERT_EQ(result, 5);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

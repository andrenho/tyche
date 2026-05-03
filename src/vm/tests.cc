#include "gtest/gtest.h"

#include "../bytecode/bytecodeprototype.hh"
#include "../bytecode/bytecode.hh"
#include "../assembler/assembler.hh"
#include "code.hh"
#include "stack.hh"
#include "vm.hh"

using namespace tyche;
using namespace tyche::bc;
using namespace tyche::vm;

static VM run(std::string oper) {
    return VM().load_bytecode(as::Assembler(std::format(R"(
            .const
                0: 3.14
                1: "Hello world"
            .func 0
                {}
                ret
        )", oper)).assemble()).call(0);
}

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
    stack.push(Value::createInteger(10));
    stack.push(Value::createInteger(20));
    stack.push(Value::createInteger(30));

    ASSERT_EQ(stack.size(), 3);
    ASSERT_EQ(stack.at(0).as_integer(), 10);
    ASSERT_EQ(stack.at(1).as_integer(), 20);
    ASSERT_EQ(stack.at(-1).as_integer(), 30);
    ASSERT_EQ(stack.at(-2).as_integer(), 20);
}

TEST(Stack, FramePointer)
{
    Stack stack;
    stack.push(Value::createInteger(10));
    stack.push(Value::createInteger(20));
    stack.push_fp();
    stack.push(Value::createInteger(30));
    stack.push(Value::createInteger(40));
    stack.push(Value::createInteger(50));

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

TEST(VM, StackOperations)
{
    ASSERT_EQ(run("pushi 5000").to_integer(-1), 5000);
    ASSERT_EQ(run("pushi -5000").to_integer(-1), -5000);
    ASSERT_FLOAT_EQ(run("pushi 5000").to_float(-1), 5000.f);
    ASSERT_FLOAT_EQ(run("pushc 0").to_float(-1), 3.14f);
    ASSERT_EQ(run("pushc 0").to_integer(-1), 3);
    EXPECT_STREQ(run("pushc 1").to_string_ptr(-1), "Hello world");
    ASSERT_TRUE(run("pushf 0").is_function(-1));
    ASSERT_EQ(run("pushi 2\n pushi 3\n pop").to_integer(-1), 2);
}

TEST(VM, IntegerIntegerOperations)
{
    auto test_op = [](int32_t op1, int32_t op2, std::string oper) {
        return VM().load_bytecode(as::Assembler(std::format(R"(
            .func 0
                pushi {}
                pushi {}
                {}
                ret
        )", op1, op2, oper)).assemble()).call(0).to_integer(-1);
    };

    ASSERT_EQ(test_op(2, 3, "sum"), 5);
    ASSERT_EQ(test_op(2, 3, "sub"), -1);
    ASSERT_EQ(test_op(2, 3, "mul"), 6);
    ASSERT_EQ(test_op(20, 3, "idiv"), 6);
    ASSERT_EQ(test_op(2, 3, "eq"), 0);
    ASSERT_EQ(test_op(2, 3, "neq"), 1);
    ASSERT_EQ(test_op(2, 3, "lt"), 1);
    ASSERT_EQ(test_op(2, 3, "lte"), 1);
    ASSERT_EQ(test_op(3, 3, "lte"), 1);
    ASSERT_EQ(test_op(4, 3, "lte"), 0);
    ASSERT_EQ(test_op(2, 3, "gt"), 0);
    ASSERT_EQ(test_op(2, 3, "gte"), 0);
    ASSERT_EQ(test_op(3, 3, "gte"), 1);
    ASSERT_EQ(test_op(4, 3, "gte"), 1);
    ASSERT_EQ(test_op(2, 3, "and"), 2);
    ASSERT_EQ(test_op(2, 3, "or"), 3);
    ASSERT_EQ(test_op(2, 3, "xor"), 1);
    ASSERT_EQ(test_op(2, 3, "pow"), 8);
    ASSERT_EQ(test_op(2, 3, "shl"), 16);
    ASSERT_EQ(test_op(30, 2, "shr"), 7);
    ASSERT_EQ(test_op(8, 3, "mod"), 2);

    ASSERT_FLOAT_EQ(run("pushi 3\n pushi 2\n div").to_float(-1), 1.5f);
}

TEST(VM, IntegerFloatOperations)
{
    auto test_op = [](int op1, std::string const& op2, std::string oper) -> VM {
        return VM().load_bytecode(as::Assembler(std::format(R"(
            .const
                0: {}
            .func 0
                pushi {}
                pushc 0
                {}
                ret
        )", op2, op1, oper)).assemble()).call(0);
    };

    ASSERT_FLOAT_EQ(test_op(2, "3.5", "sum").to_float(-1), 5.5f);
    ASSERT_FLOAT_EQ(test_op(2, "3.5", "sub").to_float(-1), -1.5f);
    ASSERT_FLOAT_EQ(test_op(2, "3.5", "mul").to_float(-1), 7.f);
    ASSERT_FLOAT_EQ(test_op(20, "3.5", "idiv").to_integer(-1), 6);
    ASSERT_FLOAT_EQ(test_op(20, "3.5", "div").to_float(-1), 5.7142859);
    ASSERT_FLOAT_EQ(test_op(3, "3.5", "eq").to_integer(-1), 0);
    ASSERT_FLOAT_EQ(test_op(3, "3.0", "eq").to_integer(-1), 1);
}

TEST(VM, FloatIntegerOperations)
{
    auto test_op = [](std::string const& op1, int op2, std::string oper) -> VM {
        return VM().load_bytecode(as::Assembler(std::format(R"(
            .const
                0: {}
            .func 0
                pushc 0
                pushi {}
                {}
                ret
        )", op1, op2, oper)).assemble()).call(0);
    };

    ASSERT_FLOAT_EQ(test_op("3.5", 2, "sum").to_float(-1), 5.5f);
    ASSERT_FLOAT_EQ(test_op("3.5", 2, "sub").to_float(-1), 1.5f);
    ASSERT_FLOAT_EQ(test_op("3.5", 2, "mul").to_float(-1), 7.f);
    ASSERT_FLOAT_EQ(test_op("3.5", 2, "idiv").to_integer(-1), 1);
    ASSERT_FLOAT_EQ(test_op("3.5", 2, "div").to_float(-1), 1.75f);
    ASSERT_FLOAT_EQ(test_op("3.5", 3, "eq").to_integer(-1), 0);
    ASSERT_FLOAT_EQ(test_op("3.0", 3, "eq").to_integer(-1), 1);
}

TEST(VM, FloatFloatOperations)
{
    auto test_op = [](std::string const& op1, std::string const& op2, std::string oper) -> VM {
        return VM().load_bytecode(as::Assembler(std::format(R"(
            .const
                0: {}
                1: {}
            .func 0
                pushc 0
                pushc 1
                {}
                ret
        )", op1, op2, oper)).assemble()).call(0);
    };

    ASSERT_FLOAT_EQ(test_op("3.5", "2.2", "sum").to_float(-1), 5.7f);
    ASSERT_FLOAT_EQ(test_op("3.5", "2.2", "sub").to_float(-1), 1.3f);
    ASSERT_FLOAT_EQ(test_op("3.5", "2.2", "mul").to_float(-1), 7.7f);
    ASSERT_FLOAT_EQ(test_op("3.5", "2.2", "idiv").to_integer(-1), 1);
    ASSERT_FLOAT_EQ(test_op("4.5", "2.5", "div").to_float(-1), 1.8f);
    ASSERT_FLOAT_EQ(test_op("3.2005", "3.2", "eq").to_integer(-1), 0);
    ASSERT_FLOAT_EQ(test_op("3.2", "3.2", "eq").to_integer(-1), 1);
}

TEST(VM, StringString)
{
    ASSERT_EQ(run(R"(
        .const
            0: "Hello"
            1: "World"
        .func 0
            pushc 0
            pushc 1
            sum
            ret
    )").to_string_ptr(-1), "HelloWorld");

    ASSERT_EQ(run(R"(
        .const
            0: "Hello"
            1: "World"
        .func 0
            pushc 0
            pushc 1
            eq
            ret
    )").to_integer(-1), 0);

    ASSERT_EQ(run(R"(
        .const
            0: "Hello"
            1: "Hello"
        .func 0
            pushc 0
            pushc 1
            eq
            ret
    )").to_integer(-1), 1);

    ASSERT_EQ(run(R"(
        .const
            0: "Hello"
        .func 0
            pushc 0
            pushi 1
            eq
            ret
    )").to_integer(-1), 0);
}

TEST(VM, LocalVariables)
{
    VM vm = run(R"(
        .func 0
            pushv 2         ; local a, b
            pushi 3         ; a = 3
            set   0
            pushi 4         ; b = 4
            set   1
            dupv  0         ; return a
            ret
    )");

    ASSERT_EQ(vm.stack_sz(), 1);
    ASSERT_EQ(vm.to_integer(-1), 3);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

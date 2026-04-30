#include "gtest/gtest.h"

#include <cstring>
#include <functional>

#include "../common/bytearray.hh"
#include "bytecodeprototype.hh"
#include "bytecode.hh"

using namespace tyche;
using namespace tyche::bc;

TEST(ByteArray, ByteArray)
{
    auto test = [](std::function<void(ByteArray&)> const& f, std::vector<uint8_t> const& expected) {
        ByteArray ba;
        f(ba);
        ASSERT_EQ(ba.data().size(), expected.size());
        ASSERT_EQ(std::memcmp(ba.data().data(), expected.data(), ba.data().size()), 0);
    };

#define TESTX(a, ...) test([](ByteArray& ba) { a; }, std::vector<uint8_t>({ __VA_ARGS__ }));

    TESTX(ba.set_byte(1, 0xab), 0x00, 0xab)

    ByteArray ba;
    ba.set_byte(1, 0xab); ASSERT_EQ(ba.get_byte(1), 0xab);

    ba.set_int8(1, 12); ASSERT_EQ(ba.get_int8(1), 12);
    ba.set_int8(1, -12); ASSERT_EQ(ba.get_int8(1), -12);
    ba.set_int16(1, 5000); ASSERT_EQ(ba.get_int16(1), 5000);
    ba.set_int32(1, 5000300); ASSERT_EQ(ba.get_int32(1), 5000300);
    ba.set_int32(1, -5000300); ASSERT_EQ(ba.get_int32(1), -5000300);

    ba.set_float(1, 3.14); ASSERT_FLOAT_EQ(ba.get_float(1), 3.14);
    ba.set_float(1, -3.14); ASSERT_FLOAT_EQ(ba.get_float(1), -3.14);
    ba.set_float(1, -5000300.1324); ASSERT_FLOAT_EQ(ba.get_float(1), -5000300.1324);

    ba.set_string(1, "Hello world!"); ASSERT_EQ(ba.get_string(1), std::make_pair("Hello world!", 13));

#undef TESTX
}

TEST(Bytecode, Constants)
{
    BytecodePrototype bp;
    bp.constants.emplace_back(42.3f);
    bp.constants.emplace_back("HELLO");

    std::vector<uint8_t> expected = {
            // header
            0x38, 0xc1, 0xb3, 0x74,                     // magic
            0x01, 0x00, 0x00, 0x00,                     // version
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,

            // index
            0x50, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, // constant index
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // function undex
            0x58, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, // raw constants
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // raw code
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

            // constant indexes
            0x00, 0x00, 0x00, 0x00,
            0x05, 0x00, 0x00, 0x00,

            // constant values
            CONST_TYPE_FLOAT, 0x33, 0x33, 0x29, 0x42,         // float: 42.3f
            CONST_TYPE_STRING, 'H', 'E', 'L', 'L', 'O', 0x00
    };

    ByteArray ba = Bytecode::generate(bp);
    ASSERT_EQ(ba.data(), expected);
}

TEST(Bytecode, Code)
{
    BytecodePrototype bp;
    auto& f = bp.functions.emplace_back(0, 0);
    f.code.append_byte(0x68);
    f.code.append_int8(42);

    auto& f2 = bp.functions.emplace_back(2, 1);
    f2.code.append_byte(0x42);

    std::vector<uint8_t> expected = {
            // header
            0x38, 0xc1, 0xb3, 0x74,                     // magic
            0x01, 0x00, 0x00, 0x00,                     // version
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,

            // index
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // constant index
            0x50, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, // variable index
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // raw constants
            0x68, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, // raw code
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

            // function definitions
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
            0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,

            // code
            0x68, 42, 0x42,
    };

    ByteArray ba = Bytecode::generate(bp);
    ASSERT_EQ(ba.data(), expected);
}

TEST(Bytecode, Parsing)
{
    // write bytecode

    BytecodePrototype bp;

    bp.constants.emplace_back(3.14f);
    bp.constants.emplace_back("HELLO");

    auto& f = bp.functions.emplace_back(0, 0);
    f.code.append_byte(0x68);
    f.code.append_int8(42);

    auto& ff = bp.functions.emplace_back(2, 1);
    ff.code.append_byte(0x42);

    ByteArray ba = Bytecode::generate(bp);
    // print(ba.data());

    // read bytecode

    Bytecode bc(std::move(ba));

    ASSERT_EQ(bc.n_constants(), 2);
    ASSERT_EQ(bc.n_functions(), 2);
    ASSERT_EQ(bc.get_function_sz(0), 2);
    ASSERT_EQ(bc.get_function_sz(1), 1);

    ASSERT_FLOAT_EQ(std::get<float>(bc.get_constant(0)), 3.14f);
    ASSERT_EQ(std::get<std::string>(bc.get_constant(1)), "HELLO");

    Bytecode::FunctionDef f1 = bc.get_function_def(0);
    ASSERT_EQ(f1.n_params, 0);
    ASSERT_EQ(f1.locals, 0);

    Bytecode::FunctionDef f2 = bc.get_function_def(1);
    ASSERT_EQ(f2.n_params, 2);
    ASSERT_EQ(f2.locals, 1);

    ASSERT_EQ(bc.get_code_byte(0, 0), 0x68);
    ASSERT_EQ(bc.get_code_int8(0, 1), 42);
    ASSERT_EQ(bc.get_code_byte(1, 0), 0x42);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
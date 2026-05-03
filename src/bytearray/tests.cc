#include "gtest/gtest.h"

#include <cstring>
#include <functional>

#include "bytearray.hh"
#include "bytearraybuilder.hh"

using namespace tyche;

TEST(StaticByteArray, StaticByteArray)
{
    auto test = [](std::function<void(ByteArrayBuilder&)> const& f, std::vector<uint8_t> const& expected) {
        ByteArrayBuilder ba;
        f(ba);
        ASSERT_EQ(ba.data().size(), expected.size());
        ASSERT_EQ(std::memcmp(ba.data().data(), expected.data(), ba.data().size()), 0);
    };

#define TESTX(a, ...) test([](ByteArrayBuilder& ba) { a; }, std::vector<uint8_t>({ __VA_ARGS__ }));

    TESTX(ba.set_byte(1, 0xab), 0x00, 0xab)

    ByteArrayBuilder ba;
    { auto b = ba.set_byte(1, 0xab).build(); ASSERT_EQ(b.get_byte(1), 0xab); }

    { auto b = ba.set_int8(1, 12).build(); ASSERT_EQ(b.get_int8(1), 12); }
    { auto b = ba.set_int8(1, -12).build(); ASSERT_EQ(b.get_int8(1), -12); }
    { auto b = ba.set_int16(1, 5000).build(); ASSERT_EQ(b.get_int16(1), 5000); }
    { auto b = ba.set_int32(1, 5000300).build(); ASSERT_EQ(b.get_int32(1), 5000300); }
    { auto b = ba.set_int32(1, -5000300).build(); ASSERT_EQ(b.get_int32(1), -5000300); }

    { auto b = ba.set_float(1, 3.14).build(); ASSERT_FLOAT_EQ(b.get_float(1), 3.14); }
    { auto b = ba.set_float(1, -3.14).build(); ASSERT_FLOAT_EQ(b.get_float(1), -3.14); }
    { auto b = ba.set_float(1, -5000300.1324).build(); ASSERT_FLOAT_EQ(b.get_float(1), -5000300.1324); }

    {
        auto b = ba.set_string(1, "Hello world!").build();
        auto str = b.get_string_ptr(1);
        EXPECT_STREQ(str.first, "Hello world!");
        ASSERT_EQ(str.second, 13);
    }

#undef TESTX
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
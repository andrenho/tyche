#include "gtest/gtest.h"

#include <cstring>
#include <functional>

#include "bytearray.hh"

using namespace tyche;

TEST(ByteArray, ByteArray)
{
    auto test = [](std::function<void(ByteArray&)> const& f, std::vector<uint8_t> const& expected) {
        ByteArray ba;
        f(ba);
        ASSERT_EQ(ba.data().size(), expected.size());
        ASSERT_EQ(std::memcmp(ba.data().data(), expected.data(), ba.data().size()), 0);
    };

#define TESTX(a, ...) test([](ByteArray& ba) { a; }, std::vector<uint8_t>({ __VA_ARGS__ }));

    TESTX(ba.add_byte(1, 0xab), 0x00, 0xab)

    ByteArray ba;
    ba.add_byte(1, 0xab); ASSERT_EQ(ba.get_byte(1), 0xab);

    ba.add_int(1, 12); ASSERT_EQ(ba.get_int(1), std::make_pair(12, 1));
    ba.add_int(1, -12); ASSERT_EQ(ba.get_int(1), std::make_pair(-12, 1));
    ba.add_int(1, 5000); ASSERT_EQ(ba.get_int(1), std::make_pair(5000, 2));
    ba.add_int(1, 5000300); ASSERT_EQ(ba.get_int(1), std::make_pair(5000300, 4));
    ba.add_int(1, -5000300); ASSERT_EQ(ba.get_int(1), std::make_pair(-5000300, 4));

    ba.add_float(1, 3.14); ASSERT_FLOAT_EQ(ba.get_float(1).first, 3.14);
    ba.add_float(1, -3.14); ASSERT_FLOAT_EQ(ba.get_float(1).first, -3.14);
    ba.add_float(1, -5000300.1324); ASSERT_FLOAT_EQ(ba.get_float(1).first, -5000300.1324);

    ba.add_string(1, "Hello world!"); ASSERT_EQ(ba.get_string(1), std::make_pair("Hello world!", 13));

#undef TESTX
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
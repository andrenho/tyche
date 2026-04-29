#include "gtest/gtest.h"

#include "../bytecode/bytecodeprototype.hh"
#include "../bytecode/bytearray.hh"
#include "../bytecode/bytecode.hh"
#include "code.hh"

using namespace tyche;

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

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

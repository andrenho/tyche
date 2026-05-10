#ifndef TYCHE_VM_EXCEPTIONS_HH
#define TYCHE_VM_EXCEPTIONS_HH

#include <stdexcept>
#include <string>

#include "expr.hh"

namespace tyche::vm {

class VMRuntimeError : public std::runtime_error
{
public:
    explicit VMRuntimeError(std::string const& str) : std::runtime_error(str.c_str()) {}
};

class VMStackUnderflow : public VMRuntimeError
{
public:
    explicit VMStackUnderflow() : VMRuntimeError("Stack underflow") {}
};

class VMStackOutOfRange : public VMRuntimeError
{
public:
    explicit VMStackOutOfRange() : VMRuntimeError("Item does not exist in stack") {}
};

class VMTypeError : public VMRuntimeError
{
public:
    explicit VMTypeError(Type expected, Type found) : VMRuntimeError("Type error (expected " + type_name(expected) + ", found " + type_name(found) + ")") {}
};

class VMInvalidOpcode : public VMRuntimeError
{
public:
    explicit VMInvalidOpcode(uint8_t opcode) : VMRuntimeError("Invalid opcode " + std::to_string(opcode)) {}
};

class VMInvalidOperation : public VMRuntimeError
{
public:
    explicit VMInvalidOperation(BinaryOperationType op, Type type1, Type type2) : VMRuntimeError("Invalid binary operation for types " + type_name(type1) + " and " + type_name(type2)) {}
};

}

#endif //TYCHE_VM_EXCEPTIONS_HH

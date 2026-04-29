#ifndef TYCHE_VM_EXCEPTIONS_HH
#define TYCHE_VM_EXCEPTIONS_HH

#include <stdexcept>
#include <string>

namespace tyche {

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

}

#endif //TYCHE_VM_EXCEPTIONS_HH

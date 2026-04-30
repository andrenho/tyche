#ifndef TYCHE_VM_EXCEPTIONS_HH
#define TYCHE_VM_EXCEPTIONS_HH

#include <stdexcept>
#include <string>

namespace tyche::as {

class AssemblyError : public std::runtime_error
{
public:
    explicit AssemblyError(std::string const& str) : std::runtime_error(str.c_str()) {}
};

}

#endif //TYCHE_VM_EXCEPTIONS_HH

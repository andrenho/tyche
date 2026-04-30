#ifndef TYCHE_VM_EXCEPTIONS_HH
#define TYCHE_VM_EXCEPTIONS_HH

#include <stdexcept>
#include <string>

namespace tyche::as {

class AssemblyError : public std::runtime_error
{
public:
    explicit AssemblyError(std::string const& str, size_t line, size_t column)
        : std::runtime_error((str + " at: line " + std::to_string(line) + ", column: " + std::to_string(column)).c_str()) {}
};

}

#endif //TYCHE_VM_EXCEPTIONS_HH

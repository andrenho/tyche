#ifndef TYCHE_BC_EXCEPTIONS_HH
#define TYCHE_BC_EXCEPTIONS_HH

#include <stdexcept>

namespace tyche::bc {

class BytecodeParsingError : public std::runtime_error {
public:
    explicit BytecodeParsingError(std::string const& str) : std::runtime_error(str.c_str()) {}
};

}

#endif //TYCHE_BC_EXCEPTIONS_HH

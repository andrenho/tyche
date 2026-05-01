#ifndef TYCHE_BYTECODEPROTOTYPE_HH
#define TYCHE_BYTECODEPROTOTYPE_HH

#include <cstdint>
#include <string>
#include <variant>
#include <vector>
#include "constant.hh"
#include "../common/bytearray.hh"

namespace tyche::bc {

struct BytecodePrototype {
    struct Function {
        uint16_t  n_pars;
        uint16_t  n_locals;
        ByteArray code {};

        Function(uint16_t n_pars_, uint16_t n_locals_) : n_pars(n_pars_), n_locals(n_locals_), code(ByteArray {}) {}
    };

    std::vector<ConstantValue> constants {};
    std::vector<Function> functions {};

    // TODO - debugging info
};

}

#endif //TYCHE_BYTECODEPROTOTYPE_HH

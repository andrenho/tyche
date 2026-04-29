#ifndef TYCHE_BYTECODEPROTOTYPE_HH
#define TYCHE_BYTECODEPROTOTYPE_HH

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace tyche {

struct BytecodePrototype {
    using ConstantValue = std::variant<int32_t, float, std::string>;

    struct Function {
        uint16_t  n_pars;
        uint16_t  n_locals;
        ByteArray code;

        Function(uint16_t n_pars_, uint16_t n_locals_) : n_pars(n_pars_), n_locals(n_locals_), code(ByteArray {}) {}
    };

    std::vector<ConstantValue> constants {};
    std::vector<Function> functions {};

    // TODO - debugging info
};

}

#endif //TYCHE_BYTECODEPROTOTYPE_HH

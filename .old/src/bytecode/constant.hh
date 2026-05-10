#ifndef TYCHE_CONSTANT_HH
#define TYCHE_CONSTANT_HH

#include <string>
#include <variant>

namespace tyche::bc {

using ConstantValue = std::variant<float, std::string>;

enum ConstantType : uint8_t { CONST_TYPE_FLOAT = 1, CONST_TYPE_STRING = 2 };

}

#endif //TYCHE_CONSTANT_HH

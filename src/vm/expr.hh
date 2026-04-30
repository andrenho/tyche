#ifndef TYCHE_EXPR_HH
#define TYCHE_EXPR_HH
#include "value.hh"

namespace tyche::vm {

enum class BinaryOperationType { Sum };

Value binary_operation(Value const& a, Value const& b, BinaryOperationType op);

}
#endif //TYCHE_EXPR_HH

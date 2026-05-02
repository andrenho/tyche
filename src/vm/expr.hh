#ifndef TYCHE_EXPR_HH
#define TYCHE_EXPR_HH
#include "value.hh"

namespace tyche::vm {

enum class BinaryOperationType
{
    Sum, Subtraction, Multiplication, Division, IntegerDivision,
    Equality, Inequality, LessThan, LessThanOrEquals,
    GreaterThan, GreaterThanOrEquals, Power, Modulo,
    BitwiseAnd, BitwiseOr, BitwiseXor, ShiftLeft, ShiftRight,
    COUNT
};

constexpr float FLOAT_EPSILON = 0.000001f;

Value binary_operation(Value const& a, Value const& b, BinaryOperationType op);

}
#endif //TYCHE_EXPR_HH

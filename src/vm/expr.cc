#include "expr.hh"

#include "vm_exceptions.hh"

namespace tyche::vm {

Value binary_operation(Value const& a, Value const& b, BinaryOperationType op)
{
    // TODO - this is temporary code

    if (a.type() == Type::Integer && b.type() == Type::Integer && op == BinaryOperationType::Sum) {
        return Value::CreateInteger(a.as_integer() + b.as_integer());
    } else {
        throw VMInvalidOperation(op, a.type(), b.type());
    }
}

}
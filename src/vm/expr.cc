#include "expr.hh"

#include <cmath>
#include <functional>

#include "vm_exceptions.hh"

namespace tyche::vm {

std::function<Value(Value const&, Value const&)> binary_ops[(size_t) BinaryOperationType::COUNT][(size_t) Type::COUNT][(size_t) Type::COUNT];

static int init_ = []() {
    // every combination, except when explicit, return type error
    for (size_t i = 0; i < (size_t) BinaryOperationType::COUNT; ++i) {
        for (size_t j = 0; j < (size_t) Type::COUNT; ++j) {
            for (size_t k = 0; k < (size_t) Type::COUNT; ++k) {
                binary_ops[i][j][k] = [&i](Value const& a, Value const& b) -> Value {
                    throw VMInvalidOperation((BinaryOperationType) i, a.type(), b.type());
                };
            }
        }
    }

    // every equality/inequality, by default, return inequal
    for (size_t j = 0; j < (size_t) Type::COUNT; ++j) {
        for (size_t k = 0; k < (size_t) Type::COUNT; ++k) {
            binary_ops[(size_t) BinaryOperationType::Equality][j][k] = [](Value const&, Value const&) { return Value::createFalse(); };
            binary_ops[(size_t) BinaryOperationType::Inequality][j][k] = [](Value const&, Value const&) { return Value::createTrue(); };
        }
    }

#define BIN_OP(op, t1, t2) binary_ops[(size_t) BinaryOperationType::op][(size_t) Type::t1][(size_t) Type::t2] = [](Value const& b, Value const& a)

    BIN_OP(Sum, Integer, Integer) { return Value::createInteger(a.as_integer() + b.as_integer()); };
    BIN_OP(Sum, Integer, Float)   { return Value::createFloat((float) a.as_integer() + b.as_float()); };
    BIN_OP(Sum, Float, Integer)   { return Value::createFloat(a.as_float() + (float) b.as_integer()); };
    BIN_OP(Sum, Float, Float)     { return Value::createFloat(a.as_float() + b.as_float()); };
    BIN_OP(Sum, String, String)   { return Value::createString(std::string(a.as_string_ptr()) + std::string(b.as_string_ptr())); };

    BIN_OP(Subtraction, Integer, Integer) { return Value::createInteger(a.as_integer() - b.as_integer()); };
    BIN_OP(Subtraction, Integer, Float)   { return Value::createFloat((float) a.as_integer() - b.as_float()); };
    BIN_OP(Subtraction, Float, Integer)   { return Value::createFloat(a.as_float() - (float) b.as_integer()); };
    BIN_OP(Subtraction, Float, Float)     { return Value::createFloat(a.as_float() - b.as_float()); };

    BIN_OP(Multiplication, Integer, Integer) { return Value::createInteger(a.as_integer() * b.as_integer()); };
    BIN_OP(Multiplication, Integer, Float)   { return Value::createFloat((float) a.as_integer() * b.as_float()); };
    BIN_OP(Multiplication, Float, Integer)   { return Value::createFloat(a.as_float() * (float) b.as_integer()); };
    BIN_OP(Multiplication, Float, Float)     { return Value::createFloat(a.as_float() * b.as_float()); };

    BIN_OP(Division, Integer, Integer) { return Value::createFloat((float) a.as_integer() / (float) b.as_integer()); };
    BIN_OP(Division, Integer, Float)   { return Value::createFloat((float) a.as_integer() / b.as_float()); };
    BIN_OP(Division, Float, Integer)   { return Value::createFloat(a.as_float() / (float) b.as_integer()); };
    BIN_OP(Division, Float, Float)     { return Value::createFloat(a.as_float() / b.as_float()); };

    BIN_OP(IntegerDivision, Integer, Integer) { return Value::createInteger(a.as_integer() / b.as_integer()); };
    BIN_OP(IntegerDivision, Integer, Float)   { return Value::createInteger(a.as_integer() / (int32_t) b.as_float()); };
    BIN_OP(IntegerDivision, Float, Integer)   { return Value::createInteger((int32_t) a.as_float() / b.as_integer()); };
    BIN_OP(IntegerDivision, Float, Float)     { return Value::createInteger((int32_t) a.as_float() / (int32_t) b.as_float()); };

    BIN_OP(Equality, Integer, Integer) { return Value::createIntegerFromBool(a.as_integer() == b.as_integer()); };
    BIN_OP(Equality, Integer, Float)   { return Value::createIntegerFromBool(std::abs((float) a.as_integer() - b.as_float()) < FLOAT_EPSILON); };
    BIN_OP(Equality, Float, Integer)   { return Value::createIntegerFromBool(std::abs(a.as_float() - (float) b.as_integer()) < FLOAT_EPSILON); };
    BIN_OP(Equality, Float, Float)     { return Value::createIntegerFromBool(std::abs(a.as_float() - b.as_float()) < FLOAT_EPSILON); };
    BIN_OP(Equality, String, String)   { return Value::createIntegerFromBool(strcmp(a.as_string_ptr(), b.as_string_ptr()) == 0); };

    BIN_OP(Inequality, Integer, Integer) { return Value::createIntegerFromBool(a.as_integer() != b.as_integer()); };
    BIN_OP(Inequality, Integer, Float)   { return Value::createIntegerFromBool(std::abs((float) a.as_integer() - b.as_float()) >= FLOAT_EPSILON); };
    BIN_OP(Inequality, Float, Integer)   { return Value::createIntegerFromBool(std::abs(a.as_float() - (float) b.as_integer()) >= FLOAT_EPSILON); };
    BIN_OP(Inequality, Float, Float)     { return Value::createIntegerFromBool(std::abs(a.as_float() - b.as_float()) >= FLOAT_EPSILON); };
    BIN_OP(Inequality, String, String)   { return Value::createIntegerFromBool(strcmp(a.as_string_ptr(), b.as_string_ptr()) != 0); };

    BIN_OP(LessThan, Integer, Integer) { return Value::createIntegerFromBool(a.as_integer() < b.as_integer()); };
    BIN_OP(LessThan, Integer, Float)   { return Value::createIntegerFromBool((float) a.as_integer() < b.as_float()); };
    BIN_OP(LessThan, Float, Integer)   { return Value::createIntegerFromBool(a.as_float() < (float) b.as_integer()); };
    BIN_OP(LessThan, Float, Float)     { return Value::createIntegerFromBool(a.as_float() < b.as_float()); };

    BIN_OP(LessThanOrEquals, Integer, Integer) { return Value::createIntegerFromBool(a.as_integer() <= b.as_integer()); };
    BIN_OP(LessThanOrEquals, Integer, Float)   { return Value::createIntegerFromBool((float) a.as_integer() <= b.as_float()); };
    BIN_OP(LessThanOrEquals, Float, Integer)   { return Value::createIntegerFromBool(a.as_float() <= (float) b.as_integer()); };
    BIN_OP(LessThanOrEquals, Float, Float)     { return Value::createIntegerFromBool(a.as_float() <= b.as_float()); };

    BIN_OP(GreaterThan, Integer, Integer) { return Value::createIntegerFromBool(a.as_integer() > b.as_integer()); };
    BIN_OP(GreaterThan, Integer, Float)   { return Value::createIntegerFromBool((float) a.as_integer() > b.as_float()); };
    BIN_OP(GreaterThan, Float, Integer)   { return Value::createIntegerFromBool(a.as_float() > (float) b.as_integer()); };
    BIN_OP(GreaterThan, Float, Float)     { return Value::createIntegerFromBool(a.as_float() > b.as_float()); };

    BIN_OP(GreaterThanOrEquals, Integer, Integer) { return Value::createIntegerFromBool(a.as_integer() >= b.as_integer()); };
    BIN_OP(GreaterThanOrEquals, Integer, Float)   { return Value::createIntegerFromBool((float) a.as_integer() >= b.as_float()); };
    BIN_OP(GreaterThanOrEquals, Float, Integer)   { return Value::createIntegerFromBool(a.as_float() >= (float) b.as_integer()); };
    BIN_OP(GreaterThanOrEquals, Float, Float)     { return Value::createIntegerFromBool(a.as_float() >= b.as_float()); };

    BIN_OP(Power, Integer, Integer) { return Value::createInteger((int32_t) powl(a.as_integer(), b.as_integer())); };
    BIN_OP(Power, Integer, Float)   { return Value::createFloat(powf((float) a.as_integer(), b.as_float())); };
    BIN_OP(Power, Float, Integer)   { return Value::createFloat(powf(a.as_float(), (float) b.as_integer())); };
    BIN_OP(Power, Float, Float)     { return Value::createFloat(powf(a.as_float(), b.as_float())); };

    BIN_OP(Modulo, Integer, Integer) { return Value::createInteger(a.as_integer() % b.as_integer()); };
    BIN_OP(ShiftLeft, Integer, Integer) { return Value::createInteger(a.as_integer() << b.as_integer()); };
    BIN_OP(ShiftRight, Integer, Integer) { return Value::createInteger(a.as_integer() >> b.as_integer()); };
    BIN_OP(BitwiseAnd, Integer, Integer) { return Value::createInteger(a.as_integer() & b.as_integer()); };
    BIN_OP(BitwiseOr, Integer, Integer) { return Value::createInteger(a.as_integer() | b.as_integer()); };
    BIN_OP(BitwiseXor, Integer, Integer) { return Value::createInteger(a.as_integer() ^ b.as_integer()); };

#undef BIN_OP

    return 0;
}();

Value binary_operation(Value const& a, Value const& b, BinaryOperationType op)
{
    return binary_ops[(size_t) op][(size_t) b.type()][(size_t) a.type()](a, b);
}

}
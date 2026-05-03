#ifndef TYCHE_VALUE_HH
#define TYCHE_VALUE_HH

#include <cstdint>
#include <string>
#include <variant>

namespace tyche::vm {

using FunctionId = uint32_t;

enum class Type : uint8_t
{
    Nil = 0, Integer, Float, String, Array, Table, Function, NativePointer, COUNT
};

std::string type_name(Type type);

class Value {
    struct Function { FunctionId f_id; };

public:
    Value() : value_(std::monostate()) {}

    static Value createNil() { return Value(std::monostate()); }
    static Value createInteger(int32_t v) { return Value(v); }
    static Value createFloat(float f) { return Value(f); }
    static Value createString(std::string const& str) { return Value(str); }
    static Value createStringFromConstant(const char* str) { return Value(str); }
    static Value createFunctionId(FunctionId f_id) { return Value(Function { f_id }); }

    static Value createFalse() { return createInteger(0); }
    static Value createTrue() { return createInteger(1); }
    static Value createIntegerFromBool(bool b) { return createInteger(b ? 1 : 0); }

    [[nodiscard]] Type type() const;

    [[nodiscard]] int32_t     as_integer() const     { return std::get<int32_t>(value_); }
    [[nodiscard]] float       as_float() const       { return std::get<float>(value_); }
    [[nodiscard]] const char* as_string_ptr() const;
    [[nodiscard]] FunctionId  as_function_id() const { return std::get<Function>(value_).f_id; }

    [[nodiscard]] std::string to_string() const;

private:
    using Internal = std::variant<std::monostate, int32_t, float, std::string, const char*, Function>;
    Internal value_;

    explicit Value(Internal const& internal) : value_(internal) {}
};

}

#endif //TYCHE_VALUE_HH

#ifndef TYCHE_VALUE_HH
#define TYCHE_VALUE_HH

#include <string>
#include <variant>

namespace tyche {

enum class Type : uint8_t
{
    Nil = 0, Integer, Float, String, Array, Table, Function, NativePointer,
};

class Value {
public:
    static Value CreateNil() { return Value(std::monostate()); }
    static Value CreateInteger(int32_t v) { return Value(v); }
    static Value CreateFloat(float f) { return Value(f); }
    static Value CreateString(std::string const& str) { return Value(str); }

    [[nodiscard]] Type type() const;

    [[nodiscard]] int32_t     as_integer() const { return std::get<int32_t>(value_); }
    [[nodiscard]] float       as_float() const   { return std::get<float>(value_); }
    [[nodiscard]] std::string as_string() const  { return std::get<std::string>(value_); }

private:
    using Internal = std::variant<std::monostate, int32_t, float, std::string>;
    Internal value_;

    explicit Value(Internal const& internal) : value_(internal) {}
};

}

#endif //TYCHE_VALUE_HH

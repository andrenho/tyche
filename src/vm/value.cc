#include "value.hh"

#include "../common/overloaded.hh"

namespace tyche::vm {

std::string type_name(Type type)
{
    switch (type) {
        case Type::Nil: return "nil";
        case Type::Integer: return "integer";
        case Type::Float: return "float";
        case Type::String: return "string";
        case Type::Array: return "array";
        case Type::Table: return "table";
        case Type::Function: return "function";
        case Type::NativePointer: return "native pointer";
        case Type::COUNT: default: return "???";
    }
}

Type Value::type() const
{
    return std::visit(overloaded {
        [](std::monostate)     { return Type::Nil; },
        [](int32_t)            { return Type::Integer; },
        [](float)              { return Type::Float; },
        [](std::string const&) { return Type::String; },
        [](Function const&)    { return Type::Function; },
    }, value_);
}

std::string Value::to_string() const
{
    return std::visit(overloaded {
        [](std::monostate)       { return std::string("nil"); },
        [](int32_t i)            { return std::to_string(i); },
        [](float f)              { return std::to_string(f); },
        [](std::string const& s) { return s; },
        [](Function const& f)    { return "@" + std::to_string(f.f_id); }
    }, value_);
}

const char* Value::as_string_ptr() const
{
    if (auto s = std::get_if<std::string>(&value_))
        return s->c_str();
    if (auto s = std::get_if<const char*>(&value_))
        return *s;
    throw std::logic_error("Shouldn't get here");
}

}

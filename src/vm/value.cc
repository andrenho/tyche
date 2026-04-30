#include "value.hh"

#include "../common/overloaded.hh"

namespace tyche::vm {

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

}

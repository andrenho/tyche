#include "value.hh"

#include "../common/overloaded.hh"

namespace tyche {

Type Value::type() const
{
    return std::visit(overloaded {
        [](std::monostate)     { return Type::Nil; },
        [](int32_t)            { return Type::Integer; },
        [](float)              { return Type::Float; },
        [](std::string const&) { return Type::String; },
    }, value_);
}

}

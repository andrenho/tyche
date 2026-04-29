#include "stack.hh"

#include "vm_exceptions.hh"

namespace tyche {

void Stack::push(Value const& value)
{
    stack_.push_back(value);
}

Value Stack::pop()
{
    if (stack_.empty())
        throw VMStackUnderflow();

    Value v = stack_.back();
    stack_.pop_back();
    return v;
}

Value Stack::at(int pos) const
{
    try {
        if (pos >= 0)
            return stack_.at(pos);
        else
            return stack_.at(stack_.size() + pos);
    } catch (std::out_of_range&) {
        throw VMStackOutOfRange();
    }
}

size_t Stack::size() const
{
    return stack_.size();
}

} // tyche

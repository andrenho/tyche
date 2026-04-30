#include "stack.hh"

#include "vm_exceptions.hh"

namespace tyche::vm {

Stack::Stack()
{
    fps_.push(0);
}

void Stack::push(Value const& value)
{
    stack_.push_back(value);
}

Value Stack::pop()
{
    if (stack_.size() <= fps_.top())
        throw VMStackUnderflow();

    Value v = stack_.back();
    stack_.pop_back();
    return v;
}

Value Stack::at(int pos) const
{
    try {
        if (pos >= 0) {
            return stack_.at(fps_.top() + pos);
        } else {
            if ((int) fps_.top() + (int) stack_.size() + pos < 0)
                throw VMStackOutOfRange();
            return stack_.at(stack_.size() + pos);
        }
    } catch (std::out_of_range&) {
        throw VMStackOutOfRange();
    }
}

size_t Stack::size() const
{
    return stack_.size() - fps_.top();
}

void Stack::push_fp()
{
    fps_.push(stack_.size());
}

void Stack::pop_fp()
{
    if (fps_.size() == 1)
        throw VMStackUnderflow();

    stack_.resize(fps_.top());
    fps_.pop();
}

std::string Stack::debug() const
{
    if (stack_.empty())
        return "empty";

    std::string out;
    for (size_t i = 0; i < stack_.size(); ++i)
        out += "[" + stack_.at(i).to_string() + "] ";
    return out;
}

} // tyche

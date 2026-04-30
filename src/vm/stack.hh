#ifndef TYCHE_STACK_HH
#define TYCHE_STACK_HH

#include <stack>
#include <vector>

#include "value.hh"

namespace tyche::vm {

class Stack {
public:
    Stack();

    void push(Value const& value);
    Value pop();

    [[nodiscard]] Value at(int pos) const;
    [[nodiscard]] size_t size() const;

    void push_fp();
    void pop_fp();

    [[nodiscard]] size_t fp_level() const { return fps_.size(); }

    [[nodiscard]] std::string debug() const;

private:
    std::vector<Value> stack_;
    std::stack<size_t> fps_;
};

} // tyche

#endif //TYCHE_STACK_HH

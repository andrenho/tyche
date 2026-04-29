#ifndef TYCHE_STACK_HH
#define TYCHE_STACK_HH

#include <vector>

#include "value.hh"

namespace tyche {

class Stack {
public:
    void push(Value const& value);
    Value pop();

    [[nodiscard]] Value at(int pos) const;
    [[nodiscard]] size_t size() const;

private:
    std::vector<Value> stack_;
};

} // tyche

#endif //TYCHE_STACK_HH

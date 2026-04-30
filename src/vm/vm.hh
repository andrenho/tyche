#ifndef TYCHE_VM_HH
#define TYCHE_VM_HH

#include "code.hh"
#include "location.hh"
#include "stack.hh"

namespace tyche::vm {

class VM {
public:
    void load_bytecode(ByteArray const& ba);

    void call(size_t n_params);

    [[nodiscard]] int32_t to_integer(int index) const;

    void push_integer(int32_t value);

    [[nodiscard]] std::string debug_stack() const { return stack_.debug(); }

private:
    void run_until_return();
    void step();

    static void assert_type(Value const& val, Type type);

    Stack stack_;
    Code code_;
    std::stack<Location> loc_;
};

} // tyche

#endif //TYCHE_VM_HH

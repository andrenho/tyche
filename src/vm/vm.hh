#ifndef TYCHE_VM_HH
#define TYCHE_VM_HH

#include "code.hh"
#include "location.hh"
#include "stack.hh"

namespace tyche::vm {

class VM {
public:
    VM& load_bytecode(ByteArray const& ba);

    VM& call(size_t n_params);

    [[nodiscard]] bool is_nil(int index) const              { return stack_.at(index).type() == Type::Nil; }
    [[nodiscard]] bool is_integer(int index) const          { return stack_.at(index).type() == Type::Integer; }
    [[nodiscard]] bool is_float(int index) const            { return stack_.at(index).type() == Type::Float; }
    [[nodiscard]] bool is_string(int index) const           { return stack_.at(index).type() == Type::String; }
    [[nodiscard]] bool is_array(int index) const            { return stack_.at(index).type() == Type::Array; }
    [[nodiscard]] bool is_table(int index) const            { return stack_.at(index).type() == Type::Table; }
    [[nodiscard]] bool is_function(int index) const         { return stack_.at(index).type() == Type::Function; }
    [[nodiscard]] bool is_native_pointer(int index) const   { return stack_.at(index).type() == Type::NativePointer; }

    [[nodiscard]] size_t stack_sz() const                   { return stack_.size(); }

    VM& push_nil();
    VM& push_integer(int32_t value);
    VM& push_float(float value);
    VM& push_string(std::string const& string);

    [[nodiscard]] int32_t     to_integer(int index) const;
    [[nodiscard]] float       to_float(int index) const;
    [[nodiscard]] std::string to_string(int index) const;

    [[nodiscard]] std::string debug_stack() const           { return stack_.debug(); }

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

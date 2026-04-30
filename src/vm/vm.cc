#include "vm.hh"

#include "vm_exceptions.hh"
#include "expr.hh"

namespace tyche::vm {

void VM::load_bytecode(ByteArray const& ba)
{
    FunctionId f_id = code_.import_bytecode(ba);
    stack_.push(Value::CreateFunctionId(f_id));
}

void VM::call(size_t n_params)
{
    // TODO - parameters

    Value f = stack_.pop();
    if (f.type() != Type::Function)
        throw VMTypeError(Type::Function, f.type());

    loc_.emplace(f.as_function_id(), 0);
    stack_.push_fp();
    run_until_return();
    // stack_.pop_fp();
    loc_.pop();
}

int32_t VM::to_integer(int index) const
{
    Value i = stack_.at(index);
    assert_type(i, Type::Integer);
    return i.as_integer();
}

void VM::push_integer(int32_t value)
{
    stack_.push(Value::CreateInteger(value));
}

void VM::run_until_return()
{
    size_t level = stack_.fp_level();

    while (stack_.fp_level() >= level)
        step();
}

void VM::step()
{
    Operation op = code_.operation(loc_.top());
    switch (op.instruction) {
        case Instruction::PushInt8:
        case Instruction::PushInt16:
        case Instruction::PushInt32:
            push_integer(op.operator_);
            break;
        case Instruction::Sum:
            stack_.push(binary_operation(stack_.pop(), stack_.pop(), BinaryOperationType::Sum));
            break;
        case Instruction::Return: {
            Value v = stack_.pop();
            stack_.pop_fp();
            stack_.push(v);
            return;
        }
        default:
            throw VMInvalidOpcode((uint8_t) op.instruction);
    }
    loc_.top() = op.next_location;
}

void VM::assert_type(Value const& val, Type type)
{
    if (val.type() != type)
        throw VMTypeError(type, val.type());
}

} // tyche

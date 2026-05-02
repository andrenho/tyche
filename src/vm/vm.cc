#include "vm.hh"

#include "vm_exceptions.hh"
#include "expr.hh"

namespace tyche::vm {

VM& VM::load_bytecode(ByteArray const& ba)
{
    FunctionId f_id = code_.import_bytecode(ba);
    stack_.push(Value::createFunctionId(f_id));
    return *this;
}

VM& VM::call(size_t n_params)
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

    return *this;
}

int32_t VM::to_integer(int index) const
{
    Value i = stack_.at(index);
    if (i.type() == Type::Integer)
        return i.as_integer();
    if (i.type() == Type::Float)
        return (int32_t) i.as_float();
    throw VMTypeError(Type::Integer, i.type());
}

float VM::to_float(int index) const
{
    Value f = stack_.at(index);
    if (f.type() == Type::Float)
        return f.as_float();
    if (f.type() == Type::Integer)
        return (float) f.as_integer();
    throw VMTypeError(Type::Float, f.type());
}

std::string VM::to_string(int index) const
{
    Value i = stack_.at(index);
    assert_type(i, Type::String);
    return i.as_string();
}

VM& VM::push_nil()
{
    stack_.push(Value::createNil());
    return *this;
}

VM& VM::push_integer(int32_t value)
{
    stack_.push(Value::createInteger(value));
    return *this;
}

VM& VM::push_float(float value)
{
    stack_.push(Value::createFloat(value));
    return *this;
}

VM& VM::push_string(std::string const& str)
{
    stack_.push(Value::createString(str));
    return *this;
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

        //
        // stack management
        //

        case Instruction::PushInt8:
        case Instruction::PushInt16:
        case Instruction::PushInt32:
            push_integer(op.operator_);
            break;

        case Instruction::PushConstant8:
        case Instruction::PushConstant16:
        case Instruction::PushConstant32: {
            auto cnst = code_.bytecode().get_constant(op.operator_);
            if (auto f = std::get_if<float>(&cnst))
                push_float(*f);
            else if (auto s = std::get_if<std::string>(&cnst))
                push_string(*s);
            else
                throw std::logic_error("Shouldn't get here");
            break;
        }

        case Instruction::PushFunction8:
        case Instruction::PushFunction16:
        case Instruction::PushFunction32:
            stack_.push(Value::createFunctionId(op.operator_));
            break;

        case Instruction::Pop:
            stack_.pop();
            break;

        case Instruction::Duplicate:
            stack_.push(stack_.peek());
            break;

        //
        // logical/arithmetic
        //

#define BIN_OP(op) { Value a = stack_.pop(); Value b = stack_.pop(); stack_.push(binary_operation(a, b, BinaryOperationType::op)); }
        case Instruction::Sum:           BIN_OP(Sum)                 break;
        case Instruction::Subtract:      BIN_OP(Subtraction)         break;
        case Instruction::Multiply:      BIN_OP(Multiplication)      break;
        case Instruction::Divide:        BIN_OP(Division)            break;
        case Instruction::DivideInt:     BIN_OP(IntegerDivision)     break;
        case Instruction::Equals:        BIN_OP(Equality)            break;
        case Instruction::NotEquals:     BIN_OP(Inequality)          break;
        case Instruction::LessThan:      BIN_OP(LessThan)            break;
        case Instruction::LessThanEq:    BIN_OP(LessThanOrEquals)    break;
        case Instruction::GreaterThan:   BIN_OP(GreaterThan)         break;
        case Instruction::GreaterThanEq: BIN_OP(GreaterThanOrEquals) break;
        case Instruction::And:           BIN_OP(BitwiseAnd)          break;
        case Instruction::Or:            BIN_OP(BitwiseOr)           break;
        case Instruction::Xor:           BIN_OP(BitwiseXor)          break;
        case Instruction::Power:         BIN_OP(Power)               break;
        case Instruction::ShiftLeft:     BIN_OP(ShiftLeft)           break;
        case Instruction::ShiftRight:    BIN_OP(ShiftRight)          break;
        case Instruction::Modulo:        BIN_OP(Modulo)              break;
#undef BIN_OP

        //
        // function operations
        //

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

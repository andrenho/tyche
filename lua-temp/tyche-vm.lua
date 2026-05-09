local pprint = require('pprint')

local TYPES = { 'nil', 'integer', 'float', 'string', 'array', 'table', 'function', 'native_pointer' }
local TYPE_MAP = {}; for _,v in ipairs(TYPES) do TYPE_MAP[v] = true end

local ARITH_LOGIC_OPS = {
    sum=true, sub=true, mul=true, div=true, idiv=true, eq=true, neq=true, lt=true, lte=true, gt=true, gte=true,
    ['and']=true, ['or']=true, xor=true, pow=true, shl=true, shr=true, mod=true
}

----------------------
--                  --
--       UTIL       --
--                  --
----------------------

local function validate_value(v)
    assert(v, "value cannot be nil")
    assert(type(v) == 'table',
        "invalid value format (expected { type='...', value=... }), received: " .. pprint.pformat(v))
    assert(TYPE_MAP[v.type], "missing field 'type' in value")
    if v.type == 'nil' then
        assert(v.value == nil)
    elseif v.type == 'number' then
        assert(type(v.value) == 'number')
    elseif v.type == 'function' then
        assert(type(v.value) == 'number' and v.value >= 0, "function must be a positive number")
    end
end

function is_zero(v)
    if v.type == 'nil' then return true end
    if v.type == 'integer' and v.value == 0 then return true end
    return false
end

----------------------
--                  --
--      STACK       --
--                  --
----------------------

local Stack = {}
Stack.__index = Stack

function Stack.new()
    local self = setmetatable({
        stack = {},
        fps = {},
    }, Stack)
    self:push_fp()
    return self
end

function Stack:top_fps()
    return self.fps[#self.fps]
end

function Stack:push(value)
    validate_value(value)
    table.insert(self.stack, value)
end

function Stack:pop()
    if #self.stack < self:top_fps() then error("Stack underflow") end
    local v = self.stack[#self.stack]
    self.stack[#self.stack] = nil
    return v
end

function Stack:peek()
    if #self.stack < self:top_fps() then error("Stack underflow") end
    return self.stack[#self.stack]
end

Stack.__len = function(self)
    return #self.stack - self:top_fps() + 1
end

Stack.__index = function(self, key)
    local idx = tonumber(key)
    if idx then
        if idx >= 0 then
            return self.stack[self:top_fps() + idx]
        else
            if self:top_fps() + #self.stack + idx < 0 then error("Stack access out of range") end
            return self.stack[#self.stack + idx + 1]
        end
    else
        return Stack[key]   -- other methods
    end
end

Stack.__newindex = function(self, key, value)
    validate_value(value)
    local idx = tonumber(key)
    if idx then
        if idx >= 0 then
            self.stack[self:top_fps() + idx] = value
        else
            if self:top_fps() + #self.stack + idx < 0 then error("Stack access out of range") end
            self.stack[#self.stack + idx + 1] = value
        end
    end
end

function Stack:push_fp()
    table.insert(self.fps, #self.stack + 1)
end

function Stack:pop_fp()
    if #self.fps == 1 then error("FPS queue underflow") end
    for i=self:top_fps(),#self.stack,1 do
        self.stack[i] = nil
    end
    table.remove(self.fps)
end

function Stack:fp_level()
    return #self.fps
end


----------------------
--                  --
--      CODE        --
--                  --
----------------------

local Code = {}
Code.__index = Code

function Code.new()
    return setmetatable({
        bytecode = nil
    }, Code)
end

function Code:load(bytecode)
    -- TODO - what if there's code already loaded?
    self.bytecode = bytecode
    return 0   -- main function
end

function Code:next_instruction(function_id, pc)
    return {
        operator = self.bytecode.functions[function_id][pc][1],
        operand = self.bytecode.functions[function_id][pc][2],
        instruction_size = 1,
    }
end

function Code:find_label(function_id, label)
    for pc, op in ipairs(self.bytecode.functions[function_id]) do
        if op.labels then
            for _,lbl in ipairs(op.labels) do
                if lbl == label then
                    return pc
                end
            end
        end
    end
end

----------------------
--                  --
--      EXPR        --
--                  --
----------------------

local EXPR = {}

-- initialize default
for op,_ in pairs(ARITH_LOGIC_OPS) do
    EXPR[op] = {}
    for _,type1 in ipairs(TYPES) do
        EXPR[op][type1] = {}
        for _,type2 in ipairs(TYPES) do
            EXPR[op][type1][type2] = function(_, _, _) error(string.format("Type mismatch for operation '%s': types '%s' and '%s'", op, type1, type2)) end
        end
    end
end

EXPR.sum.integer.integer = function(vm, b, a) vm:push_integer(a + b) end
EXPR.sub.integer.integer = function(vm, b, a) vm:push_integer(a - b) end
EXPR.mul.integer.integer = function(vm, b, a) vm:push_integer(a * b) end
-- TODO - div
EXPR.idiv.integer.integer = function(vm, b, a) vm:push_integer(math.floor(a / b)) end
EXPR.mod.integer.integer  = function(vm, b, a) vm:push_integer(a % b) end
EXPR.eq.integer.integer   = function(vm, b, a) vm:push_integer((a == b) and 1 or 0) end
EXPR.neq.integer.integer  = function(vm, b, a) vm:push_integer((a ~= b) and 1 or 0) end
EXPR.lt.integer.integer   = function(vm, b, a) vm:push_integer((a < b) and 1 or 0) end
EXPR.lte.integer.integer  = function(vm, b, a) vm:push_integer((a <= b) and 1 or 0) end
EXPR.gt.integer.integer   = function(vm, b, a) vm:push_integer((a > b) and 1 or 0) end
EXPR.gte.integer.integer  = function(vm, b, a) vm:push_integer((a >= b) and 1 or 0) end
EXPR['and'].integer.integer = function(vm, b, a) vm:push_integer(a & b) end
EXPR['or'].integer.integer = function(vm, b, a) vm:push_integer(a | b) end
EXPR.xor.integer.integer = function(vm, b, a) vm:push_integer(a ~ b) end
EXPR.pow.integer.integer = function(vm, b, a) vm:push_integer(a ^ b) end
EXPR.shl.integer.integer = function(vm, b, a) vm:push_integer(a << b) end
EXPR.shr.integer.integer = function(vm, b, a) vm:push_integer(a >> b) end

----------------------
--                  --
--       VM         --
--                  --
----------------------

local VM = {}
VM.__index = VM

function VM.new()
    return setmetatable({
        stack = Stack.new(),
        code = Code.new(),
        loc = {},
        debug = false,
    }, VM)
end

function VM:set_debug(b)
    self.debug = b
    return self
end

--
-- code management
--

function VM:load(bytecode)
    local f_id = self.code:load(bytecode)
    self.stack:push({ type = 'function', value = f_id })
    return self
end

--
-- stack management
--

function VM:push_integer(n)
    self.stack:push({ type = 'integer', value = n })
end

function VM:push_nil()
    self.stack:push({ type = 'nil' })
end

--
-- information
--

function VM:stack_sz()
    return #self.stack
end

function VM:is(idx, type_)
    return self.stack[idx].type == type_
end

function VM:to_integer(idx)
    local value = self.stack[idx]
    if value.type ~= 'integer' then error("Type error: not an integer") end
    return value.value
end

function VM:format_value(v)
    if v.type == 'integer' or v.type == 'real' then
        return tostring(v.value)
    elseif v.type == 'string' then
        return '"' .. v.value .. '"'
    elseif v.type == 'function' then
        return '@' .. tostring(v.value)
    elseif v.type == 'nil' then
        return 'nil'
    else
        return pprint.pformat(v)
    end
end

function VM:debug_stack()
    if #self.stack.stack == 0 then return "empty" end
    local ss = {}
    for i,v in ipairs(self.stack.stack) do
        for _,fp in pairs(self.stack.fps) do
            if i == fp then table.insert(ss, '^ ') end
        end
        table.insert(ss, '[' .. self:format_value(v) .. '] ')
    end
    return table.concat(ss)
end


--
-- code execution
--

function VM:_enter_function(n_pars)
    -- get parameters
    local vars = {}
    for i=1,n_pars do
        vars[i] = self.stack:pop()
    end

    -- get function
    local f = self.stack:pop()
    if f.type ~= 'function' then error("Type error: expected function") end

    -- enter function
    table.insert(self.loc, {
        f_id = f.value,
        pc = 1
    })
    self.stack:push_fp()

    -- pass parameters
    for i=1,n_pars do
        self.stack:push(vars[#vars-i+1])
    end
end

function VM:call(n_pars)
    self:_enter_function(n_pars)
    self:_run_until_return()
    return self
end

function VM:_run_until_return()
    local level = self.stack:fp_level()
    while self.stack:fp_level() >= level do
        self:_step()
    end
end

function VM:_print_stack()
    if self.debug then
        print(self:debug_stack())
    end
end

function VM:_step()
    local loc = self.loc[#self.loc]
    local op = self.code:next_instruction(loc.f_id, loc.pc)

    if self.debug then print('## ' .. loc.f_id .. ':' .. loc.pc .. '   ' .. op.operator .. ' ' .. (op.operand and op.operand or '')) end

    --
    -- stack operations
    --

    if op.operator == 'pushi' then
        self:push_integer(op.operand)

    elseif op.operator == 'pushf' then
        assert(op.operand >= 0)
        self.stack:push({ type = 'function', value = op.operand })

    elseif op.operator == 'dup' then
        self.stack:push(self.stack:peek())

    --
    -- local variables
    --

    elseif op.operator == 'pushv' then
        assert(op.operand >= 0)
        for _=1,op.operand do
            self:push_nil()
        end

    elseif op.operator == 'set' then
        assert(op.operand >= 0)
        local a = self.stack:pop()
        self.stack[op.operand] = a

    elseif op.operator == 'dupv' then
        assert(op.operand >= 0)
        local a = self.stack[op.operand]
        self.stack:push(a)

    --
    -- logic/arithmetic operations
    --

    elseif ARITH_LOGIC_OPS[op.operator] then
        local a = self.stack:pop()
        local b = self.stack:pop()
        EXPR[op.operator][a.type][b.type](self, a.value, b.value)

    --
    -- function management
    ---

    elseif op.operator == 'call' then
        assert(op.operand >= 0)
        self:_enter_function(op.operand)

    elseif op.operator == 'ret' then
        local v = self.stack:pop()
        self.stack:pop_fp()
        self.stack:push(v)
        table.remove(self.loc)
        self:_print_stack()
        return

    --
    -- jumps/branching
    --

    elseif op.operator == 'jmp' then
        loc.pc = self.code:find_label(loc.f_id, op.operand)
        self:_print_stack()
        return

    elseif op.operator == 'bz' then
        local v = self.stack:pop()
        if is_zero(v) then
            loc.pc = self.code:find_label(loc.f_id, op.operand)
            self:_print_stack()
            return
        end

    elseif op.operator == 'bnz' then
        local v = self.stack:pop()
        if not is_zero(v) then
            loc.pc = self.code:find_label(loc.f_id, op.operand)
            self:_print_stack()
            return
        end

    --
    -- instruction not found
    --

    else
        error("Unknown operator '" .. tostring(op.operator) .. "'")
    end

    self:_print_stack()

    loc.pc = loc.pc + op.instruction_size
end

return VM

local pprint = require('pprint')

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
    table.insert(self.stack, value)
end

function Stack:pop(value)
    if #self.stack < self:top_fps() then error("Stack underflow") end
    local v = self.stack[#self.stack]
    self.stack[#self.stack] = nil
    return v
end

function Stack:peek(value)
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

function Stack:debug()
    if #self.stack == 0 then return "empty" end
    local ss = {}
    for _,v in ipairs(self.stack) do table.insert(ss, '[' .. pprint.pformat(v) .. '] ') end
    return table.concat(ss)
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

--
-- information
--

function VM:stack_sz()
    return #self.stack
end

function VM:is(idx, type)
    return self.stack[idx].type == type
end

function VM:to_integer(idx)
    local value = self.stack[idx]
    if value.type ~= 'integer' then error("Type error: not an integer") end
    return value.value
end

--
-- code execution
--

function VM:call(n_pars)
    local f = self.stack:pop()
    if f.type ~= 'function' then error("Type error: expected function") end
    table.insert(self.loc, {
        f_id = f.value,
        pc = 1
    })
    self.stack:push_fp()
    self:_run_until_return()  -- execute code
    table.remove(self.loc)
    return self
end

function VM:_run_until_return()
    local level = self.stack:fp_level()
    while self.stack:fp_level() >= level do
        self:_step()
    end
end

function VM:_step()
    local loc = self.loc[#self.loc]
    local op = self.code:next_instruction(loc.f_id, loc.pc)

    if self.debug then print('## ' .. loc.f_id .. ':' .. loc.pc .. '   ' .. op.operator .. ' ' .. (op.operand and op.operand or '')) end

    if op.operator == 'pushi' then
        self:push_integer(op.operand)
    elseif op.operator == 'sum' then
        self:push_integer(self.stack:pop().value + self.stack:pop().value)
    elseif op.operator == 'ret' then
        local v = self.stack:pop()
        self.stack:pop_fp()
        self.stack:push(v)
        return
    else
        error("Unknown operator '" .. tostring(op.operator) .. "'")
    end

    if self.debug then print(self.stack:debug()) end

    loc.pc = loc.pc + op.instruction_size
end

return VM
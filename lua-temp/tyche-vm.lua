local pprint = require('pprint')

----------------------
--                  --
--      STACK       --
--                  --
----------------------

local Stack = {}
Stack.__index = Stack

function Stack.new()
    return setmetatable({
        stack = {},
        fps = { 0 },
    }, Stack)
end

function Stack:top_fps()
    return self.fps[#self.fps]
end

function Stack:push(value)
    table.insert(self.stack, value)
end

function Stack:pop(value)
    if #self.stack <= self:top_fps() then error("Stack underflow") end
    local v = self.stack[#self.stack]
    self.stack[#self.stack] = nil
    return v
end

function Stack:peek(value)
    if #self.stack <= self:top_fps() then error("Stack underflow") end
    return self.stack[#self.stack]
end

Stack.__len = function(self)
    return #self.stack - self:top_fps()
end

Stack.__index = function(self, key)
    local idx = tonumber(key)
    if idx then
        if idx >= 0 then
            return self.stack[self:top_fps() + idx + 1]
        else
            if self:top_fps() + #self.stack + idx < 0 then error("Stack access out of range") end
            return self.stack[#self.stack + idx + 1]
        end
    else
        return Stack[key]   -- other methods
    end
end

Stack.__newindex = function(self, key, value)
end

function Stack:push_fp()
end

function Stack:pop_fp()
end

function Stack:fp_level()
end

function Stack:debug()
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
        stack = Stack.new()
    }, VM)
end

return VM
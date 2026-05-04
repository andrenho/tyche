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
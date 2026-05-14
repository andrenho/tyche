----------------------
--                  --
--      PARSER      --
--                  --
----------------------

local function parse_assembly(source)
    local proto = {
        constants = {},
        functions = {},
    }

    local section = ''
    local current_f_id = 0

    local next_label = nil
    for line in source:gmatch("([^\n]+)") do
        local line = line:gsub("%s*;.*$", "") -- remove comments
        line = line:match("^%s*(.-)%s*$")     -- trim

        if #line == 0 then goto continue end

        if line == ".const" then
            section = 'const'
        elseif line:match("%.func%s+%d+") then
            section = 'function'
            local f_id = tonumber(line:match("%.func%s+(%d+)"))
            proto.functions[f_id] = {}
            current_f_id = f_id
        elseif section == 'const' then
            local k, v = line:match("^%s*(%d+)%s*:%s*(.+)$")
            if not k then error("Invalid row for constant: " .. line) end
            if v:sub(1, 1) == '"' then
                proto.constants[tonumber(k)] = line:match('"(.*)"')
            else
                proto.constants[tonumber(k)] = tonumber(v)
            end
        elseif section == 'function' then
            local regexes = {
                "^%s*(%a+)%s+(%d+)%s*$",            -- instruction + parameter
                "^%s*(%a+)%s+(@[%a_][%a%d_]*)%s*$", -- instruction + label
                "^%s*(%a+)%s*$",                    -- instruction only
                "^(@[%a_][%a%d_]*):%s*$",           -- label
            }
            local match = false
            for i, regex in ipairs(regexes) do
                local inst, par = line:match(regex)
                if inst then
                    match = true
                    if i == 1 then     -- instruction + parameter
                        table.insert(proto.functions[current_f_id], { inst, tonumber(par), labels = next_label })
                    elseif i == 2 then -- instruction + label
                        table.insert(proto.functions[current_f_id], { inst, par, labels = next_label })
                    elseif i == 3 then -- instruction only
                        table.insert(proto.functions[current_f_id], { inst, labels = next_label })
                    elseif i == 4 then -- label
                        if not next_label then
                            next_label = { inst }
                        else
                            table.insert(next_label, inst)
                        end
                    end
                    if i ~= 4 then
                        next_label = nil
                    end
                end
            end
            if not match then error("Invalid instruction: " .. line) end
        end

        ::continue::
    end

    return proto
end

----------------------
--                  --
--     BINARY       --
--                  --
----------------------

local MAGIC = 0xa7d6e9b1
local VERSION = 1

local function assemble(proto)
    local bin = {}

    local push16 = function(data)
        table.insert(bin, data & 0xff)
        table.insert(bin, (data >> 8) & 0xff)
        return #bin - 2
    end

    local push32 = function(data)
        table.insert(bin, data & 0xff)
        table.insert(bin, (data >> 8) & 0xff)
        table.insert(bin, (data >> 16) & 0xff)
        table.insert(bin, (data >> 24) & 0xff)
        return #bin - 4
    end

    local replace32 = function(pos, data)
        bin[pos] = data & 0xff
        bin[pos + 1] = (data >> 8) & 0xff
        bin[pos + 2] = (data >> 16) & 0xff
        bin[pos + 3] = (data >> 24) & 0xff
    end

    local float_to_le_bytes = function(f)
        local bytes = {}
        local packed = string.pack("<f", f)
        for i = 1, 4 do
            bytes[i] = packed:byte(i)
        end
        return bytes
    end

    -- header
    push32(MAGIC)
    push16(VERSION)
    push16(0)

    -- constants
    local code_addr_pos = push32(0) -- code address, to be replaced
    push32(#proto.constants)        -- number of constants
    local const_idx = {}
    for _, _ in ipairs(proto.constants) do
        table.insert(const_idx, push32(0)) -- constant address, to be replaced
    end
    for i, const in ipairs(proto.constants) do
        replace32(const_idx[i], #bin)
        if type(const) == 'string' then
            table.insert(bin, 0) -- string type
            push32(#const)
            for c in const:gmatch('.') do
                table.insert(bin, c:byte())
            end
        elseif type(const) == 'number' then
            table.insert(bin, 0) -- float type
            push32(float_to_le_bytes(const))
        end
    end

    replace32(code_addr_pos, #bin)

    -- code
    push32(0) -- debug address (TODO)
    push32(#proto.functions) -- number of functions
    for _,func in ipairs(proto.functions) do
        local next_function_pos = #bin
        push32(0)  -- to be replaced with next function address
        -- TODO - add code
        replace32(next_function_pos, #bin)
    end

    return string.char(table.unpack(bin))
end

----------------------
--                  --
--     GENERIC      --
--                  --
----------------------

return function(source)
    return assemble(parse_assembly(source))
end
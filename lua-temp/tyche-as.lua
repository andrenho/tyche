----------------------
--                  --
--      PARSER      --
--                  --
----------------------

local function assemble(source)
    local proto = {
        constants = {},
        functions = {},
    }

    local section = ''
    local current_f_id = 0

    for line in source:gmatch("([^\n]+)") do
        local line = line:gsub("%s*;.*$", "")   -- remove comments
        line = line:match("^%s*(.-)%s*$")       -- trim

        if #line == 1 then goto continue end

        if line == ".const" then
            section = 'const'
        elseif line:match("%.func%s+%d+") then
            section = 'function'
            local f_id = tonumber(line:match("%.func%s+(%d+)"))
            proto.functions[f_id] = {}
            current_f_id = f_id
        elseif section == 'const' then
            local k, v = line:match("^%s*(%d+)%s*:%s*(.+)$")
            if v:sub(1, 1) == '"' then
                proto.constants[tonumber(k)] = line:match('"(.*)"')
            else
                proto.constants[tonumber(k)] = tonumber(v)
            end
        elseif section == 'function' then
            local inst, par = line:match("^%s*(%a+)%s+(%d+)%s*$")
            if inst then
                table.insert(proto.functions[current_f_id], { inst, tonumber(par) })
            else
                local inst = line:match("^%s*(%a+)%s*$")
                table.insert(proto.functions[current_f_id], { inst })
            end
        end

        ::continue::
    end

    return proto
end

----------------------
--                  --
--       MAIN       --
--                  --
----------------------

if ... then
    return assemble
else
    error("Running assembler directly not supported yet")
end
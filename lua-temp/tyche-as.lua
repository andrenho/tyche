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

    local next_label = nil
    for line in source:gmatch("([^\n]+)") do
        local line = line:gsub("%s*;.*$", "")   -- remove comments
        line = line:match("^%s*(.-)%s*$")       -- trim

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
                "^%s*(%a+)%s+(%d+)%s*$",        -- instruction + parameter
                "^%s*(%a+)%s+(@[%a_]+)%s*$",    -- instruction + label
                "^%s*(%a+)%s*$",                -- instruction only
                "^(@[%a_]+):%s*$",              -- label
            }
            local match = false
            for i,regex in ipairs(regexes) do
                local inst, par = line:match(regex)
                if inst then
                    match = true
                    if i == 1 then       -- instruction + parameter
                        table.insert(proto.functions[current_f_id], { inst, tonumber(par), labels = next_label })
                    elseif i == 2 then   -- instruction + label
                        table.insert(proto.functions[current_f_id], { inst, par, labels = next_label })
                    elseif i == 3 then   -- instruction only
                        table.insert(proto.functions[current_f_id], { inst, labels = next_label })
                    elseif i == 4 then   -- label
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
--       MAIN       --
--                  --
----------------------

if ... then
    return assemble
else
    error("Running assembler directly not supported yet")
end
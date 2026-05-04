local assemble = require('tyche-as')

function tprint(o, indent)
    indent = indent or 0
    local spacing = string.rep("  ", indent)

    if type(o) == 'table' then
        local s = '{\n'
        for k, v in pairs(o) do
            -- Format keys: quote strings, leave numbers as is
            local key = type(k) == 'string' and '["'..k..'"]' or '['..k..']'
            s = s .. spacing .. "  " .. key .. " = " .. tprint(v, indent + 1) .. ",\n"
        end
        return s .. spacing .. '}'
    elseif type(o) == 'string' then
        return '"' .. o .. '"'
    else
        return tostring(o)
    end
end

function assert_eq(found, expected, key)
    assert(type(found) == type(expected), "Types not matching " .. ((key ~= nil) and ('(key: ' .. key .. ')') or ''))
    if type(found) == 'table' then
        assert(#found == #expected, "Tables are of different sizes " .. ((key ~= nil) and ('(key: ' .. key .. ')') or ''))
        for k,v in pairs(found) do
            assert_eq(v, expected[k], k)
        end
        for k,v in pairs(expected) do
            assert_eq(v, found[k], k)
        end
    else
        assert(found == expected, 'Assertion failed, expected "' .. tprint(expected) .. '", found "' .. tprint(found) .. '".')
    end
end

do
    local source = [[
.const
    0: 3.14
    1: "Hello world"

.func 0
    pushi   2   ; this is a comment
    pushi   3
    sum
    ret
.func 1
    pushi   5000
    ret ]]

    local expected = {
        constants = { [0] = 3.14, [1] = "Hello world" },
        functions = {
            [0] = {
                { "pushi", 2 },
                { "pushi", 3 },
                { "sum" },
                { "ret" },
            },
            [1] = {
                { "pushi", 5000 },
                { "ret" },
            }
        }
    }

    local found = assemble(source)
    -- tprint(expected)
    tprint(found)
    assert_eq(found, expected)
end

print('End.')
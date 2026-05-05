local pprint = require('pprint')
local assemble = require('tyche-as')
local VM = require('tyche-vm')

----------------------
--                  --
--     SUPPORT      --
--                  --
----------------------


function assert_eq(found, expected, key)
    assert(type(found) == type(expected), 'Types not matching , expected "' .. pprint.pformat(expected) .. '", found "' .. pprint.pformat(found) .. '".' .. ((key ~= nil) and ('(key: ' .. key .. ')') or ''))
    if type(found) == 'table' then
        assert(#found == #expected, "Tables are of different sizes " .. ((key ~= nil) and ('(key: ' .. key .. ')') or ''))
        for k,v in pairs(found) do
            assert_eq(v, expected[k], k)
        end
        for k,v in pairs(expected) do
            assert_eq(v, found[k], k)
        end
    else
        assert(found == expected, 'Assertion failed, expected "' .. pprint.pformat(expected) .. '", found "' .. pprint.pformat(found) .. '".')
    end
end

----------------------
--                  --
--      PARSER      --
--                  --
----------------------

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
    -- pprint(expected)
    -- pprint(found)
    assert_eq(found, expected)
end

----------------------
--                  --
--      STACK       --
--                  --
----------------------

do
    local stack = VM.new().stack
    stack:push({ type='integer', value=10 })
    stack:push({ type='integer', value=20 })
    stack:push({ type='integer', value=30 })

    assert_eq(#stack, 3)
    assert_eq(stack[0].value, 10)
    assert_eq(stack[1].value, 20)
    assert_eq(stack[-1].value, 30)
    assert_eq(stack[-2].value, 20)

    stack:pop()
    stack:pop()
    assert_eq(stack[-1].value, 10)
    stack:pop()
    assert_eq(#stack, 0)
end

do
    local stack = VM.new().stack
    stack:push({ type='integer', value=10 })
    stack:push({ type='integer', value=20 })
    stack:push_fp()
    stack:push({ type='integer', value=30 })
    stack:push({ type='integer', value=40 })
    stack:push({ type='integer', value=50 })

    assert_eq(#stack, 3)
    assert_eq(stack[0].value, 30)
    assert_eq(stack[1].value, 40)
    assert_eq(stack[-1].value, 50)
    assert_eq(stack[-2].value, 40)

    stack:pop_fp()

    assert_eq(#stack, 2)
    assert_eq(stack[0].value, 10)
    assert_eq(stack[1].value, 20)
    assert_eq(stack[-1].value, 20)
    assert_eq(stack[-2].value, 10)
end

----------------------
--                  --
--     VM ARITH     --
--                  --
----------------------

local function arith(a, b, op)
    return VM:new():load(assemble(string.format([[
        .func 0
            pushi   %d
            pushi   %d
            %s
            ret
    ]], a, b, op))):call(0)
end


do
    local vm = VM:new()
    -- vm.debug = true
    local bytecode = assemble [[
        .func 0
            pushi   2
            pushi   3
            sum
            ret
    ]]
    vm:load(bytecode)

    assert_eq(vm:stack_sz(), 1)
    assert_eq(vm:is(-1, 'function'), true)

    vm:call(0)

    assert_eq(vm:stack_sz(), 1)
    assert_eq(vm:is(-1, 'integer'), true)
    assert_eq(vm:to_integer(-1), 5)
end

do
    assert_eq(arith(2, 5, 'sum'):to_integer(-1), 7)
    assert_eq(arith(2, 5, 'sub'):to_integer(-1), -3)
end

print('End.')
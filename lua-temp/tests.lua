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
    stack:push(10)
    stack:push(20)
    stack:push(30)

    assert_eq(#stack, 3)
    assert_eq(stack[0], 10)
    assert_eq(stack[1], 20)
    assert_eq(stack[-1], 30)
    assert_eq(stack[-2], 20)

    stack:pop()
    stack:pop()
    stack:pop()
    assert_eq(#stack, 0)
end

do
    local stack = VM.new().stack
    stack:push(10)
    stack:push(20)
    stack:push_fp()
    stack:push(30)
    stack:push(40)
    stack:push(50)

    assert_eq(#stack, 3)
    assert_eq(stack[0], 30)
    assert_eq(stack[1], 40)
    assert_eq(stack[-1], 50)
    assert_eq(stack[-2], 40)

    stack:pop_fp()

    assert_eq(#stack, 2)
    assert_eq(stack[0], 10)
    assert_eq(stack[1], 20)
    assert_eq(stack[-1], 20)
    assert_eq(stack[-2], 10)
end

----------------------
--                  --
--     VM STACK     --
--                  --
----------------------

do
    local vm = VM:new()
    vm.debug = true
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
    assert_eq(vm:is(-1, 'integer'))
    assert_eq(vm:to_integer(-1), 5)
end

print('End.')
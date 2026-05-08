local pprint = require('pprint')
local assemble = require('tyche-as')
local VM = require('tyche-vm')

----------------------
--                  --
--     SUPPORT      --
--                  --
----------------------

function TEST(name)
    print("### " .. name)
end

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

do TEST "Parser"

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

do TEST "Parser: labels"

    local source = [[
        .func 0
            jmp     @my_label
            pushi   3
        @my_label:
            ret ]]

    local expected = {
        constants = {},
        functions = {
            [0] = {
                { "jmp", "@my_label" },
                { "pushi", 3 },
                { "ret", labels = { "@my_label" } },
            }
        }
    }

    local found = assemble(source)
    pprint(found)
    assert_eq(found, expected)
end

----------------------
--                  --
--      STACK       --
--                  --
----------------------

do TEST "Stack"
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

do TEST "Stack with frame pointer"
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
    return VM.new():load(assemble(string.format([[
        .func 0
            pushi   %d
            pushi   %d
            %s
            ret
    ]], a, b, op))):call(0)
end


do TEST "VM: basic"
    local vm = VM.new()
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

do TEST "VM: logic/arithmetic"
    assert_eq(arith(2, 5, 'sum'):to_integer(-1), 7)
    assert_eq(arith(2, 5, 'sub'):to_integer(-1), -3)
    assert_eq(arith(2, 5, 'mul'):to_integer(-1), 10)
    assert_eq(arith(20, 3, 'idiv'):to_integer(-1), 6)
    assert_eq(arith(5, 5, 'eq'):to_integer(-1), 1)
    assert_eq(arith(5, 5, 'neq'):to_integer(-1), 0)
    assert_eq(arith(4, 5, 'lt'):to_integer(-1), 1)
    assert_eq(arith(5, 5, 'lt'):to_integer(-1), 0)
    assert_eq(arith(4, 5, 'lte'):to_integer(-1), 1)
    assert_eq(arith(5, 5, 'lte'):to_integer(-1), 1)
    assert_eq(arith(5, 5, 'gt'):to_integer(-1), 0)
    assert_eq(arith(5, 5, 'gte'):to_integer(-1), 1)
    assert_eq(arith(20, 5, 'and'):to_integer(-1), 4)
    assert_eq(arith(20, 5, 'or'):to_integer(-1), 21)
    assert_eq(arith(20, 5, 'xor'):to_integer(-1), 17)
    assert_eq(arith(2, 5, 'pow'):to_integer(-1), 32)
    assert_eq(arith(2, 5, 'shl'):to_integer(-1), 64)
    assert_eq(arith(20, 3, 'shr'):to_integer(-1), 2)
    assert_eq(arith(20, 3, 'mod'):to_integer(-1), 2)
end

do TEST "VM: local variables"
    local vm = VM.new():load(assemble([[
        .func 0
            pushv 2         ; local a, b
            pushi 3         ; a = 3
            set   0
            pushi 4         ; b = 4
            set   1
            dupv  0         ; return a
            ret
    ]])):call(0)

    assert_eq(vm:stack_sz(), 1)
    assert_eq(vm:to_integer(-1), 3)
end

do TEST "VM: functions"
    local vm = VM.new():load(assemble([[
        .func 0
            pushf   1
            pushi   2
            pushi   3
            call    2
            ret
        .func 1
            dupv    0
            dupv    1
            sub
            ret
    ]])):call(0)

    assert_eq(vm:stack_sz(), 1)
    assert_eq(vm:to_integer(-1), -1)
end

do
    TEST "VM: jumps (jmp + bnz)"
    local vm = VM.new():load(assemble [[
        .func 0
            jmp     @x1
            pushi   5
        @x1:
            pushi   1
            bnz     @x2
            bz      @x3
        @x2:
            pushi   6
            ret
        @x3:
            pushi   7
            ret
    ]]):call(0)

    assert_eq(vm:to_integer(-1), 6)
end

do
    TEST "VM: jumps (bz)"
    pprint(assemble [[
                   .func 0
                       jmp     @x1
                       pushi   5
                   @x1:
                       pushi   0
                       bnz     @x2
                       pushi   0
                       bz      @x3
                   @x2:
                       pushi   6
                       ret
                   @x3:
                       pushi   7
                       ret
               ]])
    local vm = VM.new():set_debug(true):load(assemble [[
        .func 0
            jmp     @x1
            pushi   5
        @x1:
            pushi   0
            bnz     @x2
            bz      @x3
        @x2:
            pushi   6
            ret
        @x3:
            pushi   7
            ret
    ]]):call(0)

    assert_eq(vm:to_integer(-1), 7)
end


print('End.')
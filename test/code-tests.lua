return {
    {
        name = "VM: basic",
        code = [[
            .func 0
                pushi   2
                pushi   3
                sum
                ret
        ]],
        expected_stack_size = 1,
        expected_stack_top = 5,
    },
    {
        name = "VM: real values",
        code = [[
            .const
                0: 3.14
            .func 0
                pushc   0
                ret
        ]],
        expected_heap_size = 0,
        expected_stack_top = 3.14,
    },
    {
        name = "VM: integer expressions",
        template = [[
            .func 0
                pushi   %d
                pushi   %d
                %s
                ret
        ]],
        scenarios = {
            { parameters = {  2, 5, 'sum' },  name = "Sum", expected_stack_top = 7 },
            { parameters = {  2, 5, 'sub' },  name = "Subtraction", expected_stack_top = -3 },
            { parameters = {  2, 5, 'mul' },  name = "Multiplication", expected_stack_top = 10 },
            { parameters = { 20, 3, 'idiv' }, name = "Integer division", expected_stack_top = 6 },
            { parameters = {  5, 5, 'eq' },   name = "Equality", expected_stack_top = true },
            { parameters = {  5, 5, 'neq' },  name = "Inequality", expected_stack_top = false },
            { parameters = {  4, 5, 'lt' },   name = "Less than", expected_stack_top = true },
            { parameters = {  5, 5, 'lt' },   name = "Less than", expected_stack_top = false },
            { parameters = {  4, 5, 'lte' },  name = "Less than or equal", expected_stack_top = true },
            { parameters = {  5, 5, 'lte' },  name = "Less than or equal", expected_stack_top = true },
            { parameters = {  5, 5, 'gt' },   name = "Greater than", expected_stack_top = false },
            { parameters = {  5, 5, 'gte' },  name = "Greater than or equal", expected_stack_top = true },
            { parameters = { 20, 5, 'and' },  name = "Logical AND", expected_stack_top = 4 },
            { parameters = { 20, 5, 'or' },   name = "Logical OR", expected_stack_top = 21 },
            { parameters = { 20, 5, 'xor' },  name = "Logical XOR", expected_stack_top = 17 },
            { parameters = {  2, 5, 'pow' },  name = "Power", expected_stack_top = 32 },
            { parameters = {  2, 5, 'shl' },  name = "Shift left", expected_stack_top = 64 },
            { parameters = { 20, 3, 'shr' },  name = "Shift right", expected_stack_top = 2},
            { parameters = { 20, 3, 'mod' },  name = "Modulo", expected_stack_top = 2 },
        },
    },
    {
        name = "VM: local variables",
        code = [[
            .func 0
                pushv 2         ; local a, b
                pushi 3         ; a = 3
                set   0
                pushi 4         ; b = 4
                set   1
                dupv  0         ; return a
                ret
        ]],
        expected_stack_size = 1,
        expected_stack_top = 3,
    },
    {
        name = "VM: functions",
        code = [[
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
        ]],
        expected_stack_size = 1,
        expected_stack_top = -1,
    },
    {
        name = "VM: jumps (jmp + bnz)",
        code = [[
            .func 0
                jmp     @x1         ; 0
                pushi   5           ; 3
            @x1:
                pushi   1           ; 5
                bnz     @x2         ; 7
                pushi   1           ; 10
                bz      @x3         ; 12
            @x2:
                pushi   6           ; 15
                ret                 ; 17
            @x3:
                pushi   7           ; 18
                ret                 ; 20
        ]],
        --debug_bytecode = true,
        --decompile = true,
        --debug = true,
        expected_stack_top = 6,
    },
    {
        name = "VM: jumps (bz)",
        code = [[
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
        ]],
        expected_stack_top = 7,
    },
    {
        name = "VM: arrays",
        code = [[
            .func 0
                newa
                pushi   10
                seti    0
                pushi   20
                seti    1
                pushi   30
                seti    2
                geti    1
                ret
        ]],
        expected_stack_top = 20,
        expected_heap_size = 2,
    },
    {
        name = "VM: arrays GC",
        code = [[
            .func 0
                pushn
                newa
                pushi   10
                seti    0
                pop
                gc
                ret
        ]],
        expected_heap_size = 1,
    },
    {
        name = "VM: string from const",
        code = [[
            .const
                0: "Hello"
            .func 0
                pushc   0
                ret
        ]],
        expected_stack_top = "Hello"
    },
    {
        name = "VM: concatenate strings (GC won't delete)",
        code = [[
            .const
                0: "Hello "
                1: "world"
            .func 0
                pushc   0
                pushc   1
                sum
                gc
                ret
        ]],
        expected_stack_top = "Hello world",
        expected_heap_size = 4,
    },
    {
        name = "VM: GC strings (GC will delete)",
        code = [[
            .const
                0: "Hello "
                1: "world"
            .func 0
                pushi   50
                pushc   0
                pushc   1
                sum
                pop
                gc
                ret
        ]],
        expected_stack_top = 50,
        expected_heap_size = 3,
    },
    {
        name = "VM: array GC items (1st level) - no items removed",
        code = [[
            .const
                0: "Hello "
                1: "world"
            .func 0
                pushn
                newa
                pushc   0
                pushc   1
                sum
                seti    0
                gc
                pop
                ret
        ]],
        expected_heap_size = 5,
    },
    {
        name = "VM: array GC items (1st level) - all items removed",
        code = [[
            .const
                0: "Hello "
                1: "world"
            .func 0
                pushn
                newa
                pushc   0
                pushc   1
                sum
                seti    0
                pop
                gc
                ret
        ]],
        expected_heap_size = 3,
    },
    {
        name = "VM: array GC items (2nd level) - no items removed",
        code = [[
            .const
                0: "Hello "
                1: "world"
            .func 0
                pushn
                newa
                newa
                pushc   0
                pushc   1
                sum
                seti    0
                seti    0
                gc
                pop
                ret
        ]],
        expected_heap_size = 6,
    },
    {
        name = "VM: array GC items (2nd level) - all items removed",
        code = [[
            .const
                0: "Hello "
                1: "world"
            .func 0
                pushn
                newa
                newa
                pushc   0
                pushc   1
                sum
                seti    0
                seti    0
                pop
                gc
                ret
        ]],
        expected_heap_size = 3,
    },
    {
        name = "VM: iterate over arrays",
        code = [[
            .func 0
                pushi   0           ; sum = 0

                newa                ; create array: [10, 20, 30]
                pushi   10
                appnd
                pushi   20
                appnd
                pushi   30
                appnd

                pushn
            @startofloop:
                next                ; for i, v in array
                bnil    @endofloop

                dupv    0           ; push 'sum'
                sum
                set     0           ; sum += v, only key is left
                jmp     @startofloop

            @endofloop:
                pop                 ; remove last nil
                dupv    0
                ret
        ]],
        expected_stack_top = 60,
    },
    {
        name = "VM: tables",
        code = [[
            .const
                0: "k1"
                1: "k2"
            .func 0
                newt                ; table = { "k1": 10, "k2": 20 }
                pushc   0
                pushi   10
                setkv
                pushc   1
                pushi   20
                setkv

                pushc   1
                getkv
                ret
        ]],
        expected_stack_top = 20,
    },
    {
        name = "VM: iterate over tables",
        code = [[
            .const
                0: "k1"
                1: "k2"
            .func 0
                pushi   0           ; sum = 0

                newt                ; table = { "k1": 10, "k2": 20 }
                pushc   0
                pushi   10
                setkv
                pushc   1
                pushi   20
                setkv

                pushn
            @startofloop:
                next                ; for i, v in array
                bnil    @endofloop

                dupv    0           ; push 'sum'
                sum
                set     0           ; sum += v, only key is left
                jmp     @startofloop

            @endofloop:
                pop                 ; remove last nil
                dupv    0
                ret
        ]],
        expected_stack_top = 30,
    },
    {
        name = "Globals",
        code = [[
            .const
                0: "Hello"
            .func 0
                glbl
                pushc   0
                pushi   20
                setkv
                pop
                glbl
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = 20,
    },
    {
        name = "Supertables",
        code = [[
            .const
                0: "f"
            .func 0
                newt        ; child table
                newt        ; super table
                pushc   0   ;   "f"
                pushf   1   ;   function 1
                setkv       ; super = { "f": func1 }
                sptb        ; child <== super
                pushc   0   ;   "f"
                getkv       ; child.f
                call    0   ; child.f()
                ret
            .func 1
                pushi   30
                ret
        ]],
        expected_stack_top = 30,
    }
}
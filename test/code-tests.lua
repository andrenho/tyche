return {
    {
        name = "VM: basic",
        code = [[
            .assembly
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
            .assembly
            .const
                0: 3.14
            .func 0
                pushc   0
                ret
        ]],
        expected_heap_size = 1, -- only the global table
        expected_stack_top = 3.14,
    },
    {
        name = "VM: integer expressions",
        template = [[
            .assembly
            .func 0
                pushi   %d
                pushi   %d
                %s
                ret
        ]],
        scenarios = {
            { parameters = {  2, 5, 'sum' },  name = "Sum", expected_stack_top = 7 },
            { parameters = {  2, 5, 'sub' },  name = "Subtraction", expected_stack_top = -3 },
            { parameters = {  2, -5, 'mul' }, name = "Multiplication", expected_stack_top = -10 },
            { parameters = { 20, 3, 'idiv' }, name = "Integer division", expected_stack_top = 6 },
            { parameters = { 21, 4, 'div' },  name = "Division", expected_stack_top = 5.25 },
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
        name = "VM: integer unary expressions",
        template = [[
            .assembly
            .func 0
                pushi   %d
                %s
                ret
        ]],
        scenarios = {
            { parameters = { 23, 'not' },  name = "Not", expected_stack_top = -24 },
            { parameters = { 23, 'neg' },  name = "Neg (1)", expected_stack_top = -23 },
            { parameters = { -23, 'neg' }, name = "Neg (2)", expected_stack_top = 23 },
        },
    },
    {
        name = "VM: real unary expressions",
        template = [[
            .assembly
            .const
                0: %f
            .func 0
                pushc   0
                %s
                ret
        ]],
        scenarios = {
            { parameters = { 23.3, 'neg' },  name = "Neg (1)", expected_stack_top = -23.3 },
            { parameters = { -23.3, 'neg' }, name = "Neg (2)", expected_stack_top = 23.3 },
        },
    },
    {
        name = "VM: real expressions",
        template = [[
            .assembly
            .const
                0: %f
                1: %f
            .func 0
                pushc   0
                pushc   1
                %s
                ret
        ]],
        scenarios = {
            { parameters = {  2.3, 5.3, 'sum' },  name = "Sum", expected_stack_top = 7.6 },
            { parameters = {  2.3, 5.6, 'sub' },  name = "Subtraction", expected_stack_top = -3.3 },
            { parameters = {  2.3, 5.3, 'mul' },  name = "Multiplication", expected_stack_top = 12.19 },
            { parameters = { 20.3, 3.3, 'idiv' }, name = "Integer division", expected_stack_top = 6 },
            { parameters = { 20.4, 2.5, 'div' },  name = "Integer division", expected_stack_top = 8.16 },
            { parameters = {  5.3, 5.3, 'eq' },   name = "Equality", expected_stack_top = true },
            { parameters = {  5.3, 5.3, 'neq' },  name = "Inequality", expected_stack_top = false },
            { parameters = {  4.3, 5.3, 'lt' },   name = "Less than", expected_stack_top = true },
            { parameters = {  5.3, 5.3, 'lt' },   name = "Less than", expected_stack_top = false },
            { parameters = {  4.3, 5.3, 'lte' },  name = "Less than or equal", expected_stack_top = true },
            { parameters = {  5.3, 5.3, 'lte' },  name = "Less than or equal", expected_stack_top = true },
            { parameters = {  5.3, 5.3, 'gt' },   name = "Greater than", expected_stack_top = false },
            { parameters = {  5.3, 5.3, 'gte' },  name = "Greater than or equal", expected_stack_top = true },
            { parameters = {  2.3, 5.3, 'pow' },  name = "Power", expected_stack_top = 82.6337631255 },
        },
    },
    {
        name = "VM: mix integer and real (1)",
        code = [[
            .assembly
            .const
                0: 3.14
            .func 0
                pushc   0
                pushi   3
                sum
                ret
        ]],
        expected_stack_top = 6.14,
    },
    {
        name = "VM: mix integer and real (2)",
        code = [[
            .assembly
            .const
                0: 3.14
            .func 0
                pushi   3
                pushc   0
                sum
                ret
        ]],
        expected_stack_top = 6.14,
    },
    {
        name = "VM: boolean expressions",
        template = [[
            .assembly
            .func 0
                push%s
                push%s
                %s
                ret
        ]],
        scenarios = {
            { parameters = { 't', 'z', 'and' }, name = "And", expected_stack_top = false },
            { parameters = { 't', 'z', 'or' },  name = "Or", expected_stack_top = true },
            { parameters = { 't', 'z', 'xor' }, name = "Xor", expected_stack_top = true },
            { parameters = { 't', 'z', 'eq' },  name = "Equality", expected_stack_top = false },
            { parameters = { 't', 'z', 'neq' }, name = "Inequality", expected_stack_top = true },
        },
    },
    {
        name = "VM: unary boolean expressions",
        template = [[
            .assembly
            .func 0
                push%s
                %s
                ret
        ]],
        scenarios = {
            { parameters = { 't', 'not' },  name = "Not (1)", expected_stack_top = false },
            { parameters = { 'z', 'not' },  name = "Not (2)", expected_stack_top = true },
        },
    },
    {
        name = "VM: local variables",
        code = [[
            .assembly
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
            .assembly
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
            .assembly
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
        -- debug_bytecode = true,
        -- decompile = true,
        --debug = true,
        expected_stack_top = 6,
    },
    {
        name = "VM: jumps (bz)",
        code = [[
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
            .assembly
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
    },
    {
        name = "VM: len (string)",
        code = [[
            .assembly
            .const
                0: "Hello"
            .func 0
                pushc   0
                len
                ret
        ]],
        expected_stack_top = 5,
    },
    {
        name = "VM: len (array)",
        code = [[
            .assembly
            .func 0
                newa
                pushi   10
                seti    0
                pushi   20
                seti    1
                len
                ret
        ]],
        expected_stack_top = 2,
    },
    {
        name = "VM: len (table)",
        code = [[
            .assembly
            .func 0
                newt
                pushi   10
                pushi   20
                setkv
                pushi   20
                pushi   30
                setkv
                len
                ret
        ]],
        expected_stack_top = 2,
    },
    {
        name = "VM: type",
        code = [[
            .assembly
            .func 0
                newt
                type
                ret
        ]],
        expected_stack_top = 6,
    },
    {
        name = "VM: error handling within the same function",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                pushi   10
                thrw
                pushi   20
                ret
            @catch:
                pushi   30
                ret
        ]],
        expected_stack_top = 30,
    },
    {
        name = "VM: error handling within the same function, exception value",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                pushi   10
                thrw
                pushi   20
                ret
            @catch:
                ret
        ]],
        expected_stack_top = 10,
    },
    {
        name = "VM: error handling across functions",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                pushf   1
                call    0
                pushi   20
                ret
            @catch:
                ret

            .func 1
                pushi   10
                thrw
                pushi   40
                ret
        ]],
        expected_stack_top = 10,
    },
    {
        name = "VM: multilevel error handling (1)",
        code = [[
            .assembly
            .func 0
                pushe   @catch1
                pushf   1
                call    0
                pushi   20
                ret
            @catch1:
                ret

            .func 1
                pushe   @catch2
                pushi   30
                thrw
                ret
            @catch2:
                ret
        ]],
        expected_stack_top = 20,
    },
    {
        name = "VM: multilevel error handling (2)",
        code = [[
            .assembly
            .func 0
                pushe   @catch1
                pushf   1
                call    0
                pushi   20
                thrw
                pushi   40
                ret
            @catch1:
                ret

            .func 1
                pushe   @catch2
                pushi   30
                jmp     @skip_catch2
            @catch2:
                ret
            @skip_catch2:
                pope
                ret
        ]],
        expected_stack_top = 20,
    },
    -- uncomment to test toplevel error - this will exit the program
    --{
    --    name = "VM: toplevel error",
    --    code = [[
    --        .assembly
    --        .const
    --            0: "error"
    --            1: "Error message"
    --        .func 0
    --            newt
    --            pushc   0
    --            pushc   1
    --            setkv
    --            thrw
    --            ret
    --    ]],
    --    debug = true,
    --    expected_stack_top = 20,
    --},
    {
        name = "VM: error handling on expression",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                newt
                newt
                idiv
                pushi   20
                ret
            @catch:
                pushi   30
                ret
        ]],
        -- debug = true,
        expected_stack_top = 30,
    },
}
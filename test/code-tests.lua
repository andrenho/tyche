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
            { parameters = {  5, 5, 'eq' },   name = "Equality", expected_stack_top = 1 },
            { parameters = {  5, 5, 'neq' },  name = "Inequality", expected_stack_top = 0 },
            { parameters = {  4, 5, 'lt' },   name = "Less than", expected_stack_top = 1 },
            { parameters = {  5, 5, 'lt' },   name = "Less than", expected_stack_top = 0 },
            { parameters = {  4, 5, 'lte' },  name = "Less than or equal", expected_stack_top = 1 },
            { parameters = {  5, 5, 'lte' },  name = "Less than or equal", expected_stack_top = 1 },
            { parameters = {  5, 5, 'gt' },   name = "Greater than", expected_stack_top = 0 },
            { parameters = {  5, 5, 'gte' },  name = "Greater than or equal", expected_stack_top = 1 },
            { parameters = { 20, 5, 'and' },  name = "Logical AND", expected_stack_top = 4 },
            { parameters = { 20, 5, 'or' },   name = "Logical OR", expected_stack_top = 21 },
            { parameters = { 20, 5, 'xor' },  name = "Logical XOR", expected_stack_top = 17 },
            { parameters = {  2, 5, 'pow' },  name = "Power", expected_stack_top = 32 },
            { parameters = {  2, 5, 'shl' },  name = "Shift left", expected_stack_top = 64 },
            { parameters = { 20, 3, 'shr' },  name = "Shift right", expected_stack_top = 2},
            { parameters = { 20, 3, 'mod' },  name = "Modulo", expected_stack_top = 2 },
        },
    }
}
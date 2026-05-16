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
        replacements = {
            { 2, 3, 'sum', expected_stack_top = 5 },
        },
    }
}
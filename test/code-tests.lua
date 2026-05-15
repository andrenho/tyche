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
}
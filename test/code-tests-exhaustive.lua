-- =============================================================================
-- Tyche VM — extended test suite (edge cases, limits, errors, UB probes)
-- Same format as the existing suite. Drop-in additional file.
--
-- Conventions / caveats discovered while writing these:
--
--  * Branching (`bz`/`bnz`) tests falsiness via value_is_false(): ONLY boolean
--    `false` and `nil` are falsy. `bz`/`bnz` are valid ONLY on a boolean or nil
--    operand; branching on an integer/real/string/array/table/function is a
--    runtime error. (Integers — including 0 — are NOT branchable.)
--
--  * Comparison ops (eq/lt/...) yield BOOLEANS, so they are the intended drivers
--    for bz/bnz. Loops/recursion branch on comparison results, NOT on raw
--    integer arithmetic.
--
--  * `bnil` is a dedicated nil-test and applies to ANY value type (it just asks
--    "is this nil"), independent of the bz/bnz bool-or-nil restriction.
--
--  * `pushz` pushes boolean FALSE (not integer 0), matching the implementation.
--    Use `pushi 0` for integer zero.
--
--  * Tests that would CRASH (div-by-zero SIGFPE, OOB writes, abort()) or are
--    genuine C UB with no defined answer are COMMENTED OUT with an explanation,
--    so the runner doesn't die. Enable them once behavior is specified.
--
--  * heap_size assertions are kept minimal — exact counts depend on string
--    interning behavior and are brittle. Where present they mirror the style
--    of the original suite; verify against your heap implementation.
-- =============================================================================

return {

    -- =========================================================================
    -- 1. INTEGER BOUNDARIES & OPERAND SIZE ENCODING
    --    Exercises int8/int16/int32 operand decoding and sign-extension in
    --    code_next_instruction (the (int8_t)/(int16_t)/(int32_t) casts).
    -- =========================================================================
    {
        name = "INT: operand sign-extension boundaries",
        template = [[
            .assembly
            .func 0
                pushi   %d
                ret
        ]],
        scenarios = {
            { parameters = {          0 }, name = "zero",          expected_stack_top = 0 },
            { parameters = {        127 }, name = "int8 max",      expected_stack_top = 127 },
            { parameters = {       -128 }, name = "int8 min",      expected_stack_top = -128 },
            { parameters = {        128 }, name = "int8 max+1",    expected_stack_top = 128 },
            { parameters = {       -129 }, name = "int8 min-1",    expected_stack_top = -129 },
            { parameters = {      32767 }, name = "int16 max",     expected_stack_top = 32767 },
            { parameters = {     -32768 }, name = "int16 min",     expected_stack_top = -32768 },
            { parameters = {      32768 }, name = "int16 max+1",   expected_stack_top = 32768 },
            { parameters = {     -32769 }, name = "int16 min-1",   expected_stack_top = -32769 },
            { parameters = { 2147483647 }, name = "int32 max",     expected_stack_top = 2147483647 },
            { parameters = {-2147483648 }, name = "int32 min",     expected_stack_top = -2147483648 },
            { parameters = {         -1 }, name = "negative one",  expected_stack_top = -1 },
            { parameters = {        255 }, name = "0xFF as int16", expected_stack_top = 255 },
        },
    },
    {
        name = "INT: not at boundaries",
        template = [[
            .assembly
            .func 0
                pushi   %d
                not
                ret
        ]],
        scenarios = {
            { parameters = {          0 }, name = "~0",         expected_stack_top = -1 },
            { parameters = {         -1 }, name = "~-1",        expected_stack_top = 0 },
            { parameters = { 2147483647 }, name = "~INT32_MAX", expected_stack_top = -2147483648 },
            { parameters = {-2147483648 }, name = "~INT32_MIN", expected_stack_top = 2147483647 },
        },
    },

    -- ---- UB / undefined-result integer cases (COMMENTED: no defined answer) --
    --
    -- Signed overflow is UB in C. Decide the contract (wrap / saturate / throw)
    -- then enable with the matching expected value.
    --
    -- { name = "INT: INT32_MAX + 1 (signed overflow, UB)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 2147483647
    --         pushi 1
    --         sum
    --         ret ]],
    --   expected_stack_top = -2147483648 },  -- only if wrapping is the chosen contract
    --
    -- { name = "INT: neg INT32_MIN (UB, not representable)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi -2147483648
    --         neg
    --         ret ]],
    --   expected_stack_top = -2147483648 },  -- wrap; otherwise must be defined

    -- =========================================================================
    -- 2. REAL VALUES & SPECIAL CASES
    -- =========================================================================
    {
        name = "REAL: division always yields real (even when exact)",
        code = [[
            .assembly
            .func 0
                pushi   6
                pushi   2
                div
                type
                ret
        ]],
        expected_stack_top = 3,   -- 3 == TYC_REAL
    },
    {
        name = "REAL: idiv yields integer even from real operands",
        template = [[
            .assembly
            .const
                0: %f
                1: %f
            .func 0
                pushc   0
                pushc   1
                idiv
                type
                ret
        ]],
        scenarios = {
            { parameters = { 7.9, 2.0 }, name = "real idiv -> int", expected_stack_top = 2 },
        },
    },
    {
        name = "REAL: int + real promotes to real",
        code = [[
            .assembly
            .const
                0: 2.5
            .func 0
                pushi   3
                pushc   0
                sum
                type
                ret
        ]],
        expected_stack_top = 3,   -- real
    },
    {
        name = "REAL: negative zero compares equal to zero",
        template = [[
            .assembly
            .const
                0: %f
                1: %f
            .func 0
                pushc   0
                pushc   1
                eq
                ret
        ]],
        scenarios = {
            { parameters = { 0.0, -0.0 }, name = "0.0 == -0.0", expected_stack_top = true },
        },
    },
    {
        name = "REAL: large magnitude round-trip through const",
        template = [[
            .assembly
            .const
                0: %f
            .func 0
                pushc   0
                ret
        ]],
        scenarios = {
            { parameters = { 1e308 },   name = "very large",  expected_stack_top = 1e308 },
            { parameters = { 1e-308 },  name = "very small",  expected_stack_top = 1e-308 },
            { parameters = { 123456789.987654 }, name = "precision", expected_stack_top = 123456789.987654 },
        },
    },

    -- =========================================================================
    -- 3. DIVISION / MODULO SIGN & ROUNDING SEMANTICS
    --    These assert C-style truncation/sign. If the VM intends Lua-style
    --    floor semantics, the expected values differ — verify and adjust.
    -- =========================================================================
    {
        name = "DIV: integer division rounding/sign (assumes C truncation)",
        template = [[
            .assembly
            .func 0
                pushi   %d
                pushi   %d
                idiv
                ret
        ]],
        scenarios = {
            { parameters = {  7,  2 }, name = "7 idiv 2",   expected_stack_top = 3 },
            { parameters = { -7,  2 }, name = "-7 idiv 2",  expected_stack_top = -3 },  -- C trunc; floor=-4
            { parameters = {  7, -2 }, name = "7 idiv -2",  expected_stack_top = -3 },  -- C trunc; floor=-4
            { parameters = { -7, -2 }, name = "-7 idiv -2", expected_stack_top = 3 },
        },
    },
    {
        name = "MOD: modulo sign (assumes C %, sign follows dividend)",
        template = [[
            .assembly
            .func 0
                pushi   %d
                pushi   %d
                mod
                ret
        ]],
        scenarios = {
            { parameters = {  7,  3 }, name = "7 mod 3",   expected_stack_top = 1 },
            { parameters = { -7,  3 }, name = "-7 mod 3",  expected_stack_top = -1 },  -- C; floor-mod=2
            { parameters = {  7, -3 }, name = "7 mod -3",  expected_stack_top = 1 },   -- C; floor-mod=-2
            { parameters = { -7, -3 }, name = "-7 mod -3", expected_stack_top = -1 },
        },
    },
    {
        name = "POW: integer base/exponent edge cases",
        template = [[
            .assembly
            .func 0
                pushi   %d
                pushi   %d
                pow
                ret
        ]],
        scenarios = {
            { parameters = { 2, 0 },  name = "2^0",  expected_stack_top = 1 },
            { parameters = { 0, 0 },  name = "0^0",  expected_stack_top = 1 },   -- C pow(0,0)=1
            { parameters = { 5, 1 },  name = "5^1",  expected_stack_top = 5 },
            { parameters = { 2, 10 }, name = "2^10", expected_stack_top = 1024 },
            -- { parameters = { 2, -1 }, name = "2^-1 (real?)", expected_stack_top = 0.5 },
            --   ^ enable once you decide whether negative exponent yields real 0.5
            --     or integer 0. Verify VM behavior first.
        },
    },

    -- ---- div/mod by zero (COMMENTED: integer div-by-zero is UB / SIGFPE) -----
    --
    -- Intended behavior is almost certainly a catchable runtime error. Once that
    -- is implemented, replace these with the catch-pattern (see section 15).
    --
    -- { name = "DIV: idiv by zero (UB - should be catchable error)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 5
    --         pushi 0
    --         idiv
    --         ret ]] },
    --
    -- { name = "MOD: mod by zero (UB - should be catchable error)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 5
    --         pushi 0
    --         mod
    --         ret ]] },
    --
    -- { name = "DIV: INT32_MIN idiv -1 (overflow UB)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi -2147483648
    --         pushi -1
    --         idiv
    --         ret ]] },
    --
    -- Real division by zero is well-defined (inf), but asserting inf depends on
    -- harness float handling:
    -- { name = "DIV: real div by zero -> inf",
    --   code = [[ .assembly
    --     .const
    --         0: 1.0
    --         1: 0.0
    --     .func 0
    --         pushc 0
    --         pushc 1
    --         div
    --         ret ]],
    --   expected_stack_top = math.huge },

    -- =========================================================================
    -- 4. BITWISE & SHIFT
    -- =========================================================================
    {
        name = "SHIFT: shl/shr within range",
        template = [[
            .assembly
            .func 0
                pushi   %d
                pushi   %d
                %s
                ret
        ]],
        scenarios = {
            { parameters = { 1,  0, 'shl' }, name = "1<<0",   expected_stack_top = 1 },
            { parameters = { 1,  4, 'shl' }, name = "1<<4",   expected_stack_top = 16 },
            { parameters = { 1, 30, 'shl' }, name = "1<<30",  expected_stack_top = 1073741824 },
            { parameters = { 256, 4, 'shr' }, name = "256>>4", expected_stack_top = 16 },
            { parameters = { 1,  3, 'shr' }, name = "1>>3",   expected_stack_top = 0 },
        },
    },
    {
        name = "BITWISE: identities",
        template = [[
            .assembly
            .func 0
                pushi   %d
                pushi   %d
                %s
                ret
        ]],
        scenarios = {
            { parameters = { 12345,     0, 'or'  }, name = "x|0 = x",   expected_stack_top = 12345 },
            { parameters = { 12345,     0, 'and' }, name = "x&0 = 0",   expected_stack_top = 0 },
            { parameters = { 12345, 12345, 'xor' }, name = "x^x = 0",   expected_stack_top = 0 },
            { parameters = { 12345,    -1, 'and' }, name = "x&-1 = x",  expected_stack_top = 12345 },
        },
    },

    -- ---- shift UB (COMMENTED: shift >= width / negative shift is UB) ---------
    --
    -- { name = "SHIFT: shl by 32 (UB - shift count >= width)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 1
    --         pushi 32
    --         shl
    --         ret ]] },
    --
    -- { name = "SHIFT: shl by negative amount (UB)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 1
    --         pushi -1
    --         shl
    --         ret ]] },
    --
    -- 1<<31 sets the sign bit; result depends on operand width/signedness:
    -- { name = "SHIFT: 1<<31 (sign bit, verify width)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 1
    --         pushi 31
    --         shl
    --         ret ]],
    --   expected_stack_top = -2147483648 },  -- if 32-bit signed wrap

    -- =========================================================================
    -- 5. BRANCHING / FALSINESS  (value_is_false: only false and nil are falsy;
    --    bz/bnz valid only on a boolean or nil operand)
    -- =========================================================================
    {
        name = "BRANCH: bz on boolean false branches",
        code = [[
            .assembly
            .func 0
                pushz
                bz      @taken
                pushi   1
                ret
            @taken:
                pushi   0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "BRANCH: bz on boolean true does not branch",
        code = [[
            .assembly
            .func 0
                pusht
                bz      @taken
                pushi   1
                ret
            @taken:
                pushi   0
                ret
        ]],
        expected_stack_top = 1,
    },
    {
        name = "BRANCH: bnz on boolean true branches",
        code = [[
            .assembly
            .func 0
                pusht
                bnz     @taken
                pushi   1
                ret
            @taken:
                pushi   0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "BRANCH: bnz on boolean false does not branch",
        code = [[
            .assembly
            .func 0
                pushz
                bnz     @taken
                pushi   1
                ret
            @taken:
                pushi   0
                ret
        ]],
        expected_stack_top = 1,
    },
    {
        name = "BRANCH: bz on nil branches (nil is falsy)",
        code = [[
            .assembly
            .func 0
                pushn
                bz      @taken
                pushi   1
                ret
            @taken:
                pushi   0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "BRANCH: bnz on nil does not branch (nil is falsy)",
        code = [[
            .assembly
            .func 0
                pushn
                bnz     @taken
                pushi   1
                ret
            @taken:
                pushi   0
                ret
        ]],
        expected_stack_top = 1,
    },
    {
        name = "BRANCH: bnil on nil branches",
        code = [[
            .assembly
            .func 0
                pushn
                bnil    @isnil
                pushi   1
                ret
            @isnil:
                pushi   0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "BRANCH: bnil on non-nil (false) does not branch",
        code = [[
            .assembly
            .func 0
                pushz                ; false is not nil
                bnil    @isnil
                pushi   1
                ret
            @isnil:
                pushi   0
                ret
        ]],
        expected_stack_top = 1,
    },
    {
        name = "BRANCH: comparison result drives bnz (3 < 5 is true)",
        code = [[
            .assembly
            .func 0
                pushi   3
                pushi   5
                lt
                bnz     @yes
                pushi   0
                ret
            @yes:
                pushi   1
                ret
        ]],
        expected_stack_top = 1,
    },
    {
        name = "BRANCH: comparison result drives bnz (5 < 3 is false)",
        code = [[
            .assembly
            .func 0
                pushi   5
                pushi   3
                lt
                bnz     @yes
                pushi   0
                ret
            @yes:
                pushi   1
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "BRANCH: countdown loop (branch on eq comparison)",
        code = [[
            .assembly
            .func 0
                pushi   0       ; slot0: acc
                pushi   5       ; slot1: counter
            @loop:
                dupv    1       ; counter
                pushi   0
                eq              ; counter == 0 ? -> boolean
                bnz     @done   ; true -> done
                dupv    0       ; acc
                dupv    1       ; counter
                sum
                set     0       ; acc += counter
                dupv    1
                pushi   1
                sub
                set     1       ; counter -= 1
                jmp     @loop
            @done:
                dupv    0
                ret
        ]],
        expected_stack_top = 15,   -- 5+4+3+2+1
    },

    -- ---- bz/bnz on non-bool/non-nil (VERIFY: should be a runtime error) ------
    --
    -- Under value_is_false, only booleans and nil are valid bz/bnz operands.
    -- Branching on an integer/real/string/etc must raise an error. Whether that
    -- error is catchable (routes through tyc_throw) or a bare uncatchable ERROR
    -- determines how to assert it. If catchable, wrap in pushe/@catch like the
    -- section-15 type-error tests; otherwise the harness needs an
    -- "expected tyc_call error" assertion. Left commented pending that decision.
    --
    -- { name = "BRANCH: bz on integer is an error",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 0
    --         bz    @x
    --         ret
    --     @x:
    --         ret ]] },
    --
    -- { name = "BRANCH: bnz on real is an error",
    --   code = [[ .assembly
    --     .const
    --         0: 1.0
    --     .func 0
    --         pushc 0
    --         bnz   @x
    --         ret
    --     @x:
    --         ret ]] },
    --
    -- { name = "BRANCH: bz on string is an error",
    --   code = [[ .assembly
    --     .const
    --         0: "x"
    --     .func 0
    --         pushc 0
    --         bz    @x
    --         ret
    --     @x:
    --         ret ]] },


    -- =========================================================================
    -- 6. TYPE OPCODE COVERAGE  (TYC_* numeric tags)
    -- =========================================================================
    {
        name = "TYPE: nil",     code = [[ .assembly
            .func 0
                pushn
                type
                ret ]], expected_stack_top = 0 },
    {
        name = "TYPE: boolean", code = [[ .assembly
            .func 0
                pusht
                type
                ret ]], expected_stack_top = 1 },
    {
        name = "TYPE: integer", code = [[ .assembly
            .func 0
                pushi 7
                type
                ret ]], expected_stack_top = 2 },
    {
        name = "TYPE: real",    code = [[ .assembly
            .const
                0: 1.5
            .func 0
                pushc 0
                type
                ret ]], expected_stack_top = 3 },
    {
        name = "TYPE: string",  code = [[ .assembly
            .const
                0: "hi"
            .func 0
                pushc 0
                type
                ret ]], expected_stack_top = 4 },
    {
        name = "TYPE: array",   code = [[ .assembly
            .func 0
                newa
                type
                ret ]], expected_stack_top = 5 },
    {
        name = "TYPE: table",   code = [[ .assembly
            .func 0
                newt
                type
                ret ]], expected_stack_top = 6 },
    {
        name = "TYPE: function", code = [[ .assembly
            .func 0
                pushf 0
                type
                ret ]], expected_stack_top = 7 },
    {
        name = "VER: returns a string (type 4)",
        code = [[
            .assembly
            .func 0
                ver
                type
                ret
        ]],
        expected_stack_top = 4,
    },

    -- =========================================================================
    -- 7. STACK OPS  (dup, pop)
    -- =========================================================================
    {
        name = "STACK: dup then multiply (square)",
        code = [[
            .assembly
            .func 0
                pushi   7
                dup
                mul
                ret
        ]],
        expected_stack_top = 49,
    },
    {
        name = "STACK: pop exposes value below",
        code = [[
            .assembly
            .func 0
                pushi   1
                pushi   2
                pop
                ret
        ]],
        expected_stack_top = 1,
    },

    -- =========================================================================
    -- 8. LOCAL VARIABLES
    -- =========================================================================
    {
        name = "LOCALS: init, set, read multiple slots",
        code = [[
            .assembly
            .func 0
                pushv   3
                pushi   11
                set     0
                pushi   22
                set     1
                pushi   33
                set     2
                dupv    1
                ret
        ]],
        expected_stack_top = 22,
    },
    {
        name = "LOCALS: overwrite same slot",
        code = [[
            .assembly
            .func 0
                pushv   1
                pushi   100
                set     0
                pushi   200
                set     0
                dupv    0
                ret
        ]],
        expected_stack_top = 200,
    },
    {
        name = "LOCALS: pushv leaves nils (read before set is nil)",
        code = [[
            .assembly
            .func 0
                pushv   1
                dupv    0
                type
                ret
        ]],
        expected_stack_top = 0,   -- nil
    },

    -- =========================================================================
    -- 9. ARRAYS — bounds, growth, sparse
    -- =========================================================================
    {
        name = "ARRAY: get out of bounds returns nil",
        code = [[
            .assembly
            .func 0
                newa
                pushi   1
                seti    0
                geti    5
                type
                ret
        ]],
        expected_stack_top = 0,   -- nil
    },
    {
        name = "ARRAY: append then geti",
        code = [[
            .assembly
            .func 0
                newa
                pushi   100
                appnd
                pushi   200
                appnd
                geti    1
                ret
        ]],
        expected_stack_top = 200,
    },
    {
        name = "ARRAY: len grows with highest set index",
        code = [[
            .assembly
            .func 0
                newa
                pushi   9
                seti    0
                pushi   9
                seti    1
                pushi   9
                seti    2
                len
                ret
        ]],
        expected_stack_top = 3,
    },
    {
        name = "ARRAY: overwrite element",
        code = [[
            .assembly
            .func 0
                newa
                pushi   1
                seti    0
                pushi   2
                seti    0
                geti    0
                ret
        ]],
        expected_stack_top = 2,
    },

    -- ---- sparse growth (COMMENTED: triggers array_set single-doubling bug) ---
    --
    -- array_set() doubles cap only ONCE per call, so a large gap (cap=1, pos=20)
    -- still leaves pos out of bounds and writes past the buffer -> heap overflow.
    -- Enable after array_set loops/computes the next sufficient capacity.
    --
    -- { name = "ARRAY: sparse set at high index extends length",
    --   code = [[ .assembly
    --     .func 0
    --         newa
    --         pushi 99
    --         seti  20
    --         len
    --         ret ]],
    --   expected_stack_top = 21 },

    -- =========================================================================
    -- 10. TABLES — key types, deletion, overwrite, missing
    -- =========================================================================
    {
        name = "TABLE: integer keys",
        code = [[
            .assembly
            .func 0
                newt
                pushi   1
                pushi   111
                setkv
                pushi   2
                pushi   222
                setkv
                pushi   2
                getkv
                ret
        ]],
        expected_stack_top = 222,
    },
    {
        name = "TABLE: boolean key",
        code = [[
            .assembly
            .func 0
                newt
                pusht
                pushi   1
                setkv
                pushz
                pushi   0
                setkv
                pusht
                getkv
                ret
        ]],
        expected_stack_top = 1,
    },
    {
        name = "TABLE: real key",
        code = [[
            .assembly
            .const
                0: 3.14
            .func 0
                newt
                pushc   0
                pushi   42
                setkv
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = 42,
    },
    {
        name = "TABLE: missing key returns nil",
        code = [[
            .assembly
            .const
                0: "present"
                1: "absent"
            .func 0
                newt
                pushc   0
                pushi   1
                setkv
                pushc   1
                getkv
                type
                ret
        ]],
        expected_stack_top = 0,   -- nil
    },
    {
        name = "TABLE: setting nil value deletes key (len drops)",
        code = [[
            .assembly
            .const
                0: "k"
            .func 0
                newt
                pushc   0
                pushi   5
                setkv
                pushc   0
                pushn
                setkv
                len
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "TABLE: overwrite value for existing key",
        code = [[
            .assembly
            .const
                0: "k"
            .func 0
                newt
                pushc   0
                pushi   1
                setkv
                pushc   0
                pushi   2
                setkv
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = 2,
    },
    {
        name = "TABLE: many distinct integer keys (hash stress)",
        code = [[
            .assembly
            .func 0
                newt
                pushi   1
                pushi   10
                setkv
                pushi   2
                pushi   20
                setkv
                pushi   3
                pushi   30
                setkv
                pushi   100
                pushi   1000
                setkv
                pushi   12345
                pushi   54321
                setkv
                pushi   12345
                getkv
                ret
        ]],
        expected_stack_top = 54321,
    },

    -- =========================================================================
    -- 11. SUPERTABLES — shadowing, removal, multilevel, function lookup
    -- =========================================================================
    {
        name = "SUPER: child shadows super key",
        code = [[
            .assembly
            .const
                0: "k"
            .func 0
                newt                ; child
                newt                ; super
                pushc   0
                pushi   999
                setkv               ; super.k = 999
                sptb                ; child <== super
                pushc   0
                pushi   111
                setkv               ; child.k = 111 (shadows)
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = 111,
    },
    {
        name = "SUPER: inherited key read from super on miss",
        code = [[
            .assembly
            .const
                0: "only_in_super"
            .func 0
                newt                ; child
                newt                ; super
                pushc   0
                pushi   777
                setkv               ; super[k]=777
                sptb                ; child <== super
                pushc   0
                getkv               ; child miss -> super
                ret
        ]],
        expected_stack_top = 777,
    },
    {
        name = "SUPER: removing supertable drops inheritance",
        code = [[
            .assembly
            .const
                0: "k"
            .func 0
                newt                ; child  (slot via stack)
                newt                ; super
                pushc   0
                pushi   500
                setkv
                sptb                ; child <== super
                pushn               ; nil super
                sptb                ; child <== (none)
                pushc   0
                getkv
                type
                ret
        ]],
        expected_stack_top = 0,   -- nil (inheritance removed)
    },
    {
        name = "SUPER: two-level inheritance chain",
        code = [[
            .assembly
            .const
                0: "k"
            .func 0
                ; build grandparent
                newt                ; A (child)
                newt                ; B (mid)
                newt                ; C (grandparent)
                pushc   0
                pushi   333
                setkv               ; C.k = 333   (stack: A B C)
                ; B <== C
                sptb                ; pops C as super of B  (stack: A B)
                ; A <== B
                sptb                ; pops B as super of A  (stack: A)
                pushc   0
                getkv               ; A miss -> B miss -> C
                ret
        ]],
        expected_stack_top = 333,
    },

    -- =========================================================================
    -- 12. FUNCTIONS — recursion, nesting, retn vs ret, parameter order
    -- =========================================================================
    {
        name = "FUNC: recursion (factorial 5, branch on lte comparison)",
        code = [[
            .assembly
            .func 0
                pushf   1
                pushi   5
                call    1
                ret
            .func 1                 ; factorial(n), n at slot 0
                dupv    0
                pushi   1
                lte                 ; n <= 1 ? -> boolean
                bnz     @base       ; true -> base case
                dupv    0           ; n
                pushf   1
                dupv    0
                pushi   1
                sub
                call    1           ; factorial(n-1)
                mul                 ; n * factorial(n-1)
                ret
            @base:
                pushi   1
                ret
        ]],
        expected_stack_top = 120,
    },
    {
        name = "FUNC: three-level nested calls",
        code = [[
            .assembly
            .func 0
                pushf   1
                pushi   10
                call    1
                ret
            .func 1
                pushf   2
                dupv    0
                pushi   1
                sum
                call    1
                ret
            .func 2
                dupv    0
                pushi   2
                mul
                ret
        ]],
        -- (10+1) * 2 = 22
        expected_stack_top = 22,
    },
    {
        name = "FUNC: retn returns nil",
        code = [[
            .assembly
            .func 0
                pushf   1
                call    0
                type
                ret
            .func 1
                retn
        ]],
        expected_stack_top = 0,   -- nil
    },
    {
        name = "FUNC: parameter order (first param is slot 0)",
        code = [[
            .assembly
            .func 0
                pushf   1
                pushi   100         ; param1
                pushi   7           ; param2
                call    2
                ret
            .func 1
                dupv    0           ; param1 = 100
                dupv    1           ; param2 = 7
                sub                 ; 100 - 7
                ret
        ]],
        expected_stack_top = 93,
    },
    {
        name = "FUNC: zero-parameter call",
        code = [[
            .assembly
            .func 0
                pushf   1
                call    0
                ret
            .func 1
                pushi   55
                ret
        ]],
        expected_stack_top = 55,
    },

    -- =========================================================================
    -- 13. ITERATION EDGE CASES
    -- =========================================================================
    {
        name = "ITER: empty array yields nil immediately",
        code = [[
            .assembly
            .func 0
                pushi   0           ; slot0 result
                newa                ; empty
                pushn
                next                ; -> nil (over)
                bnil    @done
                pushi   1
                set     0           ; should not run
            @done:
                dupv    0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "ITER: empty table yields nil immediately",
        code = [[
            .assembly
            .func 0
                pushi   0
                newt
                pushn
                next
                bnil    @done
                pushi   1
                set     0
            @done:
                dupv    0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "ITER: single-element array",
        code = [[
            .assembly
            .func 0
                pushi   0           ; slot0 sum
                newa
                pushi   77
                appnd
                pushn
            @loop:
                next
                bnil    @done
                dupv    0
                sum
                set     0
                jmp     @loop
            @done:
                pop
                dupv    0
                ret
        ]],
        expected_stack_top = 77,
    },

    -- =========================================================================
    -- 14. STRINGS — concat, len, comparison, interning
    -- =========================================================================
    {
        name = "STRING: concatenation chain",
        code = [[
            .assembly
            .const
                0: "a"
                1: "b"
                2: "c"
            .func 0
                pushc   0
                pushc   1
                sum
                pushc   2
                sum
                ret
        ]],
        expected_stack_top = "abc",
    },
    {
        name = "STRING: len of empty string",
        code = [[
            .assembly
            .const
                0: ""
            .func 0
                pushc   0
                len
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "STRING: equality of equal literals",
        code = [[
            .assembly
            .const
                0: "match"
                1: "match"
            .func 0
                pushc   0
                pushc   1
                eq
                ret
        ]],
        expected_stack_top = true,
    },
    {
        name = "STRING: inequality of different strings",
        code = [[
            .assembly
            .const
                0: "foo"
                1: "bar"
            .func 0
                pushc   0
                pushc   1
                eq
                ret
        ]],
        expected_stack_top = false,
    },
    {
        name = "STRING: len after concatenation",
        code = [[
            .assembly
            .const
                0: "Hello, "
                1: "world!"
            .func 0
                pushc   0
                pushc   1
                sum
                len
                ret
        ]],
        expected_stack_top = 13,
    },

    -- =========================================================================
    -- 15. ERROR HANDLING — thrown value types, nesting, pope balance
    -- =========================================================================
    {
        name = "ERR: throw integer, catch returns it",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                pushi   123
                thrw
                pushi   456
                ret
            @catch:
                ret
        ]],
        expected_stack_top = 123,
    },
    {
        name = "ERR: throw string, catch returns it",
        code = [[
            .assembly
            .const
                0: "boom"
            .func 0
                pushe   @catch
                pushc   0
                thrw
                ret
            @catch:
                ret
        ]],
        expected_stack_top = "boom",
    },
    {
        name = "ERR: throw table, catch inspects a field",
        code = [[
            .assembly
            .const
                0: "code"
            .func 0
                pushe   @catch
                newt
                pushc   0
                pushi   42
                setkv               ; err = { code = 42 }
                thrw
                ret
            @catch:                 ; err table on top
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = 42,
    },
    {
        name = "ERR: pope removes handler, normal flow continues",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                pope                ; remove handler
                pushi   7
                ret
            @catch:
                pushi   0
                ret
        ]],
        expected_stack_top = 7,
    },
    {
        name = "ERR: nested handlers, inner catches",
        code = [[
            .assembly
            .func 0
                pushe   @outer
                pushe   @inner
                pushi   1
                thrw                ; caught by inner (most recent)
                ret
            @inner:
                pushi   100
                ret
            @outer:
                pushi   200
                ret
        ]],
        expected_stack_top = 100,
    },
    {
        name = "ERR: catchable type error in expression (table + table)",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                newt
                newt
                sum
                pushi   1
                ret
            @catch:
                pushi   0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "ERR: catchable type error (neg of table)",
        code = [[
            .assembly
            .func 0
                pushe   @catch
                newt
                neg
                ret
            @catch:
                pushi   0
                ret
        ]],
        expected_stack_top = 0,
    },
    {
        name = "ERR: catchable type error (string - integer)",
        code = [[
            .assembly
            .const
                0: "x"
            .func 0
                pushe   @catch
                pushc   0
                pushi   1
                sub
                ret
            @catch:
                pushi   0
                ret
        ]],
        expected_stack_top = 0,
    },

    -- ---- advanced error scenarios (VERIFY: depend on exact unwind semantics) -
    --
    -- tyc_throw_raw unwinds the location stack but does NOT pop frame pointers
    -- (stack_pop_fp) on the value stack. Cross-frame re-throws may leave stale
    -- frames. These mirror intended semantics; verify before trusting results.
    --
    -- { name = "ERR: re-throw from inner handler to outer",
    --   code = [[ .assembly
    --     .func 0
    --         pushe   @outer
    --         pushf   1
    --         call    0
    --         ret
    --     @outer:
    --         ret
    --     .func 1
    --         pushe   @inner
    --         pushi   42
    --         thrw
    --         ret
    --     @inner:
    --         thrw                ; re-throw caught value to outer
    --         ret ]],
    --   expected_stack_top = 42 },

    -- ---- uncatchable errors (COMMENTED: bare ERROR() returns, not throws) ----
    --
    -- These propagate TYC_ERR out of tyc_call rather than routing through the
    -- handler stack, so pushe/@catch will NOT catch them. Some also abort()
    -- under CHECK_TYCHE_BUGS (value_heap_key on a non-heap type). If your harness
    -- can assert "tyc_call returned error", convert these into that form.
    --
    -- { name = "ERR: len of integer (uncatchable ERROR)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 5
    --         len
    --         ret ]] },
    --
    -- { name = "ERR: call a non-function (uncatchable)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 5
    --         call 0
    --         ret ]] },
    --
    -- { name = "ERR: geti on a table, not array (abort under CHECK_TYCHE_BUGS)",
    --   code = [[ .assembly
    --     .func 0
    --         newt
    --         geti 0
    --         ret ]] },
    --
    -- { name = "ERR: setsupertable on a non-table (uncatchable ERROR)",
    --   code = [[ .assembly
    --     .func 0
    --         pushi 1
    --         pushn
    --         sptb
    --         ret ]] },
    --
    -- { name = "ERR: pope with no handler (underflow ERROR)",
    --   code = [[ .assembly
    --     .func 0
    --         pope
    --         ret ]] },

    -- =========================================================================
    -- 16. GLOBALS — shared across frames
    -- =========================================================================
    {
        name = "GLOBAL: written in one frame, read in another",
        code = [[
            .assembly
            .const
                0: "g"
            .func 0
                glbl
                pushc   0
                pushi   77
                setkv
                pop
                pushf   1
                call    0
                ret
            .func 1
                glbl
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = 77,
    },
    {
        name = "GLOBAL: same table identity across two glbl reads",
        code = [[
            .assembly
            .const
                0: "n"
            .func 0
                glbl
                pushc   0
                pushi   1
                setkv
                pop
                glbl                ; second fetch of same global table
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = 1,
    },

    -- =========================================================================
    -- 17. GC — reachability through nesting and globals
    -- =========================================================================
    {
        name = "GC: value referenced from global survives collection",
        code = [[
            .assembly
            .const
                0: "keep"
                1: "payload string here"
            .func 0
                glbl
                pushc   0
                pushc   1
                setkv               ; global.keep = <string>
                pop
                gc                  ; string is reachable via global -> survives
                glbl
                pushc   0
                getkv
                ret
        ]],
        expected_stack_top = "payload string here",
    },
    {
        name = "GC: unreferenced array is collected",
        code = [[
            .assembly
            .func 0
                newa
                pushi   1
                seti    0
                pop                 ; drop sole reference
                gc
                pushi   1           ; sentinel so we have a defined top
                ret
        ]],
        -- only global table remains on heap
        expected_heap_size = 1,
        expected_stack_top = 1,
    },
    {
        name = "GC: nested array keeps inner alive while outer referenced",
        code = [[
            .assembly
            .func 0
                newa                ; outer
                newa                ; inner
                pushi   9
                seti    0           ; inner[0] = 9
                seti    0           ; outer[0] = inner
                gc                  ; outer on stack -> inner reachable
                geti    0           ; fetch inner
                geti    0           ; inner[0]
                ret
        ]],
        expected_stack_top = 9,
    },

    -- ---- GC cycle (VERIFY: does the collector handle cycles?) ----------------
    --
    -- A table referencing itself, then dropped, must still be collected by a
    -- proper mark-sweep. If the GC is refcount-based this would leak.
    -- Enable and assert expected_heap_size once GC strategy is confirmed.
    --
    -- { name = "GC: self-referential table is collected after drop",
    --   code = [[ .assembly
    --     .const
    --         0: "self"
    --     .func 0
    --         newt
    --         dup
    --         pushc 0
    --         dup ... ]] },  -- (sketch; setkv arg order: table,key,value)
}
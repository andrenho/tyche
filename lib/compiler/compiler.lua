----------------------
--                  --
--      PARSER      --
--                  --
----------------------

local function parse_assembly(source)
    local proto = {
        constants = {},
        functions = {},
    }

    local section = ''
    local current_f_id = 0

    local next_label = nil
    for line in source:gmatch("([^\n]+)") do
        local line = line:gsub("%s*;.*$", "") -- remove comments
        line = line:match("^%s*(.-)%s*$")     -- trim

        if #line == 0 then goto continue end

        if line == ".const" then
            section = 'const'
        elseif line:match("%.func%s+%d+") then
            section = 'function'
            local f_id = tonumber(line:match("%.func%s+(%d+)"))
            proto.functions[f_id] = {}
            current_f_id = f_id
        elseif section == 'const' then
            local k, v = line:match("^%s*(%d+)%s*:%s*(.+)$")
            if not k then error("Invalid row for constant: " .. line) end
            if v:sub(1, 1) == '"' then
                proto.constants[tonumber(k)] = line:match('"(.*)"')
            else
                proto.constants[tonumber(k)] = tonumber(v)
            end
        elseif section == 'function' then
            local regexes = {
                "^%s*(%a+)%s+(-?%d+)%s*$",          -- instruction + parameter
                "^%s*(%a+)%s+(@[%a_][%a%d_]*)%s*$", -- instruction + label
                "^%s*(%a+)%s*$",                    -- instruction only
                "^(@[%a_][%a%d_]*):%s*$",           -- label
            }
            local match = false
            for i, regex in ipairs(regexes) do
                local inst, par = line:match(regex)
                if inst then
                    match = true
                    if i == 1 then     -- instruction + parameter
                        table.insert(proto.functions[current_f_id], { inst, tonumber(par), labels = next_label })
                    elseif i == 2 then -- instruction + label
                        table.insert(proto.functions[current_f_id], { inst, par, labels = next_label })
                    elseif i == 3 then -- instruction only
                        table.insert(proto.functions[current_f_id], { inst, labels = next_label })
                    elseif i == 4 then -- label
                        if not next_label then
                            next_label = { inst }
                        else
                            table.insert(next_label, inst)
                        end
                    end
                    if i ~= 4 then
                        next_label = nil
                    end
                end
            end
            if not match then error("Invalid instruction: " .. line) end
        end

        ::continue::
    end

    return proto
end

----------------------
--                  --
--     BINARY       --
--                  --
----------------------

local instructions = {
    -- stack operations
    pushi = 0xa0,
    pushc = 0xa1,
    pushf = 0xa2,
    pushn = 0x00,
    pushz = 0x01,
    pusht = 0x02,
    newa  = 0x03,
    newt  = 0x04,
    pop   = 0x05,
    dup   = 0x06,

    -- local variables
    pushv = 0xa3,
    set   = 0xae,
    dupv  = 0xa4,
    setg  = 0xa5,
    glbl  = 0xa6,

    -- function operations
    call = 0xa7,
    ret  = 0x10,
    reti = 0x11,

    -- table and array operations
    getkv = 0x16,
    setkv = 0x17,
    geti  = 0xa8,
    seti  = 0xa9,
    appnd = 0x18,
    next  = 0x19,
    sptb  = 0x1a,
    supr  = 0x1b,

    -- logical/arithmetic
    sum     = 0x20,
    sub     = 0x21,
    mul     = 0x22,
    div     = 0x23,
    idiv    = 0x24,
    mod     = 0x25,
    eq      = 0x26,
    neq     = 0x27,
    lt      = 0x28,
    lte     = 0x29,
    gt      = 0x2a,
    gte     = 0x2b,
    ['and'] = 0x2c,
    ['or']  = 0x2d,
    xor     = 0x2e,
    pow     = 0x2f,
    shl     = 0x30,
    shr     = 0x31,

    -- other value operations
    len  = 0x40,
    type = 0x41,
    cast = 0xad,
    ver  = 0x42,

    -- external code
    cmpl  = 0x48,
    asmbl = 0x49,
    load  = 0x4a,

    -- control flow
    bz   = 0xca,
    bnz  = 0xcb,
    bnil = 0xcf,
    jmp  = 0xcc,

    -- memory management
    gc = 0x4b,
}

local MAGIC = 0xa7d6e9b1
local VERSION = 1

local function assemble(proto)
    local bin = {}

    local push8 = function(data)
        table.insert(bin, data & 0xff)
    end

    local push16 = function(data)
        table.insert(bin, data & 0xff)
        table.insert(bin, (data >> 8) & 0xff)
        return #bin - 1
    end

    local push32 = function(data)
        table.insert(bin, data & 0xff)
        table.insert(bin, (data >> 8) & 0xff)
        table.insert(bin, (data >> 16) & 0xff)
        table.insert(bin, (data >> 24) & 0xff)
        return #bin - 3
    end

    local push64 = function(data)
        table.insert(bin, data & 0xff)
        table.insert(bin, (data >> 8) & 0xff)
        table.insert(bin, (data >> 16) & 0xff)
        table.insert(bin, (data >> 24) & 0xff)
        table.insert(bin, (data >> 32) & 0xff)
        table.insert(bin, (data >> 40) & 0xff)
        table.insert(bin, (data >> 48) & 0xff)
        table.insert(bin, (data >> 56) & 0xff)
        return #bin - 7
    end

    local replace16 = function(pos, data)
        bin[pos] = data & 0xff
        bin[pos + 1] = (data >> 8) & 0xff
    end

    local replace32 = function(pos, data)
        bin[pos] = data & 0xff
        bin[pos + 1] = (data >> 8) & 0xff
        bin[pos + 2] = (data >> 16) & 0xff
        bin[pos + 3] = (data >> 24) & 0xff
    end

    local function double_to_bits(f)
        local bytes = string.pack("<d", f)
        return string.unpack("<I8", bytes)
    end

    -- header
    push32(MAGIC)
    push16(VERSION)
    push16(0)

    -- constants
    local code_addr_pos = push32(0) -- code address, to be replaced

    -- number of constants
    if proto.constants[0] then
        push32(#proto.constants + 1)
    else
        push32(0)
    end

    for i=0,#proto.constants do
        local const = proto.constants[i]
        if type(const) == 'string' then
            push8(0) -- string type
            for c in const:gmatch('.') do
                push8(c:byte())
            end
            push8(0) -- string terminator
        elseif type(const) == 'number' then
            push8(1) -- float type
            push64(double_to_bits(const))
        end
    end

    replace32(code_addr_pos, #bin)

    -- code
    push32(0)                    -- debug address (TODO)
    push32(#proto.functions + 1) -- number of functions

    for i = 0, #proto.functions do

        local func = proto.functions[i]
        local next_function_pos = #bin + 1
        push32(0) -- to be replaced with next function address

        local function_start = #bin
        local labels = {}

        for _, inst in ipairs(func) do
            -- add labels
            if inst.labels then
                for _, lbl in ipairs(inst.labels) do
                    labels[lbl] = #bin - function_start
                end
            end

            local opcode, operand = instructions[inst[1]], inst[2]
            if opcode == nil then error("Unknown instruction " .. inst[1]) end
            if operand == nil then
                push8(opcode)
            elseif type(operand) == 'string' then
                push8(opcode)
                table.insert(bin, operand) -- insert the label
                push8(0)  -- byte to be replaced (label is 16-bit)
            else
                if opcode >= 0xc0 and opcode < 0xe0 then
                    push8(opcode)
                    push16(operand)
                elseif operand >= -128 and operand <= 127 then
                    push8(opcode)
                    push8(operand)
                elseif operand >= -32768 and operand <= 32767 then
                    push8(opcode + 0x20)
                    push16(operand)
                else
                    push8(opcode + 0x40)
                    push32(operand)
                end
            end

        end

        -- replace labels
        for i=function_start,#bin do
            if type(bin[i]) == 'string' then
                local label_addr = labels[bin[i]]
                if label_addr == nil then error("Label not found: " .. bin[i]) end
                replace16(i, label_addr)
            end
        end

        replace32(next_function_pos, #bin)
    end

    -- for _, b in ipairs(bin) do io.write(string.format("%02x", b) .. ' ') end; print()
    return string.char(table.unpack(bin))

end

----------------------
--                  --
--     GENERIC      --
--                  --
----------------------

return function(source)
    return assemble(parse_assembly(source))
end
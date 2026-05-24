## C

Decisions:
 - How to handle errors
 - How values and heap values will be represented
 - Transparency and log levels

# Version 0

- [x] Makefile
- [x] VALUE
- [x] Stack
  - [x] Test application
- [x] Heap
  - [x] Heap value
  - [x] Strings
  - [x] Arrays
  - [x] Tables
- [x] VM
  - [x] (Lua interface) call assembler
  - [x] (Lua) generate bytecode
    - [x] Labels
  - [x] Code
    - [x] Interpret bytecode (fast)
  - [x] Execution loop (fast)
- [x] VM operations
  - [x] Expressions
  - [x] Local variables
  - [x] Functions
    - [x] With parameters
  - [x] Debug VM execution
  - [x] Control flow
    - [x] Recursion
  - [x] Strings
    - [x] From constants
    - [x] Garbage collection
  - [x] Arrays
    - [x] Garbage collection
  - [x] Tables
    - [x] Garbage collection
    - [x] Iteration
  - [x] Run GC on tyc destruction (also: tyc_gc())
  - [x] Improve error reporting
    - macro
    - messages
    - abort if debug
  - [x] Globals (?)
  - [ ] Expression improvements
    - [x] Supertables
    - [x] Boolean and floats
      - [x] Support for booleans
      - [x] Boolean opcodes
      - [x] Support for doubles in const
    - [ ] Operators
      - [x] Real
      - [x] Mix real and integer
      - [x] Unary operators (NOT, NEG, INC, DEC, LEN)
      - [x] Boolean expressions (and, or, xor, eq, neq, not)
      - [x] Len (table, array, string)
      - [x] Type, ver
  - [x] Error handling
  - [ ] Native pointers
  - [ ] Run GC occasionally
  - [ ] Assembler in C
  - [ ] Main application
- [ ] Prepare for release
  - [ ] Review public function names
  - [ ] Documentation and webpage

# Version 1

- [ ] Compiler
- [ ] REPL
- [ ] Debugging information
- [ ] Stack trace in case of errors
- [ ] Debugger

# Version 2

- [ ] Closure/upvalues
- [ ] Table, array equality
- [ ] "+" for tables, array
- [ ] % for printf (?)
- [ ] << instead of appnd
- [ ] Operator overloading
- [ ] stdlib
- [ ] regex

## Future versions

- [ ] Debugging information
- [ ] Debugger
- [ ] Dynamic language
- [ ] Support tools
  - [ ] Editor syntax file, etc
- [ ] Static language


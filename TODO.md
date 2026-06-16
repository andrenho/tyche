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
  - [x] Expression improvements
    - [x] Supertables
    - [x] Boolean and floats
      - [x] Support for booleans
      - [x] Boolean opcodes
      - [x] Support for doubles in const
    - [x] Operators
      - [x] Real
      - [x] Mix real and integer
      - [x] Unary operators (NOT, NEG, INC, DEC, LEN)
      - [x] Boolean expressions (and, or, xor, eq, neq, not)
      - [x] Len (table, array, string)
      - [x] Type, ver
  - [x] Error handling
  - [x] Native pointers
  - [x] Run GC occasionally
  - [x] Assembler in C
  - [x] Main application
  - [ ] Improvements
    - [x] Rename "operator" (because of C++)
    - [x] Move away from Lua
    - [x] Equality for multiple types
    - [x] Implement hash for VM
    - [x] Test for hash and equality
    - [x] Table
      - [x] Store multiple values for the same hash
    - [ ] Null native pointer
    - [ ] Test tyc_global
    - [x] Array equality
    - [ ] Table loop detection (ex. `A <- B <- A` and `A: { B: A }`)
      - [ ] table_get, table_set, table_del
      - [ ] table_next
      - [ ] GC
- [ ] Prepare for release
  - [x] Documentation and webpage
  - [ ] Review public function names
  - [ ] Actions
  - [ ] Code review

# Version 1

- [ ] Compiler
- [ ] REPL
- [ ] Debugging information
- [ ] Stack trace in case of errors
- [ ] Debugger
- [ ] REPL

# Version 2

- [ ] Closure/upvalues
- [ ] Table, array equality
- [ ] Operator overload (new opcode)
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


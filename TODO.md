## Bytecode

- [x] Byte array
  - Auto-expand
  - Add/retrive byte/int/float/string
  - Should not be larger than the byte array itself
- [x] Bytecode
  - Add/retrive all types of data
  - Keeps no memory except for caching
  - [x] Refactor bytecode code

Improvements:
- [x] Fixed int type (based on opcode)
- [x] Constant type (only floats and strings for now)

After some additional development:
- [ ] Bytecode debugging info


## VM

- [x] VM
  - [x] Code
    - [x] Simple bytecode loader
      - [x] Output bytecode format
  - [x] Value object
  - [x] Stack object
  - [x] External interface
  - [x] Code execution (except functions)
  - [x] Functions
  - [x] Print stack
- [x] Assembler
- [ ] VM execution
  - [x] Stack operations (nil, integer, float, string, function)
    - [x] Integer
    - [x] Float
    - [x] String
  - [x] Expressions
    - [x] Integer
    - [x] Float
    - [x] String
  - [ ] Local/global variables
  - [ ] Functions
  - [ ] Constants
  - [ ] Other operations
  - [ ] Arrays
    - [ ] Iteration
    - [ ] Expressions
  - [ ] Tables
    - [ ] Iteration
    - [ ] Metatables
    - [ ] Expressions
  - [ ] Control flow
  - [ ] Compilation
  - [ ] Error handling
- [ ] C++ API
  - [ ] Run native code on VM
  - [ ] Run tyche code from C++
  - [ ] C API

After some additional development:
- [ ] Bytecode loader
  - Combine multiple chunks
  - Resolve function ids, constant ids, etc
- [ ] Upvalues
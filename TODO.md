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

- [ ] VM
  - [x] Code
    - [x] Simple bytecode loader
      - [x] Output bytecode format
  - [x] Value object
  - [x] Stack object
  - [ ] External interface
  - [ ] Code execution (except functions)
  - [ ] Functions

After some additional development:
- [ ] Bytecode loader
  - Combine multiple chunks
  - Resolve function ids, constant ids, etc
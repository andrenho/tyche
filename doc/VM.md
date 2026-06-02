TycheVM is a stack-based VM for the Tyche language. It's meant to run interpreted.

Types
-----

* __Nil__: represent a null, non-initialized or missing value.
* __Boolean__: true/false.
* __Integer__: a 32-bit signed integer value.
* __Real__: a 64-bit floating point value (equivalent to `double` in C). It can be NaN too.
* __String__: a char string. It can either be:
  * a static string constant in the binary - it's just referred and stored in stack/heap as a integer reference
  * a string created during runtime.
* __Array__: an array of values.
* __Table__: a hashmap. See below.
* __Function__: a reference to a Tyche function.
* __Native Pointer__: a pointer to data in C (equivalent to a `void *`)


Tables
------

Tables are hashmaps that can use any type of value as a key. The most common type for key is `string`.

Table keys are resolved in two steps

1) Calculate hash value of the key value _(see below)_
2) Run the equality operator (which can be overloaded for tables)

# Hash value of keys

Boolean, integer, real, functions and native pointer use the value itself to calculate the hash.

Tables calculate, by default, the hash from a conjunction of the keys and value hashes, and also equality
for all keys/values. This can be slow, and both the hash function and equality function can be overriden.

Arrays compute a hash based on the hash of all records in the table. This ensures that equal arrays will
have the same key in a table.

Nils can't be used as keys.

## Supertables

A table can be linked to a supertable, which works similar to inheritance. The rules are:

1) On the moment a table is linked to a supertable (at runtime), all supertable's keys and values are copied to the
   child table (except for functions).
2) When a field is accessed in the table, and that field is not find in the table, the corresponding field
   in the supertable is used, if found AND it's a function.

A supertable can also have its own supertable, creating a chain of calls.

This is useful for creating Object Orientation, for example:

```
ParentClass = {
    defaultValue = 3;
    
    doSomething = method() { 
        print("Parent class, my default value is %d" % [self.defaultValue]);
    }
}

ChildClass = {
    defaultValue = 4;
    
    new = func() {
        return {} <- ChildClass
    }
    
    doSomething = method() { 
        print("Child class, my default value is %d" % [self.defaultValue]);
    }
} <- ParentClass

my_object := ChildClass.new();
my_object.doSomething();             // result: Child class, my default value is 4
```

## Operator overloading

The following special fields can be used to do operation overloading on tables:

__sum   +
__sub   -
__mul   *
__div   /
__idiv  |/
__mod   %
__eq    ==  (automatic: neq)
__lt    <   (automatic: lte)
__gt    >   (automatic: gte)
__and   &
__or    |
__xor   ~
__pow   ^
__shl   <<
__shr   >>
__hash  hash

These are special keys which do not share the same space as regular keys.


Memory
------

Every value is stored in a [nanbox](https://github.com/philihp/nanbox), and takes up 64 bits (8 bytes).

## Stack

TycheVM is stack based, and the stack is where most operations happen. Values can be pushed, popped,
and accessed out of order.

There's a secondary FP (frame pointer) stack. The top of that stack points to a stack position that serves
as top of the stack. The goal of this is to allow entering/leaving scopes, particularly when entering/leaving
functions.

Example: stack positions below the FP top are invisible and shown as XX.

```
Stack: XX XX XX XX 00 01 02 03
                   ^
                  FP
```

## Heap

The heap stores values which are larger than 64-bits: non-static strings, arrays, tables and native function pointers.

Every time the number of allocations hits a ceiling, the GC (garbage collector) is executed. The garbage collector is
a mark & sweep GC that will receive (1) the global and (2) values in the stack, and will clean up anything that is not
part of these inputs.

## Global

There's a single global table that serves as global, this global table can be loaded and changed at any point of the 
runtime.


Error management
----------------

## Exceptions

TycheVM support exceptions. A try block will push an error handler into an error handler stack, and when a error is
thrown it'll pop our of the error handler stack to know where to return to.

There's no support for `finally` blocks at this time.

## Default error format

This is the error format, when it's thrown due to an error in the VM. Users are encouraged to use the same format,
but are free to use any object type.

```
{ error: "Error message" }
```

Additional fields:
  * if debugging info: file + line
  * if no debugging info: function_id + pc

-------------------------------------------------------------------------------------------------------------------

Reference - Virtual Machine Opcodes
-----------------------------------

Operations take either 0 or 1 parameter. The ones that take a parameter, it can be either an int8, int16 or int32.

Instructions follow this logic:

| Range       | Parameter       |
| ----------- | --------------- |
| `00` ~ `9F` | no parameter    |
| `A0` ~ `BF` | int8 (1 byte)   |
| `C0` ~ `DF` | int16 (2 bytes) |
| `E0` ~ `FF` | int32 (4 bytes) |

The operations of 1, 2 and 4 bytes are always interchangeable by adding/subtracting `0x20`.

Column legend:

- **NP** — no parameter
- **I8** — int8
- **I16** — int16
- **I32** — int32

## Stack operations

| NP   | I8   | I16  | I32  | Instruction        | Description               |
| ---- | ---- | ---- | ---- | ------------------ | ------------------------- |
|      | `a0` | `c0` | `e0` | `pushi [int]`      | Push int                  |
|      | `a1` | `c1` | `e1` | `pushc [index]`    | Push constant             |
|      | `a2` | `c2` | `e2` | `pushf [function]` | Push function id          |
| `00` |      |      |      | `pushn`            | Push nil                  |
| `01` |      |      |      | `pushz`            | Push zero (or false)      |
| `02` |      |      |      | `pusht`            | Push true                 |
| `03` |      |      |      | `newa`             | Push (create) empty array |
| `04` |      |      |      | `newt`             | Push (create) empty table |
| `05` |      |      |      | `pop`              |                           |
| `06` |      |      |      | `dup`              |                           |

## Local variables

| NP   | I8   | I16  | I32  | Instruction    | Description                                                |
| ---- | ---- | ---- | ---- | -------------- | ---------------------------------------------------------- |
|      | `a3` | `c3` | `e3` | `pushv [int]`  | Push n nil values into the stack (used to init local vars) |
|      | `ae` | `ce` | `ee` | `set [index]`  | Set value in stack position (set local variable)           |
|      | `a4` | `c4` | `e4` | `dupv [index]` | Duplicate stack value (load local variable)                |
| `07` |      |      |      | `glbl [int]`   | Get global table                                           |

## Function operations

| NP   | I8   | I16  | I32  | Instruction     | Description                                                                  |
| ---- | ---- | ---- | ---- | --------------- | ---------------------------------------------------------------------------- |
|      | `a7` | `c7` | `e7` | `call [n_pars]` | Enter function on stack toplevel (passing n next stack values as parameters) |
| `10` |      |      |      | `ret`           | Leave a function (return value in stack)                                     |
| `11` |      |      |      | `retn`          | Leave a function (return nil)                                                |

## Table and array operations

| NP   | I8   | I16  | I32  | Instruction | Description                                                                |
| ---- | ---- | ---- | ---- |-------------| -------------------------------------------------------------------------- |
| `16` |      |      |      | `getkv`     | Get table's value based on key (pull 1 value, push 1 value)                |
| `17` |      |      |      | `setkv`     | Set table's key and value (pull 2 values from stack)                       |
| `1c` |      |      |      | `setop`     | Overload table's operator                                                  |
|      | `a8` | `c8` | `e8` | `geti`      | Get array's position value                                                 |
|      | `a9` | `c9` | `e9` | `seti`      | Set array's position value                                                 |
| `18` |      |      |      | `appnd`     | Add value to the end of array                                              |
| `19` |      |      |      | `next`      | Push the next pair into the stack (for loops), pushes nil as key when over |
| `1a` |      |      |      | `sptb`      | Set table supertable                                                       |
| `1b` |      |      |      | `supr`      | Fetch supertable                                                           |

## Logical/arithmetic

| NP   | Instruction | Description                           |
| ---- | ----------- | ------------------------------------- |
| `20` | `sum`       | Sum top 2 values in stack             |
| `21` | `sub`       | Subtract top 2 values in stack        |
| `22` | `mul`       | Multiply top 2 values in stack        |
| `23` | `div`       | Float division                        |
| `24` | `idiv`      | Integer division                      |
| `25` | `mod`       | Modulo                                |
| `26` | `eq`        | Equality                              |
| `27` | `neq`       | Inequality                            |
| `28` | `lt`        | Less than                             |
| `29` | `lte`       | Less than or equals                   |
| `2a` | `gt`        | Greater than                          |
| `2b` | `gte`       | Greater than or equals                |
| `2c` | `and`       | Bitwise AND                           |
| `2d` | `or`        | Bitwise OR                            |
| `2e` | `xor`       | Bitwise XOR                           |
| `2f` | `pow`       | Power                                 |
| `30` | `shl`       | Shift left                            |
| `31` | `shr`       | Shift right                           |
| `32` | `not`       | Invert value bits (or negate boolean) |
| `33` | `neg`       | Invert number sign (negative)         |
| `34` | `hash`      | Calculate hash for a value            |

## Other value operations

| NP   | Instruction | Description                                 |
| ---- | ----------- | ------------------------------------------- |
| `40` | `len`       | Get table, array or string size             |
| `41` | `type`      | Get type from value at the top of the stack |
| `42` | `ver`       | Return VM version                           |

## External code

| NP   | Instruction | Description                                              |
| ---- | ----------- | -------------------------------------------------------- |
| `48` | `cmpl`      | Compile code to assembly                                 |
| `49` | `asmbl`     | Assemble code to bytecode format                         |
| `4a` | `load`      | Load bytecode as function (will place function on stack) |

## Control flow

The destination is always a 16-bit field.

| I16  | Instruction | Description        |
| ---- | ----------- | ------------------ |
| `ca` | `bz [pc]`   | Branch if zero     |
| `cb` | `bnz [pc]`  | Branch if not zero |
| `cf` | `bnil [pc]` | Branch if nil      |
| `cc` | `jmp [pc]`  | Unconditional jump |

> Jumps can only happen within the same function.

## Memory management

| NP   | Instruction | Description            |
| ---- | ----------- | ---------------------- |
| `4b` | `gc`        | Call garbage collector |

## Error handling

| NP   | I16  | Instruction  | Description        |
| ---- | ---- | ------------ | ------------------ |
|      | `cd` | `pushe [pc]` | Push error handler |
| `5a` |      | `pope`       | Pop error handler  |
| `5b` |      | `thrw`       | Throw              |


Reference - Bytecode format
---------------------------

```
The bytecode file is composed of the following sections:

 * HEADER: 8-byte header
   [0:3]: Magic
   [4:5]: Bytecode version
   [6:7]: Reserved for future use

 * CONSTANTS
   [0:3]: Code start address
   [4:7]: Number of constants
   Each constant:
     [0]: Type (0 = string, 1 = real)
     if string:
       [...]:  string (NULL terminated)
     if real
       [1..4]: real

 * CODE
   [0:3]: Debug start address (or zero)
   [4:7]: Number of functions
   Each function:
     [0:3] Address of next function (include this on last too)
     [...] Code
       [0] : Opcode
       [between 1 and 4] : Operand

The max file size is 2 Gb.

## Values can be encoded in the following ways:
 * The type is defined by the operator.
 * Constant indexes and function ids are encoded as ints
```


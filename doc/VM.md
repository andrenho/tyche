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

Nil, boolean, integer, real, functions and native pointer use the value itself to calculate the hash.

Tables calculate, by default, the hash from a conjunction of the keys and value hashes, and also equality
for all keys/values. This can be slow, and both the hash function and equality function can be overriden.

Arrays compute a hash based on the hash of all records in the table. This ensures that equal arrays will
have the same key in a table.

## Supertables

A table can be linked to a supertable, which works similar to inheritance. The rules are:

1) On the moment a table is linked to a supertable, all supertable's keys and values are copied to the
   child table (except for functions).
2) When a field is accessed in the table, and that field is not find in the table, the corresponding field
   in the supertable is used, if found AND it's a function.

A supertable can also have its own supertable, creating a chain of calls.

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

Errors
------

## Error format

{ error: "Error message" }

Additional fields:
if debugging info: file + line
if no debugging info: function_id + pc

Errors:
- OutOfRange



Additional things not implemented yet
-------------------------------------

## Operator overloading

_(not implemented yet)_

sum   +
sub   -
mul   *
div   /
idiv  |/
mod   %
eq    ==  (automatic: neq)
lt    <   (automatic: lte)
gt    >   (automatic: gte)
and   &
or    |
xor   ~
pow   ^
shl   <<
shr   >>
# tyche

![Cornucopia](doc/cornucopia-64.png)

A programming language that can be run either by itself, or embedded in an application.

The language has the following inspirations:

* internals are inspired by Lua
* language syntax is similar to C/C++/Go/Javascript
* standard library is based on Python's and Ruby's

At this point only the VM is available.

## Documentation

* Bytecode format: [doc/BYTECODE](doc/BYTECODE)
* VM opcodes: [doc/OPCODES](doc/OPCODES)
* Types and memory management: [doc/VM.md](doc/VM.md)
* C interface: [lib/tyche.h](lib/tyche.h)
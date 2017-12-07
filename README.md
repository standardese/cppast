# cppast

[![Build Status](https://travis-ci.org/foonathan/cppast.svg?branch=master)](https://travis-ci.org/foonathan/cppast)
[![Build status](https://ci.appveyor.com/api/projects/status/8gp5btjq7eassvn7?svg=true)](https://ci.appveyor.com/project/foonathan/cppast)
[![Coverage Status](https://coveralls.io/repos/github/foonathan/cppast/badge.svg)](https://coveralls.io/github/foonathan/cppast)

Library interface to the C++ AST &mdash; parse source files, synthesize entities, get documentation comments and generate code.

[![Patreon](https://c5.patreon.com/external/logo/become_a_patron_button.png)](https://patreon.com/foonathan)

## Motivation

If you're writing a tool that needs access to the C++ AST (i.e. documentation generator, reflection library, …), your only option apart from writing your own parser is to use [clang](https://clang.llvm.org).
It offers [three interfaces for tools](https://clang.llvm.org/docs/Tooling.html), but the only one that really works for standalone applications is [libclang](http://clang.llvm.org/doxygen/group__CINDEX.html).
However, libclang has various limitations and does not expose the entire AST.

So there is no feasible option &mdash; except for this library.
It was originally a part of the [standardese documentation generator](http://standardese.foonathan.net), but has been extracted into an independent library.

See [this blog post](http://foonathan.net/blog/2017/04/20/cppast.html) for more information about the motiviation and design.

## Features

* Exposes (almost) all C++ entities: Supports everything from functions to classes, templates to friend declarations, macros to enums;
* Exposes full information about C++ types;
* Supports and exposes documentation comments in various formats with smart entity matching;
* Supports C++11 attributes (including user-defined ones);
* AST hierarchy completely decoupled from parser: This allows synthesizing AST entities and multiple parsing backends;
* Parser based on libclang: While libclang does have its limitations and/or bugs, the implemented parser uses various workarounds/hacks to provide a parser that breaks only in rare edge cases you won't notice. See [issues tagged with `libclang-parser` for a list](https://github.com/foonathan/cppast/issues?q=is%3Aissue+is%3Aopen+label%3Alibclang-parser);
* Simple yet customizable code generation interface.

## Missing features

* Support modification of parsed entities: they're currently all immutable, need to find a decent way of implementing that
* Full support for expressions: currently only literal expressions are exposed;
* Support for statements: currently function bodies aren't parsed at all;
* Support for member specialization: members of a template can be specialized separately, this is not supported.

## Example

See [tool/main.cpp](tool/main.cpp) for a simple application of the library that prints the AST.

## Documentation

TODO, refer to documentation comments in header file.

### Installation

The library can be used as CMake subdirectory, download it and call `add_subdirectory(path/to/cppast)`, then link to the `cppast` target and enable C++11 or higher.

The parser needs `libclang` and the `clang++` binary, at least version 3.9.1, but works better with 4.0.0.

*Note: The project will drop support for older LLVM versions very soon; this minimizes the workaround code when the `libclang` API catches up.*

The CMake code requires `llvm-config`, you may need to install `llvm` and not just `clang` to get it (e.g. on ArchLinux).
If `llvm-config` is in your path and the version is compatible, it should just work out of the box.
Else you need to set the CMake variable `LLVM_CONFIG_BINARY` to the proper path.

If you don't have a proper clang version installed, it can also be downloaded.
For that you need to set `LLVM_DOWNLOAD_OS_NAME`.
This is the name of the operating system used on the [LLVM pre-built binary archive](http://releases.llvm.org/download.html#4.0.0), e.g. `x86_64-linux-gnu-ubuntu-16.10` for Ubuntu 16.10.

If you don't have `llvm-config`, you need to pass the locations explictly.
For that set the option `LLVM_VERSION_EXPLICIT` to the version you're using, `LIBCLANG_LIBRARY` to the location of the libclang library file, `LIBCLANG_INCLUDE_DIR` to the directory where the header files are located (so they can be included with `clang-c/Index.h`), `LIBCLANG_SYSTEM_INCLUDE_DIR` where the system header files are located (i.e. `stddef.h` etc, usually under `prefix/lib/clang/<version>/include`) and `CLANG_BINARY` to the full path of the `clang++` exectuable.

The other dependencies like [type_safe](http://type_safe.foonathan.net) are installed automatically with git submodules, if they're not installed already.

If you run into any issues with the installation, please report them.

### Quick API Overview

There are three class hierarchies that represent the AST:

* `cpp_entity`: This is the base class for all C++ *entities*, i.e. declarations/definitions or things like `static_assert()` and function parameters;
* `cpp_type`: This is the base class for the C++ type hierachy. It is used in the `cpp_entity` hierachy, i.e. `cpp_type_alias` contains an `underlying_type()`. Derived classes are, for example, `cpp_builtin_type` or `cpp_pointer_type`;
* `cpp_expression`: This is the base class for all C++ expressions. It is used in the `cpp_entity` hierarchy, i.e. `cpp_function_parameter` contains a `default_value()` as expression. Derived classes are currently only `cpp_literal_expression` and `cpp_unexposed_expression`;

In order to parse a C++ source file, you need an implementation of `parser`.
The library provides one, `libclang_parser`, but you could also write one yourself.
Parsing is as simple as calling the `parse()` member function passing it three things:

* An object of type `cpp_entity_index`: This is only required to resolve cross-references in the AST (i.e. if you want to get the `cpp_class` referenced in the return type of a `cpp_function`); it does not own the entities;
* The path to the file;
* An object of a type derived from `compile_config`: It stores the compilation flags used for compiling the file, it needs to match the parser, i.e. use `libclang_compile_config` with `libclang_parser`;

It returns `nullptr` on failure and prints diagnostics using a given `diagnostic_logger` &mdash; note that it will only return `nullptr` on fatal parse errors, else it will just skip the one where the error occured.
If everything went succesful, it returns a `std::unique_ptr<cpp_file>` which is the top-level AST entity of the current file.
You can then work with it.

# cppast

Library interface to the C++ AST &mdash; parse source files, synthesize entities, get documentation comments and generate code.

> |[![](https://www.jonathanmueller.dev/embarcadero-logo.png)](https://www.embarcadero.com/de/products/cbuilder/starter) | Sponsored by [Embarcadero C++Builder](https://www.embarcadero.com/de/products/cbuilder/starter). |
> |-------------------------------------|----------------|
>
> If you like this project, consider [supporting me](https://jonathanmueller.dev/support-me/).

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

The parser needs `libclang` and the `clang++` binary, at least version 4.0.0.
The `clang++` binary will be found in `PATH` and in the same directory as the program that is being executed.

*Note: The project will drop support for older LLVM versions very soon; this minimizes the workaround code when the `libclang` API catches up.*

The CMake code requires `llvm-config`, you may need to install `llvm` and not just `clang` to get it (e.g. on ArchLinux).
If `llvm-config` is in your path and the version is compatible, it should just work out of the box.
Else you need to set the CMake variable `LLVM_CONFIG_BINARY` to the proper path.

If you don't have a proper clang version installed, it can also be downloaded.
For that you need to set `LLVM_DOWNLOAD_OS_NAME`.
This is the name of the operating system used on the [LLVM pre-built binary archive](http://releases.llvm.org/download.html#4.0.0), e.g. `x86_64-linux-gnu-ubuntu-16.10` for Ubuntu 16.10.

You can also set `LLVM_DOWNLOAD_URL` to a custom url, to download a specific version or from a mirror.

If you don't have `llvm-config`, you need to pass the locations explictly.
For that set the option `LLVM_VERSION_EXPLICIT` to the version you're using,
`LIBCLANG_LIBRARY` to the location of the libclang library file,
`LIBCLANG_INCLUDE_DIR` to the directory where the header files are located (so they can be included with `clang-c/Index.h`),
and `CLANG_BINARY` to the full path of the `clang++` exectuable.

The other dependencies like [type_safe](http://type_safe.foonathan.net) are installed automatically with FetchContent, if they're not installed already.

If you run into any issues with the installation, please report them.

### Installation on Windows

Similar to the above instructions for `cppast`, there are a couple extra requirements for Windows.

The LLVM team does not currently distribute `llvm-config.exe` as part of the release binaries, so the only way to get it is through manual compilation or from 3rd parties. To prevent version mismatches, it's best to compile LLVM, libclang, and `llvm-config.exe` from source to ensure proper version matching. However, this is a non-trivial task, requiring a lot of time. The easiest way to work with LLVM and `llvm-config.exe` is to leverage the [Chocolatey](https://chocolatey.org/) `llvm` package, and then compile the `llvm-config.exe` tool as a standalone binary.

* Install Visual Studio 2017 with the Desktop C++ development feature enabled.
* Install `llvm` and `clang` with `choco install llvm` 
* Check the version with `clang.exe --version`
* Clone the LLVM project: `git clone https://github.com/llvm/llvm-project`
* Checkout a release version matching the version output, such as 7.0.1, with `git checkout llvmorg-7.0.1`
* `cd llvm-project && mkdir build && cd build` to prep the build environment.
* `cmake -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_TARGETS_TO_BUILD="X86" -G "Visual Studio 15 2017" -Thost=x64 ..\llvm`
  * This will configure clang and LLVM using a 64-bit toolchain. You'll have all the necessary projects configured for building clang, if you need other LLVM tools. See the [LLVM documentation](https://llvm.org/docs/CMake.html) and [clang documentation](http://clang.llvm.org/get_started.html) if you only need more assistance.
* Open the `LLVM.sln` solution, and set the build type to be "Release".
* Build the `Tools/llvm-config` target.
* Copy the release binary to from `build\Release\bin\llvm-config.exe` to `C:\Program Files\LLVM\bin\llvm-config.exe`
* Open a new Powershell window and test accessiblity of `llvm-config.exe`, it should return with it's help message.

In your `cppast` based project, if you run into issues with cmake not finding libclang, set `LIBCLANG_LIBRARY` to be `C:/Program Files/LLVM/lib` in your CMakeLists.txt file.

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

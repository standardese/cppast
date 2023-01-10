load("@bazel_skylib//rules:copy_file.bzl", "copy_file")

package(default_visibility = ["//visibility:public"])

# Copy the LLVM clang-c files needed by cppast out of the llvm_toolchain and into this BUILD.
copy_file(name = "clang_Index",
          src = "@llvm_toolchain//:include/clang-c/Index.h",
          out = "Index.h"
)
copy_file(name = "clang_BuildSystem",
          src = "@llvm_toolchain//:include/clang-c/BuildSystem.h",
          out = "BuildSystem.h"
)
copy_file(name = "clang_CXErrorCode",
          src = "@llvm_toolchain//:include/clang-c/CXErrorCode.h",
          out = "CXErrorCode.h"
)
copy_file(name = "clang_ExternC",
          src = "@llvm_toolchain//:include/clang-c/ExternC.h",
          out = "ExternC.h"
)
copy_file(name = "clang_Platform",
          src = "@llvm_toolchain//:include/clang-c/Platform.h",
          out = "Platform.h"
)
copy_file(name = "clang_CXString",
          src = "@llvm_toolchain//:include/clang-c/CXString.h",
          out = "CXString.h"
)
copy_file(name = "clang_CXCompilationDatabase",
          src = "@llvm_toolchain//:include/clang-c/CXCompilationDatabase.h",
          out = "CXCompilationDatabase.h"
)

# Build all the of third party code needed by cppast.
cc_library(name = "third_party_deps",
           hdrs = ["Index.h","BuildSystem.h","CXErrorCode.h","ExternC.h","Platform.h", "CXString.h", "CXCompilationDatabase.h"],
           srcs = ["@llvm_toolchain//:lib"],
           includes = ["."],
           include_prefix = "clang-c",
           deps = [
            "@type_safe",
           ]
)

cc_library(
    name = "cppast",
    hdrs = glob(["include/**/*.hpp"]) + glob(["src/**/*.hpp"]) + glob(["external/**/*.hpp"]),
    srcs = glob(["src/**/*.cpp"]) + glob(["external/**/*.cpp"], exclude = ["external/tpl/process_win.cpp"]),
    includes = [".","include","src","src/libclang", "external/tpl"],
    copts = [
        "-DCPPAST_CLANG_BINARY='\"/bin/clang\"'",
        "-DCPPAST_VERSION_MAJOR='\"\"'",
        "-DCPPAST_VERSION_MINOR='\"\"'"
    ],
    deps = [
        ":third_party_deps"
    ]
)


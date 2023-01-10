load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "type_safe",
    hdrs = glob(["include/**/*.hpp"]) + glob(["external/**/*.hpp"]),
    includes = ["include","external/debug_assert"],
    visibility = ["//visibility:public"],
)



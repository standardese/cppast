load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def bazelSkylib():
    http_archive(
        name="bazel_skylib",
        sha256="d007737f4508c8fb9e5de09c33346cb5971f6f4a629210e4619c20a785452fd7" ,
        strip_prefix="bazel-skylib-2a89db4749d1aa860ea42ab50491cdc40d9a199a" ,
        urls = [
            "https://github.com/bazelbuild/bazel-skylib/archive/d007737f4508c8fb9e5de09c33346cb5971f6f4a629210e4619c20a785452fd7.tar.gz"
        ],
    )


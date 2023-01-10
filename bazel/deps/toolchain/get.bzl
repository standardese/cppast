load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def toolchain():
    http_archive(
        name="com_grail_bazel_toolchain" ,
        sha256="6bf2b30c8ab190eee44e1843f0cff457a5379250ad8239155dcf5bd38e4cc70f" ,
        strip_prefix="bazel-toolchain-f2d1ba2c9d713b2aa6e7063f6d11dd3d64aa402a" ,
        urls = [
            "https://github.com/Unilang/grailbio/archive/f2d1ba2c9d713b2aa6e7063f6d11dd3d64aa402a.tar.gz"
        ],
        patches = [
            "//deps/toolchain:p1.patch",
        ],
        patch_args = [
            "-p1",
        ],
    )


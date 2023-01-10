load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def aspectBazelLib():
    http_archive(
        name="aspect_bazel_lib" ,
        sha256="ca7cd1463ad994778b212ba704a8144c9a27c638e11480e859897e9f65e6496f" ,
        strip_prefix="bazel-lib-b1ee3675f28cc7089fb34b305d5ae09e77b76deb" ,
        urls = [
            "https://github.com/aspect-build/bazel-lib/archive/b1ee3675f28cc7089fb34b305d5ae09e77b76deb.tar.gz"
        ],
    )
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

def typeSafe():
    http_archive(
        name="type_safe" ,
        build_file="//deps/type_safe:build.BUILD" ,
        sha256="b1286d2b8598cdf6641887fa866c56abad47c464469cbe8002486db3eb8f051c" ,
        strip_prefix="type_safe-3612e2828b4b4e0d1cc689373e63a6d59d4bfd79" ,
        urls = [
            "https://github.com/foonathan/type_safe/archive/3612e2828b4b4e0d1cc689373e63a6d59d4bfd79.tar.gz"
        ],
    )


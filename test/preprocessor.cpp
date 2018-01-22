#include <catch.hpp>
#include <fstream>

#include "libclang/preprocessor.hpp"
#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("preprocessor_parses_escaped_character", "[!hide][clang4]")
{
    write_file("ppec.hpp", R"(
)");
    // This is an actual macro from the rapidjson source (reader.h)
    write_file("ppec.cpp", R"(
#define Z16 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
 static const char escape[256] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 0,'\"', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'/',
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,'\\', 0, 0, 0,
            0, 0,'\b', 0, 0, 0,'\f', 0, 0, 0, 0, 0, 0, 0,'\n', 0,
            0, 0,'\r', 0,'\t', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        };
#undef Z16

#include "ppec.hpp"
)");

    libclang_compile_config config;
    config.set_flags(cpp_standard::cpp_latest);

    auto&& preprocessed = detail::preprocess(config, "ppec.cpp", default_logger().get());
    REQUIRE(preprocessed.includes.size() == 1);
    REQUIRE(preprocessed.includes[0].file_name == "ppec.hpp");
}

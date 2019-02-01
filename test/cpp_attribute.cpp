// Copyright (C) 2017-2019 Jonathan Müller <jonathanmueller.dev@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#include <cppast/cpp_attribute.hpp>

#include <cppast/cpp_function.hpp>
#include <cppast/cpp_variable.hpp>

#include "test_parser.hpp"

using namespace cppast;

TEST_CASE("cpp_attribute")
{
    auto code = R"(
// multiple attributes
[[attribute1]] void [[attribute2]] a();
[[attribute1, attribute2]] void b();

// variadic attributes - not actually supported by clang
//[[variadic...]] void c();

// scoped attributes
[[ns::attribute]] void d();

// argument attributes
[[attribute(arg1, arg2, +(){}, 42, "Hello!")]] void e();

// all of the above
[[ns::attribute(+, -, 0 4), other_attribute]] void f();

// known attributes
[[deprecated]] void g();
[[maybe_unused]] void h();
[[nodiscard]] int i();
[[noreturn]] void j();

// alignas
struct alignas(8) type {};
alignas(type) int var;

// keyword attributes
[[const]] int k();

// multiple attributes but separately
[[a]] [[b]] [[c]] int l();
)";

    auto file = parse({}, "cpp_attribute.cpp", code);

    auto check_attribute
        = [](const cpp_attribute& attr, const char* name, type_safe::optional<std::string> scope,
             bool variadic, const char* args, cpp_attribute_kind kind) {
              REQUIRE(attr.kind() == kind);
              REQUIRE(attr.name() == name);
              REQUIRE(attr.scope() == scope);
              REQUIRE(attr.is_variadic() == variadic);

              if (attr.arguments())
                  REQUIRE(attr.arguments().value().as_string() == args);
              else
                  REQUIRE(*args == '\0');
          };

    auto count
        = test_visit<cpp_function>(*file,
                                   [&](const cpp_entity& e) {
                                       auto& attributes = e.attributes();
                                       REQUIRE(attributes.size() >= 1u);
                                       auto& attr = attributes.front();

                                       if (e.name() == "a" || e.name() == "b")
                                       {
                                           REQUIRE(attributes.size() == 2u);
                                           REQUIRE(has_attribute(e, "attribute1"));
                                           REQUIRE(has_attribute(e, "attribute2"));
                                           check_attribute(attr, "attribute1", type_safe::nullopt,
                                                           false, "", cpp_attribute_kind::unknown);
                                           check_attribute(attributes[1u], "attribute2",
                                                           type_safe::nullopt, false, "",
                                                           cpp_attribute_kind::unknown);
                                       }
                                       else if (e.name() == "c")
                                           check_attribute(attr, "variadic", type_safe::nullopt,
                                                           true, "", cpp_attribute_kind::unknown);
                                       else if (e.name() == "d")
                                       {
                                           REQUIRE(has_attribute(e, "ns::attribute"));
                                           check_attribute(attr, "attribute", "ns", false, "",
                                                           cpp_attribute_kind::unknown);
                                       }
                                       else if (e.name() == "e")
                                           check_attribute(attr, "attribute", type_safe::nullopt,
                                                           false, R"(arg1,arg2,+(){},42,"Hello!")",
                                                           cpp_attribute_kind::unknown);
                                       else if (e.name() == "f")
                                       {
                                           REQUIRE(attributes.size() == 2u);
                                           check_attribute(attr, "attribute", "ns", false,
                                                           "+,-,0 4", cpp_attribute_kind::unknown);
                                           check_attribute(attributes[1u], "other_attribute",
                                                           type_safe::nullopt, false, "",
                                                           cpp_attribute_kind::unknown);
                                       }
                                       else if (e.name() == "g")
                                       {
                                           REQUIRE(
                                               has_attribute(e, cpp_attribute_kind::deprecated));
                                           check_attribute(attr, "deprecated", type_safe::nullopt,
                                                           false, "",
                                                           cpp_attribute_kind::deprecated);
                                       }
                                       else if (e.name() == "h")
                                           check_attribute(attr, "maybe_unused", type_safe::nullopt,
                                                           false, "",
                                                           cpp_attribute_kind::maybe_unused);
                                       else if (e.name() == "i")
                                           check_attribute(attr, "nodiscard", type_safe::nullopt,
                                                           false, "",
                                                           cpp_attribute_kind::nodiscard);
                                       else if (e.name() == "j")
                                           check_attribute(attr, "noreturn", type_safe::nullopt,
                                                           false, "", cpp_attribute_kind::noreturn);
                                       else if (e.name() == "k")
                                           check_attribute(attr, "const", type_safe::nullopt, false,
                                                           "", cpp_attribute_kind::unknown);
                                       else if (e.name() == "l")
                                       {
                                           REQUIRE_NOTHROW(attributes.size() == 3);
                                           check_attribute(attributes[0], "a", type_safe::nullopt,
                                                           false, "", cpp_attribute_kind::unknown);
                                           check_attribute(attributes[1], "b", type_safe::nullopt,
                                                           false, "", cpp_attribute_kind::unknown);
                                           check_attribute(attributes[2], "c", type_safe::nullopt,
                                                           false, "", cpp_attribute_kind::unknown);
                                       }
                                   },
                                   false);
    REQUIRE(count == 11);

    count = test_visit<cpp_class>(*file,
                                  [&](const cpp_entity& e) {
                                      auto& attributes = e.attributes();
                                      REQUIRE(attributes.size() == 1u);
                                      auto& attr = attributes.front();
                                      check_attribute(attr, "alignas", type_safe::nullopt, false,
                                                      "8", cpp_attribute_kind::alignas_);
                                  },
                                  false);
    REQUIRE(count == 1u);

    count = test_visit<cpp_variable>(*file,
                                     [&](const cpp_entity& e) {
                                         auto& attributes = e.attributes();
                                         INFO(e.name());
                                         REQUIRE(attributes.size() == 1u);
                                         auto& attr = attributes.front();
                                         check_attribute(attr, "alignas", type_safe::nullopt, false,
                                                         "type", cpp_attribute_kind::alignas_);
                                     },
                                     false);
    REQUIRE(count == 1u);
}

TEST_CASE("cpp_attribute matching")
{
    auto code = R"(
// classes
struct [[a]] a {};
class [[b]] b {};

template <typename T>
class [[c]] c {};
template <typename T>
class [[c]] c<T*> {};
template <>
class [[c]] c<int> {};

// enums
enum [[e]] e {};
enum class [[f]] f
{
    a [[a]],
    b [[b]] = 42,
};

// functions
[[g]] void g();
void [[h]] h();
void i [[i]] ();
void j() [[j]];
auto k() -> int [[k]];

struct [[member_functions]] member_functions
{
    void a() [[a]];
    void b() const && [[b]];
    virtual void c() [[c]] final;
    virtual void d() [[d]] = 0;

    [[member_functions]] member_functions();
    member_functions(const member_functions&) [[member_functions]];
};

// variables
[[l]] const int l = 42;
static void* [[m]] m;

void [[function_params]] function_params
([[a]] int a, int [[b]] b, int c [[c]] = 42);

struct [[members]] members
{
    int [[a]] a;
    int [[b]] b : 2;
};

struct [[bases]] bases
: [[a]] public a,
  [[members]] members
{};

// namespace
namespace [[n]] n {}

// type aliases
using o [[o]] = int;

template <typename T>
using p [[p]] = T;

// constructor
struct [[q]] q
{
    [[q]] q();
};

struct [[r]] r
{
    [[r]]
    r();
};

// type defined inline
struct [[inline_type]] inline_type
{
    [[field]] int field;
}
[[s]] s;

int t [[t]];
)";

    auto file = parse({}, "cpp_attribute__matching.cpp", code);

    auto count = 0u;
    auto check = [&](const cppast::cpp_entity& e) {
        INFO(e.name());
        REQUIRE(e.attributes().size() == 1u);
        REQUIRE(e.attributes().begin()->name() == e.name());
        ++count;
    };

    visit(*file, [&](const cppast::cpp_entity& e, const cppast::visitor_info& info) {
        if (info.event != cppast::visitor_info::container_entity_exit
            && e.kind() != cppast::cpp_file::kind() && !is_friended(e) && !is_templated(e))
        {
            check(e);
            if (e.kind() == cppast::cpp_function::kind())
                for (auto& param : static_cast<const cppast::cpp_function&>(e).parameters())
                    check(param);
            else if (e.kind() == cppast::cpp_class::kind())
                for (auto& base : static_cast<const cppast::cpp_class&>(e).bases())
                    check(base);
        }

        return true;
    });
    REQUIRE(count == 44u);
}

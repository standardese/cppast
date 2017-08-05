#ifndef CPPAST_TEST_DETAIL_PARSER_SYNTAX_GENERATOR_HPP_INCLUDED
#define CPPAST_TEST_DETAIL_PARSER_SYNTAX_GENERATOR_HPP_INCLUDED

#include <vector>
#include "token_generator.hpp"

namespace cppast
{

namespace test
{

class syntax_generator
{
public:
private:
    cppast::test::token_generator token_generator;
};

} // namespace cppast::test

} // namespace cppast

#endif // CPPAST_TEST_DETAIL_PARSER_SYNTAX_GENERATOR_HPP_INCLUDED

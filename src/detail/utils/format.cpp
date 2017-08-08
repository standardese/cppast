// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_USE_FMT
#include <cppast/detail/utils/format.hpp>
#include <regex>

namespace cppast
{

namespace detail
{

namespace utils
{

namespace detail
{

std::string format_replace(const std::string& format_string, const std::initializer_list<std::string>& values)
{
    static std::regex placeholder_regex{"\\{[0-9]*\\}"};
    std::smatch match;
    std::size_t match_index = 0;
    std::ostringstream os;
    std::size_t prefix_begin = 0;
    enum class placeholder_mode
    {
        unknown,
        indexed,
        ordered
    };

    placeholder_mode mode = placeholder_mode::unknown;

    for(auto it = std::sregex_iterator(format_string.begin(), format_string.end(), placeholder_regex); it != std::sregex_iterator(); ++it)
    {
        auto match = *it;
        std::size_t index = match_index++;

        if(match[0].str() == "{}")
        {
            if(mode == placeholder_mode::unknown)
            {
                mode = placeholder_mode::ordered;
            }
            else if(mode == placeholder_mode::indexed)
            {
                throw cppast::detail::utils::bad_format{
                    "Using indexed placeholders, cannot mix with ordered placeholders"
                };
            }
        }
        else
        {
            if(mode == placeholder_mode::unknown)
            {
                mode = placeholder_mode::indexed;
            }
            else if(mode == placeholder_mode::ordered)
            {
                throw cppast::detail::utils::bad_format{
                    "Using ordered placeholders, cannot mix with indexed placeholders"
                };
            }
            else
            {
                auto index_str = match[0].str().substr(1, match[0].str().size() - 1);
                index = std::strtoull(index_str.c_str(), nullptr, 10);
            }
        }

        if(index >= values.size())
        {
            throw cppast::detail::utils::bad_format{
                "Placeholder index out of bounds (index: " + std::to_string(index) +
                ", total values: " + std::to_string(values.size()) + ")"
            };
        }

        os << format_string.substr(prefix_begin, (match[0].first - format_string.begin()) - prefix_begin);
        os << *(values.begin() + index);
        prefix_begin = match[0].second - format_string.begin();
    }

    if(prefix_begin < format_string.size())
    {
        os << format_string.substr(prefix_begin, format_string.size() - prefix_begin);
    }

    return os.str();
}

} // namespace cppast::detail::utils::detail

} // namespace cppast::detail::utils

} // namespace cppast::detail

} // namespace cppast
#endif // CPPAST_USE_FMT

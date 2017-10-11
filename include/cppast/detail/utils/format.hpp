// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_UTILS_FORMAT_HPP_INCLUDED
#define CPPAST_DETAIL_UTILS_FORMAT_HPP_INCLUDED

#ifdef CPPAST_USE_FMT
    #include <fmt/format.h>
    #include <fmt/ostream.h>
#else
    #include <sstream>
    #include <stdexcept>
    #include <initializer_list>
#endif // CPPAST_USE_FMT

namespace cppast
{

namespace detail
{

namespace utils
{

#ifdef CPPAST_USE_FMT

template<typename... Args>
std::string format(const std::string& str, Args&&... args)
{
    return fmt::format(str, std::forward<Args>(args)...);
}

#else // CPPAST_USE_FORMAT

class bad_format : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};

namespace detail
{

template<typename T>
std::string lexical_cast(const T& value)
{
    std::ostringstream os;
    os << value;
    return os.str();
}

std::string format_replace(const std::string& format_string, const std::initializer_list<std::string>& values);

}

template<typename... Args>
std::string format(const std::string& str, Args&&... args)
{
    if(sizeof...(Args) == 0)
    {
        return str;
    }

    return detail::format_replace(str, {detail::lexical_cast(std::forward<Args>(args))...});
}

#endif // CPPAST_USE_FORMAT

} // namespace cppast::detail::format

} // namespace cppast::detail

} // namespace cppast

#endif  // CPPAST_DETAIL_UTILS_FORMAT_HPP_INCLUDED

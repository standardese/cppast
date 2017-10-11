// Copyright (C) 2017 Manu Sanchez <Manu343726@gmail.com>
// This file is subject to the license terms in the LICENSE file
// found in the top-level directory of this distribution.

#ifndef CPPAST_DETAIL_UTILS_OVERLOADED_FUNCTION_INCLUDED
#define CPPAST_DETAIL_UTILS_OVERLOADED_FUNCTION_INCLUDED

namespace cppast
{

namespace detail
{

namespace utils
{

namespace
{
    template<typename...>
    struct typelist
    {
    };

    template<typename Functions>
    struct overloaded_function_t;

    template<typename Head, typename Second, typename... Tail>
    struct overloaded_function_t<typelist<Head, Second, Tail...>> : public Head, overloaded_function_t<typelist<Second, Tail...>>
    {
        constexpr overloaded_function_t(Head head, Second second, Tail... tail) :
            Head{head},
            overloaded_function_t<typelist<Second, Tail...>>{second, tail...}
        {}

        using overloaded_function_t<typelist<Second, Tail...>>::operator();
        using Head::operator();
    };

    template<typename Head>
    struct overloaded_function_t<typelist<Head>> : public Head
    {
        constexpr overloaded_function_t(Head head) :
            Head{head}
        {}

        using Head::operator();
    };
}

template<typename... Functions>
constexpr overloaded_function_t<typelist<Functions...>> overloaded_function(Functions... functions)
{
    return { functions... };
}

} // namespace cppast::detail::utils

} // namespace cppast::detail

} // namespace cppast

#endif // CPPAST_DETAIL_UTILS_OVERLOADED_FUNCTION_INCLUDED

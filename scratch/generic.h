#pragma once

namespace tinysimd {
namespace detail {

    template <typename T, typename N>
    struct generic {};

} // namespace detail

namespace abi {

    template <typename T, unsigned N>
    struct generic { using type = detail::generic<T, N>; };

} // namespace abi
} // namespace tinysimd

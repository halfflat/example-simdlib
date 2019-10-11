#pragma once

namespace tinysimd {
namespace abi {

    template <typename T, unsigned N> struct avx2 { using type = void; };

} // namespace abi
} // namespace tinysimd

#if defined(__AVX2__) && defined(__FMA__)

namespace tinysimd {

struct avx2_int4 {};
struct avx2_double4 {};

namespace abi {

    template<> struct avx2<int, 4> { using type = avx2_int4; };
    template<> struct avx2<double, 4> { using type = avx2_double4; };

} // namespace abi
} // namespace tinysimd

#endif // defined(__AVX2__) && defined(__FMA__)

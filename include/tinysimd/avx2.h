#pragma once

#include <immintrin.h>
#include <cstdint>
#include <tinysimd/common.h>

namespace tinysimd {

namespace abi {

template <typename T, unsigned N> struct avx2 { using type = void; };

} // namespace abi


#if defined(__AVX2__) && defined(__FMA__)

struct avx2_double4;
template <> struct simd_traits<avx2_double4> {
    static constexpr unsigned width = 4;
    using scalar_type = double;
    using vector_type = __m256d;
};

struct avx2_int4;
template <> struct simd_traits<avx2_int4> {
    static constexpr unsigned width = 4;
    using scalar_type = std::int32_t;
    using vector_type = __m128i;
};

namespace abi {

template <> struct avx2<int, 4> { using type = avx2_int4; };
template <> struct avx2<double, 4> { using type = avx2_double4; };

} // namespace abi

struct avx2_double4: fallback<avx2_double4> {
    static void copy_to(const __m256d& v, double* p) {
        _mm256_storeu_pd(p, v);
    }
    static __m256d copy_from(const double* p) {
        return _mm256_loadu_pd(p);
    }
    static __m256d broadcast(double v) {
        return _mm256_set1_pd(v);
    }
    static __m256d add(const __m256d& a, const __m256d& b) {
        return _mm256_add_pd(a, b);
    }
    static __m256d mul(const __m256d& a, const __m256d& b) {
        return _mm256_mul_pd(a, b);
    }
    static __m256d fma(const __m256d& u, const __m256d& v, const __m256d& w) {
        return _mm256_fmadd_pd(u, v, w);
    }

    using fallback<avx2_double4>::gather;
    static __m256d gather(tag<avx2_int4>, const double* p, const __m128i& index) {
        return _mm256_i32gather_pd(p, index, 8);
    }
};

struct avx2_int4: fallback<avx2_int4> {
    using i32 = std::int32_t;

    static void copy_to(const __m128i& v, i32* p) {
        _mm_storeu_si128((__m128i*)p, v);
    }
    static __m128i copy_from(const i32* p) {
        return _mm_loadu_si128((const __m128i*)p);
    }
    static __m128i broadcast(i32 v) {
        return _mm_set1_epi32(v);
    }
    static __m128i add(const __m128i& a, const __m128i& b) {
        return _mm_add_epi32(a, b);
    }
    static __m128i mul(const __m128i& a, const __m128i& b) {
        return _mm_mul_epi32(a, b);
    }
    static __m128i fma(const __m128i& u, const __m128i& v, const __m128i& w) {
        return add(mul(u, v), w);
    }

    using fallback<avx2_int4>::gather;
    static __m128i gather(tag<avx2_int4>, const i32* p, const __m128i& index) {
        return _mm_i32gather_epi32(p, index, 4);
    }
};

#endif // defined(__AVX2__) && defined(__FMA__)

} // namespace tinysimd

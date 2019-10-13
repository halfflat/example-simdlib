#include <gtest/gtest.h>
#include <tinysimd/simd.h>

template <typename X> struct tinysimd_test: public ::testing::Test {};
TYPED_TEST_SUITE_P(tinysimd_test);

TYPED_TEST_P(tinysimd_test, copy) {
    ASSERT_TRUE(true);
}

REGISTER_TYPED_TEST_SUITE_P(tinysimd_test, copy);
typedef ::testing::Types<tinysimd::avx2_double4, tinysimd::avx2_int4, tinysimd::generic<float, 4>> simd_test_types;
INSTANTIATE_TYPED_TEST_SUIT_P(tinysimd_test, simd_test_types);

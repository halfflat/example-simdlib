#pragma once

#include <type_traits>
#include <iterator>
#include <algorithm>
#include <random>
#include <stdexcept>
#include <string>

#include <tinysimd/simd.h>

static std::minstd_rand R;

namespace tinysimd {
inline std::string to_string(tinysimd::constraint c) {
    using namespace tinysimd;
    switch (c) {
    case constraint::none: return "none";
    case constraint::constant: return "constant";
    case constraint::independent: return "independent";
    case constraint::contiguous: return "contiguous";
    case constraint::monotonic: return "monotonic";
    default: return "unknown";
    }
}
}

template <typename Container>
void generate_test_data(Container& data) {
    using std::begin;
    using std::end;
    using scalar = std::decay_t<decltype(*begin(data))>;
    std::uniform_int_distribution<unsigned> U(0, 99);

    std::generate(begin(data), end(data), [&] { return scalar(U(R)/scalar{16}); });
}

template <typename Container>
void generate_constrained_indices(Container& data, unsigned upto, tinysimd::constraint c) {
    using std::begin;
    using std::end;
    using scalar = std::decay_t<decltype(*begin(data))>;

    switch (c) {
    case tinysimd::constraint::constant:
        {
            std::uniform_int_distribution<scalar> U(0, upto-1);
            std::fill(begin(data), end(data), U(R));
        }
        return;

    case tinysimd::constraint::contiguous:
        {
            unsigned n = std::distance(begin(data), end(data));
            if (n>=upto) throw std::out_of_range("upto too small");

            std::uniform_int_distribution<scalar> U(0, upto-n);
            std::iota(begin(data), end(data), U(R));
        }
        return;

    case tinysimd::constraint::independent:
        {
            // Simple reservoir sampling.

            unsigned n = std::distance(begin(data), end(data));
            if (n>=upto) throw std::out_of_range("upto too small");

            std::vector<scalar> pool(n);

            for (unsigned i = 0; i<n; ++i) {
                pool[i] = i;
            }

            std::uniform_real_distribution<float> U;
            std::uniform_int_distribution<unsigned> S(0u, n-1);

            const float p = (float)n/upto;
            for (unsigned i = n; i<upto; ++i) {
                if (U(R)>p) pool[S(R)] = i;
            }

            std::copy(begin(pool), end(pool), begin(data));
        }
        return;

    case tinysimd::constraint::none:
        {
            std::uniform_int_distribution<scalar> U(0, upto-1);
            std::generate(begin(data), end(data), [&] { return U(R); });
        }
        return;

    case tinysimd::constraint::monotonic:
        {
            std::uniform_int_distribution<scalar> U(0, upto-1);
            std::generate(begin(data), end(data), [&] { return U(R); });
            std::sort(begin(data), end(data));
        }
        return;
    }
}

template <typename Container>
void fill_zero(Container& c) {
    using std::begin;
    using std::end;
    using scalar = std::decay_t<decltype(*begin(c))>;

    std::fill(begin(c), end(c), scalar(0));
}

static auto sequence_equal = [](const auto& s1, const auto& s2) {
    using std::begin;
    using std::end;

    return std::equal(begin(s1), end(s1), begin(s2), end(s2));
};


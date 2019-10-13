#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>

namespace tinysimd {

template <typename I>
struct simd_traits {
    static constexpr unsigned width = 0;
    using scalar_type = void;
    using vector_type = void;
};

enum class constraint {
    none, independent, monotonic, contiguous, constant
};

template <typename I> struct tag {};

template <typename I>
struct fallback {
    static constexpr unsigned width = simd_traits<I>::width;
    using scalar_type = typename simd_traits<I>::scalar_type;
    using vector_type = typename simd_traits<I>::vector_type;
    using store = scalar_type[width];

    static vector_type broadcast(scalar_type x) {
        store a;
        std::fill(std::begin(a), std::end(a), x);
        return I::copy_from(a);
    }

    static vector_type add(const vector_type& u, const vector_type& v) {
        store a, b, r;
        I::copy_to(u, a);
        I::copy_to(v, b);
        for (unsigned i = 0; i<width; ++i) r[i] = a[i]+b[i];
        return I::copy_from(r);
    }

    static vector_type mul(const vector_type& u, const vector_type& v) {
        store a, b, r;
        I::copy_to(u, a);
        I::copy_to(v, b);
        for (unsigned i = 0; i<width; ++i) r[i] = a[i]*b[i];
        return I::copy_from(r);
    }

    static vector_type fma(const vector_type& u, const vector_type& v, const vector_type& w) {
        store a, b, c, r;
        I::copy_to(u, a);
        I::copy_to(v, b);
        I::copy_to(w, c);
        for (unsigned i = 0; i<width; ++i) r[i] = std::fma(a[i], b[i], c[i]);
        return I::copy_from(r);
    }

    static scalar_type element(const vector_type& u, unsigned i) {
        store a;
        I::copy_to(u, a);
        return a[i];
    }

    static void set_element(vector_type& u, unsigned i, const scalar_type& x) {
        store a;
        I::copy_to(u, a);
        a[i] = x;
        u = I::copy_from(a);
    }

    static scalar_type reduce_add(const vector_type& u) {
        store a;
        I::copy_to(u, a);
        return std::accumulate(std::begin(a), std::end(a), scalar_type{0});
    }

    template <typename IndexI>
    static vector_type gather(tag<IndexI>, const scalar_type* __restrict p,
                              const typename simd_traits<IndexI>::vector_type& index) {
        store a;
        for (unsigned i = 0; i<width; ++i) a[i] = p[index[i]];
        return I::copy_from(a);
    }

    template <typename IndexI>
    static void scatter(tag<IndexI>, const vector_type& u, scalar_type* __restrict p,
                        const typename simd_traits<IndexI>::vector_type& index) {
        store a;
        I::copy_to(u, a);
        for (unsigned i = 0; i<width; ++i) p[index[i]] = a[i];
    }

    template <typename IndexI>
    static vector_type gather(tag<IndexI>, const scalar_type* __restrict p,
                              const typename simd_traits<IndexI>::vector_type& index, constraint c) {
        switch (c) {
        case constraint::none:
        case constraint::monotonic:
        case constraint::independent:
            return I::gather(tag<IndexI>{}, p, index);

        case constraint::contiguous:
            return I::copy_from(p+IndexI::element(index, 0));

        case constraint::constant:
            return I::broadcast(p[IndexI::element(index, 0)]);
        }
    }

    template <typename IndexI>
    static void scatter(tag<IndexI>, const vector_type& u, scalar_type* __restrict p,
                              const typename simd_traits<IndexI>::vector_type& index, constraint c) {

        switch (c) {
        case constraint::none:
        case constraint::monotonic:
        case constraint::independent:
            I::scatter(tag<IndexI>{}, u, p, index);
            return;

        case constraint::contiguous:
            I::copy_to(u, p+IndexI::element(index, 0));
            return;

        case constraint::constant:
            p[IndexI::element(index, 0)] = I::element(u, width-1);
            return;
        }
    }

    template <typename IndexI>
    static void scatter_add(tag<IndexI>, const vector_type& u, scalar_type* __restrict p,
                            const typename simd_traits<IndexI>::vector_type& index, constraint c) {

        using index_store = typename simd_traits<IndexI>::scalar_type[width];

        switch (c) {
        case constraint::none:
            {
                store a;
                I::copy_to(u, a);

                index_store j;
                IndexI::copy_to(index, j);

                for (unsigned i = 0; i<width; ++i) p[j[i]] += a[i];
            }
            break;

        case constraint::monotonic:
            {
                store a;
                I::copy_to(u, a);

                index_store j;
                IndexI::copy_to(index, j);

                scalar_type sum = 0;
                for (unsigned i = 0; i<width-1; ++i) {
                    sum += a[i];
                    if (j[i]!=j[i+1]) {
                        p[j[i]] += sum;
                        sum = 0;
                    }
                }
                sum += a[width-1];
                p[j[width-1]] += sum;
            }
            break;

        case constraint::independent:
            I::scatter(tag<IndexI>{}, I::add(u, I::gather(tag<IndexI>{}, p, index, c)), p, index, c);
            break;

        case constraint::contiguous:
            p += IndexI::element(index, 0);
            I::copy_to(I::add(u, I::copy_from(p)), p);
            break;

        case constraint::constant:
            p[IndexI::element(index, 0)] += I::reduce_add(u);
            break;
        }
    }
};

} // namespace tinysimd

#include <memory>
#include <random>

#include <iostream>
#include <vector>

#include <benchmark/benchmark.h>
#include <tinysimd/simd.h>

#define N 32768

using namespace tinysimd;

[[gnu::noinline]]
void sq_scatter_add(double* __restrict out, const double* a, const int* index) {
    for (unsigned i = 0; i<N; ++i) {
        out[index[i]] += a[i]*a[i];
    }
}

template <unsigned width>
[[gnu::noinline]]
void sq_scatter_add_simd(double* __restrict out, const double* a, const int* index, constraint c = constraint::none) {
    using vdouble = simd<double, width>;
    using vint = simd<int, width>;

    for (unsigned i = 0; i<N; i += width) {
        vdouble va(a+i);
        vint vindex(index+i);

        indirect(out, vindex, c) += va*va;
    }
}

template <unsigned width>
[[gnu::noinline]]
void sq_scatter_add_simd_subset(double* __restrict out, const double* a,
                                unsigned subset_size,
                                const int* offset, const int* index, constraint c) {
    using vdouble = simd<double, width>;
    using vint = simd<int, width>;

    for (unsigned i = 0; i<subset_size; ++i) {
        vdouble va(a+offset[i]);
        vint vindex(index+offset[i]);

        indirect(out, vindex, c) += va*va;
    }
}

std::unique_ptr<double[]> random_array(unsigned n) {
    std::minstd_rand R;
    std::uniform_real_distribution<double> U;

    std::unique_ptr<double[]> p(new double[n]);
    std::generate(p.get(), p.get()+n, [&] { return U(R); });
    return p;
}

std::unique_ptr<int[]> random_monotonic_index(unsigned n, unsigned upto) {
    std::minstd_rand R;
    std::uniform_int_distribution<int> U(0, upto-1);

    std::unique_ptr<int[]> p(new int[n]);
    std::generate(p.get(), p.get()+n, [&] { return U(R); });
    std::sort(p.get(), p.get()+n);
    return p;
}

struct test_data {
    std::unique_ptr<double[]> out;
    std::unique_ptr<double[]> a;
    std::unique_ptr<int[]> index;
};

test_data generate_test_data(float density) {
    unsigned data_size = N/density;
    return {random_array(data_size), random_array(N), random_monotonic_index(N, data_size)};
}

void bench_sq_scatter_add(benchmark::State& state) {
    test_data D = generate_test_data(state.range(0)/100.f);

    for (auto _: state) {
        sq_scatter_add(D.out.get(), D.a.get(), D.index.get());
    }
}

void bench_sq_scatter_add_simd(benchmark::State& state) {
    test_data D = generate_test_data(state.range(0)/100.f);

    for (auto _: state) {
        sq_scatter_add_simd<4>(D.out.get(), D.a.get(), D.index.get());
    }
}

void bench_sq_scatter_add_preconstrain(benchmark::State& state) {
    constexpr unsigned width = 4;

    test_data D = generate_test_data(state.range(0)/100.f);
    std::unique_ptr<constraint[]> pre(new constraint[N/width]);

    std::vector<int> constant_offsets;
    std::vector<int> monotonic_offsets;
    std::vector<int> independent_offsets;

    const int* indices = D.index.get();
    for (unsigned i = 0; i<N; i+=width) {
        if (indices[i]==indices[i+width-1]) constant_offsets.push_back(i);
        else {
            bool indep = true;
            for (unsigned j = 0; j<width-1; ++j) indep &= indices[i+j]!=indices[i+j+1];
            if (indep) independent_offsets.push_back(i);
            else monotonic_offsets.push_back(i);
        }
    }

    for (auto _: state) {
        sq_scatter_add_simd_subset<width>(D.out.get(), D.a.get(), constant_offsets.size(),
            constant_offsets.data(), D.index.get(), constraint::constant);
        sq_scatter_add_simd_subset<width>(D.out.get(), D.a.get(), independent_offsets.size(),
            independent_offsets.data(), D.index.get(), constraint::independent);
        sq_scatter_add_simd_subset<width>(D.out.get(), D.a.get(), monotonic_offsets.size(),
            monotonic_offsets.data(), D.index.get(), constraint::monotonic);
    }
}

constexpr int sparse = 10;
constexpr int moderate = 100;
constexpr int dense = 1000;

BENCHMARK(bench_sq_scatter_add)->Arg(sparse)->Arg(moderate)->Arg(dense);
BENCHMARK(bench_sq_scatter_add_simd)->Arg(sparse)->Arg(moderate)->Arg(dense);
BENCHMARK(bench_sq_scatter_add_preconstrain)->Arg(sparse)->Arg(moderate)->Arg(dense);
BENCHMARK_MAIN();


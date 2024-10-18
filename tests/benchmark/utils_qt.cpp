/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <benchmark/benchmark.h>

static void StubBenchmark(benchmark::State& state)
{
    int i = 0;

    while (state.KeepRunning())
        i++;

    (void)i;
}

BENCHMARK(StubBenchmark);

BENCHMARK_MAIN();

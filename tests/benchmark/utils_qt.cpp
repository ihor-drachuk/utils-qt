/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <benchmark/benchmark.h>

#include <UtilsQt/datatostring.h>

static void StubBenchmark(benchmark::State& state)
{
    int i = 0;

    while (state.KeepRunning())
        i++;

    (void)i;
}

BENCHMARK(StubBenchmark);


static void Benchmark_dataToString(benchmark::State& state)
{
    std::string someData = "1234My567Data12";
    someData[0] = 1;
    someData[1] = 2;
    someData[2] = 3;
    someData[3] = 4;
    someData[6] = 5;
    someData[7] = 6;
    someData[8] = 7;
    someData[13] = 8;
    someData[14] = 9;

    while (state.KeepRunning())
        dataToString(someData.data(), someData.size());
}

BENCHMARK(Benchmark_dataToString);

BENCHMARK_MAIN();

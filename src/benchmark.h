#include "heap_storage.h"
#include <lmdb.h>
#include <vector>

#include <chrono>
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;
using TimePoint = std::chrono::steady_clock::time_point;
using TimeSpan = std::chrono::duration<double>;

// Define the types used in the benchmark
typedef BTFile BenchFile;
typedef SlottedPage BenchPage;

class Benchmark {
public:
    Benchmark() = delete;

    static std::vector<MDB_val*> block_data;
    static void run(std::string filename = "__benchmark.db");

    static TimeSpan read_test(BenchFile &file, size_t n);
    static TimeSpan write_test(BenchFile &file, size_t n);

protected:
    static std::vector<BenchPage*> *init_pages(BenchFile &file, size_t n);
    static void free_pages(std::vector<BenchPage*> *pages);
};

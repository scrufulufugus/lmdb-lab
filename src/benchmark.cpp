#include "benchmark.h"
#include "heap_storage.h"
#include <cassert>
#include <cstddef>
#include <string>
#include <vector>

std::vector<Dbt*> Benchmark::block_data;;

void Benchmark::run(std::string filename) {

    size_t n[] = {1000, 10000, 100000, 1000000, 10000000};
    BenchFile *file;

    if (block_data.empty()) {
        std::string *data = new std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        for (size_t i = 0; i < 133; i++) {
            block_data.push_back(new Dbt((void *)data->c_str(), data->size()));
        }
    }
    printf("n,type,seconds\n");

    for (size_t i = 0; i < 5; i++) {
        file = new BenchFile(filename);
        file->create();
        printf("%lu,write,%f\n", n[i], write_test(*file, n[i]).count());
        file->close();
        delete file;
        file = new BenchFile(filename);
        file->open();
        printf("%lu,read,%f\n", n[i], write_test(*file, n[i]).count());
        file->drop();
        delete file;
    }

    for (auto data : block_data) {
        delete data;
    }
    block_data.clear();
}

TimeSpan Benchmark::write_test(BenchFile &file, size_t n) {
    std::vector<BenchPage*> *pages = init_pages(file, n);

    // Begin benchmark
    TimePoint start_time = steady_clock::now();

    for (BenchPage *page : *pages) {
        for (const Dbt *data : block_data) {
            page->add(data);
        }
    }

    // End benchmark
    TimePoint end_time = steady_clock::now();

    free_pages(pages);
    TimeSpan span = duration_cast<TimeSpan>(end_time - start_time);
    return span;
}

TimeSpan Benchmark::read_test(BenchFile &file, size_t n) {
    std::vector<BenchPage*> *pages = init_pages(file, n);

    // Begin benchmark
    TimePoint start_time = steady_clock::now();

    for (BenchPage *page : *pages) {
        RecordIDs *ids = page->ids();
        for (size_t i = 0; i < ids->size(); i++) {
            Dbt *data = page->get((*ids)[i]);
            // Assert that the data is the same as the data we put in
            assert(memcmp(data->mv_data, block_data[i]->mv_data, data->mv_size) == 0);
            delete data;
        }
        delete ids;
    }

    // End benchmark
    TimePoint end_time = steady_clock::now();

    free_pages(pages);
    TimeSpan span = duration_cast<TimeSpan>(end_time - start_time);
    return span;
}

std::vector<BenchPage*> *Benchmark::init_pages(BenchFile &file, size_t n) {
    std::vector<BenchPage*> *pages = new std::vector<BenchPage*>();
    for (size_t i = 0; i < n; i++) {
        BenchPage *page = file.get_new();
        pages->push_back(page);
    }
    return pages;
}

void Benchmark::free_pages(std::vector<BenchPage*> *pages) {
    for (auto page : *pages) {
        delete page;
    }
    delete pages;
}

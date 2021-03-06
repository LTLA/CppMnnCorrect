#ifndef CUSTOM_PARALLEL_H
#define CUSTOM_PARALLEL_H

#include <cmath>
#include <vector>
#include <thread>

template<class Function>
void parallelize(size_t n, Function f) {
    size_t jobs_per_worker = std::ceil(static_cast<double>(n) / 3);
    size_t start = 0;
    std::vector<std::thread> jobs;
    
    for (size_t w = 0; w < 3; ++w) {
        size_t end = std::min(n, start + jobs_per_worker);
        if (start >= end) {
            break;
        }
        jobs.emplace_back(f, start, end);
        start += jobs_per_worker;
    }

    for (auto& job : jobs) {
        job.join();
    }
}

#define MNNCORRECT_CUSTOM_PARALLEL parallelize
#endif

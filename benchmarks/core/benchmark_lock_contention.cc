#include <nanobench.h>
#include "../benchmark_helpers.hh"

namespace musac::benchmark {

void register_lock_contention_benchmarks(ankerl::nanobench::Bench& bench) {
    bench.title("Lock Contention");
    // TODO: Implement lock contention benchmarks
}

} // namespace musac::benchmark
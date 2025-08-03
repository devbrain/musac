//
// Created by igor on 4/28/25.
//

#include "musac/in_use_guard.hh"

namespace musac {
    in_use_guard::in_use_guard(std::atomic<uint32_t>& c)
        : counter(c), usage_mutex(nullptr), usage_cv(nullptr), valid(true) { 
        counter.fetch_add(1, std::memory_order_acquire); 
    }
    
    in_use_guard::in_use_guard(std::atomic<uint32_t>& c, std::mutex* m, std::condition_variable* cv, bool v)
        : counter(c), usage_mutex(m), usage_cv(cv), valid(v) {
        if (valid) {
            counter.fetch_add(1, std::memory_order_acquire);
        }
    }

    in_use_guard::~in_use_guard() { 
        if (valid) {
            if (counter.fetch_sub(1, std::memory_order_release) == 1) {
                // We were the last user, notify if we have the cv
                if (usage_cv && usage_mutex) {
                    std::lock_guard<std::mutex> lock(*usage_mutex);
                    usage_cv->notify_all();
                }
            }
        }
    }
}
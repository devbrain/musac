//
// Created by igor on 4/28/25.
//

#pragma once
#include <cstdint>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace musac {
    struct in_use_guard {
        std::atomic<uint32_t>& counter;
        std::mutex* usage_mutex;
        std::condition_variable* usage_cv;
        bool valid;

        explicit in_use_guard(std::atomic<uint32_t>& c);
        in_use_guard(std::atomic<uint32_t>& c, std::mutex* m, std::condition_variable* cv, bool v);
        ~in_use_guard();
        
        operator bool() const { return valid; }
    };
}





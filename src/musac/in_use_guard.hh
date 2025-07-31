//
// Created by igor on 4/28/25.
//

#pragma once
#include <cstdint>
#include "atomic"
namespace musac {
    struct InUseGuard {
        std::atomic <uint32_t>& counter;

        explicit InUseGuard(std::atomic <uint32_t>& c);

        ~InUseGuard();
    };
}





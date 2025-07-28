//
// Created by igor on 4/28/25.
//

#include "in_use_guard.hh"

namespace musac {
    InUseGuard::InUseGuard(std::atomic <uint32_t>& c)
        : counter(c) { counter.fetch_add(1); }

    InUseGuard::~InUseGuard() { counter.fetch_sub(1); }
}

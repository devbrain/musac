//
// Created by igor on 3/17/25.
//

#include "musac/mutex.hh"
namespace musac {
    mutex::mutex()
        : m_mutex(SDL_CreateMutex()) {
    }

    mutex::~mutex() {
        SDL_DestroyMutex(m_mutex);
    }

    void mutex::lock() {
        SDL_LockMutex(m_mutex);
    }

    void mutex::unlock() {
        SDL_UnlockMutex(m_mutex);
    }
}

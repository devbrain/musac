//
// Created by igor on 3/17/25.
//

#ifndef  MUTEX_HH
#define  MUTEX_HH

#include <SDL3/SDL.h>

namespace musac {
    class mutex {
        public:
            mutex();
            ~mutex();

            mutex(const mutex&) = delete;
            mutex& operator =(const mutex&) = delete;

            void lock();
            void unlock();

        private:
            SDL_Mutex* m_mutex;
    };
}

#endif

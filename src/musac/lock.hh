//
// Created by igor on 3/17/25.
//

#ifndef LOCK_HH
#define LOCK_HH

namespace musac {
    template<typename Lockable>
    struct scoped_lock {
        scoped_lock(Lockable& obj)
            : m_obj(obj) {
            m_obj.lock();
        }

        ~scoped_lock() {
            m_obj.unlock();
        }

        private:
            Lockable& m_obj;
    };
}

#endif

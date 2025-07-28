#include "callback_dispatcher.hh"

namespace musac {
    CallbackDispatcher& CallbackDispatcher::instance() {
        static CallbackDispatcher inst;
        return inst;
    }

    void CallbackDispatcher::enqueue(const callback_t& cbk) {
        std::lock_guard <std::mutex> lk(m_mutex);
        m_queue.emplace_back(cbk);
    }

    void CallbackDispatcher::dispatch() {
        // 1) snapshot & clear under lock
        std::vector <callback_t> toDispatch; {
            std::lock_guard <std::mutex> lk(m_mutex);
            toDispatch.assign(m_queue.begin(), m_queue.end());
            m_queue.clear();
        }

        // 2) invoke each on the main thread
        for (auto& [token, cbk] : toDispatch) {
            cbk();
        }
    }

    void CallbackDispatcher::cleanup(int token) {
        std::lock_guard <std::mutex> lk(m_mutex);
        auto it = m_queue.begin();
        while (it != m_queue.end()) {
            if (it->first == token) {
                it = m_queue.erase(it);
            } else {
                ++it;
            }
        }
    }

    CallbackDispatcher::CallbackDispatcher() = default;
    CallbackDispatcher::~CallbackDispatcher() = default;
}

//
// Created by igor on 4/28/25.
//

#pragma once

#include <deque>
#include <mutex>
#include <functional>
#include <utility>

namespace musac {

    /**
     * CallbackDispatcher queues finish and loop callbacks from the audio thread
     * and dispatches them safely on the main thread in response to SDL events.
     */
    class callback_dispatcher {
        public:
            using callback_t = std::pair<int, std::function <void()>>;

            callback_dispatcher(const callback_dispatcher&) = delete;
            callback_dispatcher& operator=(const callback_dispatcher&) = delete;
            /**
             * Get the singleton instance of the dispatcher.
             */
            static callback_dispatcher& instance();

            /**
             * Enqueue a callback event from the audio thread.
             */
            void enqueue(const callback_t& cbk);

            void dispatch();
            void cleanup(int token);
        private:
            callback_dispatcher();
            ~callback_dispatcher();


            std::deque<callback_t> m_queue;
            std::mutex             m_mutex;
    };

} // namespace musac

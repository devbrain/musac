//
// Created by igor on 4/28/25.
//

#pragma once

#include <deque>
#include <mutex>
#include <functional>
#include <utility>
#include <SDL3/SDL.h>

namespace musac {

    /**
     * CallbackDispatcher queues finish and loop callbacks from the audio thread
     * and dispatches them safely on the main thread in response to SDL events.
     */
    class CallbackDispatcher {
        public:
            using callback_t = std::pair<int, std::function <void()>>;

            CallbackDispatcher(const CallbackDispatcher&) = delete;
            CallbackDispatcher& operator=(const CallbackDispatcher&) = delete;
            /**
             * Get the singleton instance of the dispatcher.
             */
            static CallbackDispatcher& instance();

            /**
             * Enqueue a callback event from the audio thread.
             */
            void enqueue(const callback_t& cbk);

            void dispatch();
            void cleanup(int token);
        private:
            CallbackDispatcher();
            ~CallbackDispatcher();


            std::deque<callback_t> m_queue;
            std::mutex             m_mutex;
    };

} // namespace musac

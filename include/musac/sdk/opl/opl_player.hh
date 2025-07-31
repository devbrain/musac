//
// Created by igor on 3/20/25.
//

#ifndef  OPL_PLAYER_HH
#define  OPL_PLAYER_HH

#include <vector>

#include <musac/sdk/opl/opl.hh>
#include <musac/sdk/opl/opl_command.h>
#include <musac/sdk/export_musac_sdk.h>

namespace musac {
    class MUSAC_SDK_EXPORT opl_player {
        public:
            explicit opl_player(unsigned int rate);
            ~opl_player();

            void copy(const opl_command* commands, std::size_t size);
            void copy(std::vector<opl_command>&& commands);

            unsigned int render(float* buffer, unsigned int len); // decoder interface
            void rewind();
        private:
            std::size_t do_render(int16_t* buffer, std::size_t sample_pairs);

            class commands_queue {
                public:
                    commands_queue();
                    void copy(const opl_command* commands, std::size_t size);
                    void copy(std::vector<opl_command>&& commands);
                    [[nodiscard]] bool empty() const;
                    [[nodiscard]] const opl_command& top() const;
                    void pop();
                    void rewind();
                private:
                    std::vector <opl_command> m_commands;
                    std::size_t m_top;
            };

        private:
            enum state_t {
                INITIAL_STATE,
                REMAINS_STATE
            };

        private:
            const unsigned int m_rate;
            opl m_proc;
            commands_queue m_queue;
            state_t m_state;
            double m_time;
            std::size_t m_sample_remains;
    };
}

#endif

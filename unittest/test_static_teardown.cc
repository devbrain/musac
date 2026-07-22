// Static-teardown regression test (standalone binary, NOT part of
// musac_unittest — the scenario under test happens after main() returns).
//
// Verifies the guarantee established by commit "Fix static-destruction-order
// double-free in audio teardown" and the immortal-globals follow-up: a client
// may keep an audio_stream / audio_device in its own statics and call
// audio_system::done() from a late static destructor, and none of that may
// touch a destroyed musac global (mixer, callback mutex, shared device data,
// device-layer backend/mutexes, system holders).
//
// Two teardown hooks exercise the scenario:
// - portable (all compilers, incl. MSVC): a static object declared before
//   the cached device/stream statics in this TU, so it is destroyed after
//   them — its destructor calls done(), the documented scenario exactly;
// - GCC/Clang only: an __attribute__((destructor)) probe in .fini_array,
//   which glibc runs after ALL C++ static destructors — an even later,
//   harsher ordering (done() is idempotent, so the second call is safe).
//
// Success criterion: exit code 0. Run under ASan to catch what the OS would
// forgive at exit.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>

#include <musac/audio_device.hh>
#include <musac/audio_source.hh>
#include <musac/audio_system.hh>
#include <musac/sdk/decoder.hh>
#include <musac/sdk/io_stream.hh>
#include <musac/stream.hh>

#include "mock_backends.hh"

namespace {

    class silence_decoder final : public musac::decoder {
        public:
            [[nodiscard]] const char* get_name() const override { return "silence"; }
            void open(musac::io_stream* /*rwops*/) override { set_is_open(true); }
            [[nodiscard]] musac::channels_t get_channels() const override { return 2; }
            [[nodiscard]] musac::sample_rate_t get_rate() const override { return 44100; }
            bool rewind() override { m_pos = 0; return true; }
            [[nodiscard]] std::chrono::microseconds duration() const override {
                return std::chrono::seconds(1);
            }
            bool seek_to_time(std::chrono::microseconds /*pos*/) override { return true; }

        protected:
            size_t do_decode(float* buf, size_t len, bool& call_again) override {
                constexpr size_t total = 44100 * 2;
                size_t n = std::min(len, total - std::min(m_pos, total));
                std::fill_n(buf, n, 0.0f);
                m_pos += n;
                call_again = m_pos < total;
                return n;
            }

        private:
            size_t m_pos = 0;
    };

    // Portable teardown hook: within one TU, statics are constructed in
    // declaration order and destroyed in exactly reverse order (guaranteed by
    // the standard on every compiler, including MSVC). Declared FIRST, so it
    // is destroyed LAST — after the cached device/stream statics below are
    // gone, which is the documented "late static destructor" scenario.
    struct teardown_last {
        ~teardown_last() {
            musac::audio_system::done();
            std::puts("teardown-last: done() survived");
        }
    };
    teardown_last g_teardown;

    // Client-side statics, like a game caching a device and a sound effect.
    // Declared device-first so destruction order (reverse) destroys the
    // stream before its parent device, per the documented contract — both
    // after main() returns, both before ~teardown_last.
    std::unique_ptr<musac::audio_device> g_cached_device;
    std::unique_ptr<musac::audio_stream> g_cached_stream;

#if defined(__GNUC__) || defined(__clang__)
    // Extra GNU-only probe: glibc runs .fini_array after ALL C++ static
    // destructors in the program — later than any static-destructor slot.
    // done() is idempotent, so re-running it here checks the even-harsher
    // ordering. MSVC has no equivalent attribute; the portable teardown_last
    // object above carries the scenario there.
    __attribute__((destructor)) void teardown_after_fini() {
        musac::audio_system::done();
        std::puts("fini-array probe: done() survived");
    }
#endif

} // namespace

int main() {
    auto backend = std::make_shared<musac::test::mock_backend_v2_enhanced>();
    if (!musac::audio_system::init(backend)) {
        std::puts("FAIL: audio_system::init");
        return 1;
    }

    g_cached_device = std::make_unique<musac::audio_device>(
        musac::audio_device::open_default_device(backend));

    // The decoder ignores its stream, but audio_source requires one
    static const uint8_t dummy[16] = {};
    musac::audio_source source(std::make_unique<silence_decoder>(),
                               musac::io_from_memory(dummy, sizeof(dummy)));
    g_cached_stream = std::make_unique<musac::audio_stream>(
        g_cached_device->create_stream(std::move(source)));

    if (!g_cached_stream->play()) {
        std::puts("FAIL: play");
        return 1;
    }

    std::puts("main: ok");
    return 0;
    // ...then static destructors run (stream, then device, then
    // ~teardown_last calls done()), then on GNU the .fini_array probe
    // calls done() once more.
}

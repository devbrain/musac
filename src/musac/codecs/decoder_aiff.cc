#include <musac/codecs/decoder_aiff.hh>
#include <stdexcept>
#include <musac/codecs/aiff/aiff_container.hh>
#include <musac/codecs/aiff/aiff_codec_factory.hh>
#include <musac/codecs/aiff/aiff_codec_base.hh>
#include <musac/error.hh>
#include <musac/sdk/io_stream.hh>
#include <iff/fourcc.hh>
#include <vector>
#include <cmath>

namespace musac {
    // Implementation class
    struct decoder_aiff::impl {
        public:
            impl()
                : m_io(nullptr)
                  , m_all_data_loaded(false) {
            }

            ~impl() = default;

            void open(io_stream* io) {
                if (!io) {
                    throw std::runtime_error("No IO stream provided");
                }

                // Reset any previous state
                m_container.reset();
                m_codec.reset();

                m_io = io;

                // Create and parse container
                m_container = std::make_unique <aiff::aiff_container>(io);
                m_container->parse();

                // Get codec parameters
                auto params = m_container->get_codec_params();

                // Create appropriate codec
                m_codec = aiff::aiff_codec_factory::create(
                    m_container->get_compression_type(),
                    params
                );

                // For IMA4, load ALL data at once like V2 does
                if (m_container->get_compression_type() == iff::fourcc("ima4")) {
                    // Load entire SSND data into memory for IMA4
                    const auto& ssnd = m_container->get_ssnd_data();
                    m_read_buffer.resize(ssnd.data_size);

                    // Seek to beginning and read all audio data
                    m_container->seek_to_frame(0);
                    size_t bytes_read = m_container->read_audio_data(m_read_buffer.data(), ssnd.data_size);
                    if (bytes_read != ssnd.data_size) {
                        throw std::runtime_error("Failed to read complete IMA4 data");
                    }

                    m_all_data_loaded = true;
                    m_data_read_position = 0;

                    // Reset to beginning for subsequent reads
                    m_container->seek_to_frame(0);
                } else {
                    // For other formats, use chunked reading
                    // Size it for reasonable chunks (e.g., 1 second of audio)
                    size_t buffer_size = params.sample_rate * params.channels * 4;
                    m_read_buffer.resize(buffer_size);
                    m_all_data_loaded = false;
                }
                // Successfully opened
                // Note: set_is_open is called from the base class
            }

            const char* get_name() const {
                if (m_codec) {
                    return m_codec->name();
                }
                return "AIFF";
            }

            sample_rate_t get_rate() const {
                if (!m_container) return 0;
                return static_cast <sample_rate_t>(m_container->get_comm_data().sample_rate);
            }

            channels_t get_channels() const {
                if (!m_container) return 0;
                return m_container->get_comm_data().num_channels;
            }

            std::chrono::microseconds duration() const {
                if (!m_container) {
                    return std::chrono::microseconds(0);
                }

                const auto& comm = m_container->get_comm_data();
                if (comm.sample_rate <= 0) {
                    return std::chrono::microseconds(0);
                }

                // Calculate duration from frames and sample rate
                double seconds = static_cast <double>(comm.num_sample_frames) / comm.sample_rate;
                return std::chrono::microseconds(static_cast <int64_t>(seconds * 1000000.0));
            }

            bool rewind() {
                if (!m_container) return false;

                // Reset container position
                bool result = m_container->seek_to_frame(0);

                // Reset codec state
                if (m_codec) {
                    m_codec->reset();
                }

                // Reset read position for pre-loaded data
                if (m_all_data_loaded) {
                    m_data_read_position = 0;
                }

                return result;
            }

            bool seek_to_time(std::chrono::microseconds time) {
                if (!m_container) return false;

                const auto& comm = m_container->get_comm_data();
                if (comm.sample_rate <= 0) return false;

                // Convert time to sample position
                double seconds = time.count() / 1000000.0;
                uint64_t sample_position = static_cast <uint64_t>(seconds * comm.sample_rate);

                // For block-based codecs, align to block boundary
                if (m_codec) {
                    size_t block_align = m_codec->get_block_align();
                    if (block_align > 1) {
                        sample_position = (sample_position / block_align) * block_align;
                    }
                }

                // Seek in container
                bool result = m_container->seek_to_frame(sample_position);

                // Reset codec state for seeking
                if (m_codec && result) {
                    m_codec->reset();
                }

                // Update read position for pre-loaded IMA4 data
                if (m_all_data_loaded && comm.compression_type == iff::fourcc("ima4")) {
                    // Calculate byte position for IMA4
                    size_t blocks = sample_position / 64;
                    m_data_read_position = blocks * 34 * comm.num_channels;
                }

                return result;
            }

            size_t decode(float* buf, size_t len, bool& call_again) {
                if (!m_container || !m_codec) {
                    call_again = false;
                    return 0;
                }

                // CRITICAL: Respect the output buffer size to prevent buffer overruns
                // Convert from samples to frames based on channel count
                const auto& comm = m_container->get_comm_data();
                const auto& ssnd = m_container->get_ssnd_data();

                // Special handling for IMA4 - use pre-loaded data like V2
                if (m_all_data_loaded && comm.compression_type == iff::fourcc("ima4")) {
                    // Calculate frames remaining based on our position in the loaded data
                    size_t blocks_consumed = m_data_read_position / (34 * comm.num_channels);
                    size_t frames_consumed = blocks_consumed * 64;
                    size_t frames_remaining = comm.num_sample_frames - frames_consumed;
                    size_t frames_to_decode = std::min(len / comm.num_channels, frames_remaining);
                    
                    // IMA4 MUST be decoded in complete blocks of 64 frames
                    // Round down to complete blocks
                    size_t blocks_to_decode = frames_to_decode / 64;
                    if (blocks_to_decode == 0 && frames_remaining > 0) {
                        // If we have less than a full block remaining but still have data,
                        // we need to decode at least one block if the output buffer can hold it
                        if (len >= 64 * comm.num_channels) {
                            blocks_to_decode = 1;
                        }
                    }
                    
                    frames_to_decode = blocks_to_decode * 64;
                    size_t samples_to_decode = frames_to_decode * comm.num_channels;

                    // Calculate bytes to decode (34 bytes per block per channel)
                    size_t bytes_to_decode = blocks_to_decode * 34 * comm.num_channels;
                    size_t bytes_available = m_read_buffer.size() - m_data_read_position;
                    
                    if (bytes_to_decode == 0 || bytes_to_decode > bytes_available) {
                        call_again = false;
                        return 0;
                    }

                    // Decode from pre-loaded buffer
                    size_t samples_decoded = m_codec->decode(
                        m_read_buffer.data() + m_data_read_position,
                        bytes_to_decode,
                        buf,
                        len  // Pass the full buffer size, decoder will limit to what it can decode
                    );

                    // Update position in pre-loaded data
                    // We need to update by the actual bytes consumed
                    size_t blocks_actually_decoded = samples_decoded / (64 * comm.num_channels);
                    size_t bytes_actually_consumed = blocks_actually_decoded * 34 * comm.num_channels;
                    m_data_read_position += bytes_actually_consumed;

                    // Update container's frame position to match
                    m_container->seek_to_frame(m_container->get_current_frame() + blocks_actually_decoded * 64);

                    call_again = (samples_decoded > 0) && (m_data_read_position < m_read_buffer.size());

                    return samples_decoded;
                }

                // Original chunked reading code for non-IMA4 formats
                size_t frames_remaining = comm.num_sample_frames - m_container->get_current_frame();

                // For G.711 with malformed files (like the test data), we need to limit
                // based on actual data size using the same calculation as v2
                if (comm.compression_type == iff::fourcc("ULAW") || comm.compression_type == iff::fourcc("ulaw") ||
                    comm.compression_type == iff::fourcc("ALAW") || comm.compression_type == iff::fourcc("alaw")) {
                    // G.711 (A-law and μ-law) is always 1 byte per sample regardless of what COMM says
                    // Some AIFF files incorrectly report 16-bit sample size for 8-bit G.711 data
                    size_t bytes_per_sample = 1; // G.711 is always 1 byte per sample
                    size_t actual_frames = ssnd.data_size / (bytes_per_sample * comm.num_channels);
                    if (m_container->get_current_frame() < actual_frames) {
                        size_t actual_remaining = actual_frames - m_container->get_current_frame();
                        frames_remaining = std::min(frames_remaining, actual_remaining);
                    } else {
                        frames_remaining = 0; // No more data
                    }
                }

                size_t frames_to_decode = std::min(len / comm.num_channels, frames_remaining);
                size_t samples_to_decode = frames_to_decode * comm.num_channels;

                // Ensure we don't decode more than the output buffer can hold
                if (samples_to_decode > len) {
                    samples_to_decode = len;
                    frames_to_decode = samples_to_decode / comm.num_channels;
                }

                // Calculate how many compressed bytes we need for these samples
                size_t bytes_needed = m_codec->get_input_bytes_for_samples(samples_to_decode);

                // Don't read more than our buffer can hold
                if (bytes_needed > m_read_buffer.size()) {
                    bytes_needed = m_read_buffer.size();
                }

                // Read compressed audio data from container
                size_t bytes_read = m_container->read_audio_data(
                    m_read_buffer.data(),
                    bytes_needed
                );

                if (bytes_read == 0) {
                    call_again = false;
                    return 0;
                }

                // Decode to float samples
                // Pass the actual limit (samples_to_decode) not the full buffer size (len)
                size_t samples_decoded = m_codec->decode(
                    m_read_buffer.data(),
                    bytes_read,
                    buf,
                    samples_to_decode // CRITICAL: Use calculated limit, not full buffer size
                );

                // Check if we have more data
                // We should check against the frame count, but also handle case where
                // actual data is less than what COMM chunk claims
                // If we got no data or decoded no samples, we're done
                call_again = (samples_decoded > 0) &&
                             (m_container->get_current_frame() < m_container->get_total_frames());

                return samples_decoded;
            }

            static bool accept(io_stream* io) {
                if (!io) return false;

                // Save position
                auto pos = io->tell();

                // Check for FORM....AIFF or FORM....AIFC
                char magic[12];
                bool result = false;

                if (io->read(magic, 12) == 12) {
                    if (std::memcmp(magic, "FORM", 4) == 0) {
                        if (std::memcmp(magic + 8, "AIFF", 4) == 0 ||
                            std::memcmp(magic + 8, "AIFC", 4) == 0) {
                            result = true;
                        }
                    }
                }

                // Restore position
                io->seek(pos, seek_origin::set);

                return result;
            }

        private:
            io_stream* m_io;

            std::unique_ptr <aiff::aiff_container> m_container;
            std::unique_ptr <aiff::aiff_codec_base> m_codec;
            std::vector <uint8_t> m_read_buffer;

            // For IMA4: load all data at once like V2
            bool m_all_data_loaded;
            size_t m_data_read_position;
    };

    // Public interface implementation
    decoder_aiff::decoder_aiff()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_aiff::~decoder_aiff() = default;

    void decoder_aiff::open(io_stream* io) {
        m_pimpl->open(io);
        set_is_open(true);
    }

    const char* decoder_aiff::get_name() const {
        return m_pimpl->get_name();
    }

    sample_rate_t decoder_aiff::get_rate() const {
        return m_pimpl->get_rate();
    }

    channels_t decoder_aiff::get_channels() const {
        return m_pimpl->get_channels();
    }

    std::chrono::microseconds decoder_aiff::duration() const {
        return m_pimpl->duration();
    }

    bool decoder_aiff::rewind() {
        return m_pimpl->rewind();
    }

    bool decoder_aiff::seek_to_time(std::chrono::microseconds time) {
        return m_pimpl->seek_to_time(time);
    }

    bool decoder_aiff::accept(io_stream* io) {
        return impl::accept(io);
    }

    size_t decoder_aiff::do_decode(float* buf, size_t len, bool& call_again) {
        return m_pimpl->decode(buf, len, call_again);
    }
} // namespace musac

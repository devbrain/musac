//
// Enhanced VOC decoder with support for A-law, µ-law, and ADPCM codecs
//

#include <musac/codecs/decoder_voc.hh>
#include <musac/codecs/common/g711_codec.hh>
#include <musac/codecs/common/adpcm_codec.hh>
#include <musac/sdk/samples_converter.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/audio_converter.hh>
#include <musac/sdk/endian.hh>

#include <musac/sdk/musac_sdk_config.h>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>
#include <algorithm>
#include <cstring>
#include <vector>

namespace musac {
    // Helper function to convert audio samples and return a vector
    static std::vector <uint8_t> convert_audio_samples_to_vector(
        const audio_spec& src_spec, const uint8_t* src_data, size_t src_len,
        const audio_spec& dst_spec) {
        // Use new audio converter API
        buffer <uint8_t> converted = audio_converter::convert(src_spec, src_data, src_len, dst_spec);
        // Copy to vector (until we refactor to use buffer throughout)
        return std::vector <uint8_t>(converted.begin(), converted.end());
    }

    // VOC format constants
    static constexpr int ST_SIZE_BYTE = 1;
    static constexpr int ST_SIZE_WORD = 2;

    static constexpr int VOC_TERM = 0;
    static constexpr int VOC_DATA = 1;
    static constexpr int VOC_CONT = 2;
    static constexpr int VOC_SILENCE = 3;
    static constexpr int VOC_MARKER = 4;
    static constexpr int VOC_TEXT = 5;
    static constexpr int VOC_LOOP = 6;
    static constexpr int VOC_LOOPEND = 7;
    static constexpr int VOC_EXTENDED = 8;
    static constexpr int VOC_DATA_16 = 9;

    static constexpr uint32_t VOC_BAD_RATE = ~((uint32_t)0);

    // VOC codec types
    enum voc_codec_type {
        VOC_CODEC_PCM_U8 = 0, // 8-bit unsigned PCM
        VOC_CODEC_ADPCM_4 = 1, // Creative ADPCM 4-bit
        VOC_CODEC_ADPCM_26 = 2, // Creative ADPCM 2.6-bit
        VOC_CODEC_ADPCM_2 = 3, // Creative ADPCM 2-bit
        VOC_CODEC_PCM_S16 = 4, // 16-bit signed PCM
        VOC_CODEC_ALAW = 6, // A-law
        VOC_CODEC_MULAW = 7 // µ-law
    };

    struct voc_data {
        uint32_t rest; // bytes remaining in current block
        uint32_t rate; // rate code (byte) of this chunk
        int silent; // sound or silence?
        uint32_t srate; // rate code (byte) of silence
        uint32_t blockseek; // start of current output block
        uint32_t samples; // number of samples output
        uint32_t size; // word length of data
        uint8_t channels; // number of sound channels
        int has_extended; // Has an extended block been read?
        voc_codec_type codec; // Current codec type
    };

    struct decoder_voc::impl {
        io_stream* m_rwops = nullptr;
        audio_spec m_spec = {};
        std::vector <uint8_t> m_buffer;
        size_t m_total_samples = 0;
        size_t m_consumed = 0;
        to_float_converter_func_t m_converter = nullptr;

        // Codec states for stateful decoders
        codecs::common::creative_adpcm_state m_adpcm_state;

        bool check_header() {
            // VOC magic header
            uint8_t signature[20]; // "Creative Voice File\032"
            uint16_t datablockofs;

            m_rwops->seek(0, musac::seek_origin::set);

            if (m_rwops->read(signature, sizeof(signature)) != sizeof(signature)) {
                return false;
            }

            if (std::memcmp(signature, "Creative Voice File\032", sizeof(signature)) != 0) {
                THROW_RUNTIME("Unrecognized file type (not VOC)");
            }

            // get the offset where the first datablock is located
            if (m_rwops->read(&datablockofs, sizeof(uint16_t)) != sizeof(uint16_t)) {
                return false;
            }

            datablockofs = musac::swap16le(datablockofs);

            if (m_rwops->seek(datablockofs, musac::seek_origin::set) != datablockofs) {
                return false;
            }

            return true;
        }

        bool get_block(voc_data& v, audio_spec& spec) {
            uint8_t bits24[3];
            uint8_t uc, block;
            uint32_t sblen;
            uint16_t new_rate_short;
            uint32_t new_rate_long;
            uint8_t trash[6];
            uint16_t period;
            uint8_t codec_byte;

            v.silent = 0;
            while (v.rest == 0) {
                if (m_rwops->read(&block, sizeof(block)) != sizeof(block)) {
                    return true; // assume that's the end of the file.
                }

                if (block == VOC_TERM) {
                    // Terminator block - no more data
                    v.rest = 0;
                    return false;
                }

                if (m_rwops->read(bits24, sizeof(bits24)) != sizeof(bits24)) {
                    return true; // assume that's the end of the file.
                }

                // Size is an 24-bit value. Ugh.
                sblen = (uint32_t)((bits24[0]) | (bits24[1] << 8) | (bits24[2] << 16));

                // Sanity check: blocks shouldn't be larger than 16MB
                if (sblen > 16 * 1024 * 1024) {
                    THROW_RUNTIME("VOC block size too large");
                }

                switch (block) {
                    case VOC_DATA:
                        if (m_rwops->read(&uc, sizeof(uc)) != sizeof(uc)) {
                            return false;
                        }

                        // When DATA block preceeded by an EXTENDED
                        // block, the DATA blocks rate value is invalid
                        if (!v.has_extended) {
                            if (uc == 0) {
                                THROW_RUNTIME("VOC Sample rate is zero?");
                            }

                            if ((v.rate != VOC_BAD_RATE) && (uc != v.rate)) {
                                THROW_RUNTIME("VOC sample rate codes differ");
                            }

                            v.rate = uc;
                            if (v.rate >= 256) {
                                THROW_RUNTIME("VOC sample rate out of range");
                            }
                            spec.freq = (uint16_t)(1000000.0 / (256 - v.rate));
                            v.channels = 1;
                        }

                        // Read codec byte
                        if (m_rwops->read(&codec_byte, sizeof(codec_byte)) != sizeof(codec_byte)) {
                            return false;
                        }

                        // Set codec type
                        v.codec = static_cast <voc_codec_type>(codec_byte);

                        // Validate codec type
                        switch (v.codec) {
                            case VOC_CODEC_PCM_U8:
                                v.size = ST_SIZE_BYTE;
                                break;
                            case VOC_CODEC_ADPCM_4:
                            case VOC_CODEC_ADPCM_26:
                            case VOC_CODEC_ADPCM_2:
                                v.size = ST_SIZE_BYTE; // ADPCM is packed into bytes
                                m_adpcm_state.reset(); // Reset ADPCM state for new block
                                break;
                            case VOC_CODEC_ALAW:
                            case VOC_CODEC_MULAW:
                                v.size = ST_SIZE_BYTE; // G.711 is 8-bit
                                break;
                            default:
                                THROW_RUNTIME("VOC decoder: unsupported codec type");
                        }

                        v.has_extended = 0;
                        v.rest = sblen - 2;
                        return true;

                    case VOC_DATA_16:
                        if (m_rwops->read(&new_rate_long, sizeof(new_rate_long)) != sizeof(new_rate_long)) {
                            return false;
                        }
                        new_rate_long = musac::swap32le(new_rate_long);
                        if (new_rate_long == 0) {
                            THROW_RUNTIME("VOC Sample rate is zero?");
                        }
                        if ((v.rate != VOC_BAD_RATE) && (new_rate_long != v.rate)) {
                            THROW_RUNTIME("VOC sample rate codes differ");
                        }
                        v.rate = new_rate_long;
                        spec.freq = (int)new_rate_long;

                        if (m_rwops->read(&uc, sizeof(uc)) != sizeof(uc)) {
                            return false;
                        }

                        switch (uc) {
                            case 8: v.size = ST_SIZE_BYTE;
                                break;
                            case 16: v.size = ST_SIZE_WORD;
                                break;
                            default:
                                THROW_RUNTIME("VOC with unknown data size");
                        }

                        if (m_rwops->read(&v.channels, sizeof(uint8_t)) != sizeof(uint8_t)) {
                            return false;
                        }

                        // Read codec byte from the 6 format bytes
                        uint8_t format_info[6];
                        if (m_rwops->read(format_info, 6) != 6) {
                            return false;
                        }

                        // format_info[0] is the codec type
                        v.codec = static_cast <voc_codec_type>(format_info[0]);

                        v.rest = sblen - 12;
                        return true;

                    case VOC_CONT:
                        v.rest = sblen;
                        return true;

                    case VOC_SILENCE:
                        if (m_rwops->read(&period, sizeof(period)) != sizeof(period)) {
                            return false;
                        }
                        period = musac::swap16le(period);

                        if (m_rwops->read(&uc, sizeof(uc)) != sizeof(uc)) {
                            return false;
                        }
                        if (uc == 0) {
                            THROW_RUNTIME("VOC silence sample rate is zero");
                        }

                        // Some silence-packed files have gratuitously
                        // different sample rate codes in silence.
                        // Adjust period.
                        if ((v.rate != VOC_BAD_RATE) && (uc != v.rate))
                            period = (uint16_t)((period * (256 - uc)) / (256 - v.rate));
                        else
                            v.rate = uc;
                        v.rest = period;
                        v.silent = 1;
                        return true;

                    case VOC_LOOP:
                    case VOC_LOOPEND:
                        for (unsigned int i = 0; i < sblen; i++) {
                            // skip repeat loops.
                            if (m_rwops->read(trash, sizeof(uint8_t)) != sizeof(uint8_t)) {
                                return false;
                            }
                        }
                        break;

                    case VOC_EXTENDED:
                        // An Extended block is followed by a data block
                        // Set this byte so we know to use the rate
                        // value from the extended block and not the
                        // data block.
                        v.has_extended = 1;
                        if (m_rwops->read(&new_rate_short, sizeof(new_rate_short)) != sizeof(new_rate_short)) {
                            return false;
                        }
                        new_rate_short = musac::swap16le(new_rate_short);
                        if (new_rate_short == 0) {
                            THROW_RUNTIME("VOC sample rate is zero");
                        }
                        if ((v.rate != VOC_BAD_RATE) && (new_rate_short != v.rate)) {
                            THROW_RUNTIME("VOC sample rate codes differ");
                        }
                        v.rate = new_rate_short;

                        if (m_rwops->read(&uc, sizeof(uc)) != sizeof(uc)) {
                            return false;
                        }

                        // This was the codec byte in the extended block
                        v.codec = static_cast <voc_codec_type>(uc);

                        if (m_rwops->read(&uc, sizeof(uc)) != sizeof(uc)) {
                            return false;
                        }

                        if (uc) // Stereo
                            spec.channels = 2;
                        else
                            spec.channels = 1;
                        // Needed number of channels before finishing
                        // compute for rate
                        if (v.rate >= 65536L) {
                            THROW_RUNTIME("VOC extended rate out of range");
                        }
                        spec.freq = (256000000L / (65536L - v.rate)) / spec.channels;
                        // An extended block must be followed by a data
                        // block to be valid so loop back to top so it
                        // can be grabed.
                        continue;

                    case VOC_MARKER:
                        if (m_rwops->read(trash, 2) != 2) {
                            return false;
                        }
                    // fallthrough

                    default: // text block or other krapola.
                        for (unsigned int i = 0; i < sblen; i++) {
                            if (m_rwops->read(trash, sizeof(uint8_t)) != sizeof(uint8_t)) {
                                return false;
                            }
                        }

                        if (block == VOC_TEXT) {
                            continue; // get next block
                        }
                }
            }

            return true;
        }

        uint32_t voc_read(voc_data& v, uint8_t* buf, uint32_t buf_size, audio_spec& spec) {
            int64_t done = 0;
            uint8_t silence = 0x80;

            if (v.rest == 0) {
                if (!get_block(v, spec)) {
                    return 0;
                }
            }

            if (v.rest == 0) {
                return 0;
            }

            if (v.silent) {
                if (v.size == ST_SIZE_WORD) {
                    silence = 0x00;
                }

                // Fill in silence (limited by buffer size)
                uint32_t to_fill = (v.rest < buf_size) ? v.rest : buf_size;
                std::memset(buf, silence, to_fill);
                done = to_fill;
                v.rest -= to_fill;
            } else {
                // For expanding codecs, limit read based on how much compressed data will fit after expansion
                uint32_t max_read = buf_size;
                if (v.codec == VOC_CODEC_ADPCM_4) {
                    // ADPCM 4-bit expands 2x (1 byte -> 2 samples -> 4 bytes)
                    max_read = buf_size / 4;
                } else if (v.codec == VOC_CODEC_ADPCM_26) {
                    // ADPCM 2.6-bit expands ~3x (1 byte -> 3 samples -> 6 bytes)
                    max_read = buf_size / 6;
                } else if (v.codec == VOC_CODEC_ADPCM_2) {
                    // ADPCM 2-bit expands 4x (1 byte -> 4 samples -> 8 bytes)
                    max_read = buf_size / 8;
                } else if (v.codec == VOC_CODEC_ALAW || v.codec == VOC_CODEC_MULAW) {
                    // A-law/μ-law expand 2x (1 byte -> 1 sample -> 2 bytes)
                    max_read = buf_size / 2;
                }

                uint32_t to_read = (v.rest < max_read) ? v.rest : max_read;
                done = m_rwops->read(buf, to_read);
                if (done <= 0) {
                    return 0;
                }

                v.rest = (uint32_t)(v.rest - done);

                // Decode based on codec type
                std::vector <uint8_t> decoded_buffer;

                switch (v.codec) {
                    case VOC_CODEC_PCM_U8:
                        // No decoding needed for PCM
                        break;

                    case VOC_CODEC_PCM_S16:
                        // Handle endianness for 16-bit PCM
                        if (v.size == ST_SIZE_WORD) {
#if MUSAC_BIG_ENDIAN
                            uint16_t* samples = (uint16_t*)buf;
                            size_t sample_count = done / 2;
                            for (size_t i = 0; i < sample_count; i++) {
                                samples[i] = musac::swap16le(samples[i]);
                            }
#endif
                        }
                        break;

                    case VOC_CODEC_ADPCM_4: {
                        // Decode Creative ADPCM 4-bit to int16
                        size_t output_samples = done * 2; // Each byte has 2 samples
                        size_t output_bytes = output_samples * sizeof(int16_t);

                        // Check if output fits in buffer
                        if (output_bytes > buf_size) {
                            // Limit to what fits in buffer
                            output_samples = buf_size / sizeof(int16_t);
                            output_bytes = output_samples * sizeof(int16_t);
                            done = output_samples / 2; // Adjust input size
                        }

                        decoded_buffer.resize(output_bytes);
                        int16_t* output = reinterpret_cast <int16_t*>(decoded_buffer.data());

                        size_t decoded = codecs::common::decode_creative_adpcm_4bit_to_int16(
                            buf, done, output, m_adpcm_state);

                        // Copy decoded data back to buffer (now safe)
                        std::memcpy(buf, decoded_buffer.data(), decoded * sizeof(int16_t));
                        done = decoded * sizeof(int16_t); // Return byte count
                        v.size = ST_SIZE_WORD; // Output is 16-bit
                        break;
                    }

                    case VOC_CODEC_ADPCM_26: {
                        // Decode Creative ADPCM 2.6-bit to int16
                        size_t output_samples = done * 3; // Each byte has ~3 samples
                        size_t output_bytes = output_samples * sizeof(int16_t);

                        // Check if output fits in buffer
                        if (output_bytes > buf_size) {
                            // Limit to what fits in buffer
                            output_samples = buf_size / sizeof(int16_t);
                            output_bytes = output_samples * sizeof(int16_t);
                            done = output_samples / 3; // Adjust input size
                        }

                        decoded_buffer.resize(output_bytes);
                        int16_t* output = reinterpret_cast <int16_t*>(decoded_buffer.data());

                        size_t decoded = codecs::common::decode_creative_adpcm_26bit_to_int16(
                            buf, done, output, m_adpcm_state);

                        // Copy decoded data back to buffer (now safe)
                        std::memcpy(buf, decoded_buffer.data(), decoded * sizeof(int16_t));
                        done = decoded * sizeof(int16_t); // Return byte count
                        v.size = ST_SIZE_WORD; // Output is 16-bit
                        break;
                    }

                    case VOC_CODEC_ADPCM_2: {
                        // Decode Creative ADPCM 2-bit to int16
                        size_t output_samples = done * 4; // Each byte has 4 samples
                        size_t output_bytes = output_samples * sizeof(int16_t);

                        // Check if output fits in buffer
                        if (output_bytes > buf_size) {
                            // Limit to what fits in buffer
                            output_samples = buf_size / sizeof(int16_t);
                            output_bytes = output_samples * sizeof(int16_t);
                            done = output_samples / 4; // Adjust input size
                        }

                        decoded_buffer.resize(output_bytes);
                        int16_t* output = reinterpret_cast <int16_t*>(decoded_buffer.data());

                        size_t decoded = codecs::common::decode_creative_adpcm_2bit_to_int16(
                            buf, done, output, m_adpcm_state);

                        // Copy decoded data back to buffer (now safe)
                        std::memcpy(buf, decoded_buffer.data(), decoded * sizeof(int16_t));
                        done = decoded * sizeof(int16_t); // Return byte count
                        v.size = ST_SIZE_WORD; // Output is 16-bit
                        break;
                    }

                    case VOC_CODEC_ALAW: {
                        // Decode A-law to int16 (1:2 expansion)
                        size_t output_bytes = done * sizeof(int16_t);

                        // Check if output fits in buffer
                        if (output_bytes > buf_size) {
                            // Limit to what fits in buffer
                            done = buf_size / sizeof(int16_t);
                            output_bytes = done * sizeof(int16_t);
                        }

                        decoded_buffer.resize(output_bytes);
                        int16_t* output = reinterpret_cast <int16_t*>(decoded_buffer.data());

                        size_t decoded = codecs::common::decode_alaw_to_int16(buf, output, done);

                        // Copy decoded data back to buffer (now safe)
                        std::memcpy(buf, decoded_buffer.data(), decoded * sizeof(int16_t));
                        done = decoded * sizeof(int16_t); // Return byte count
                        v.size = ST_SIZE_WORD; // Output is 16-bit
                        break;
                    }

                    case VOC_CODEC_MULAW: {
                        // Decode µ-law to int16 (1:2 expansion)
                        size_t output_bytes = done * sizeof(int16_t);

                        // Check if output fits in buffer
                        if (output_bytes > buf_size) {
                            // Limit to what fits in buffer
                            done = buf_size / sizeof(int16_t);
                            output_bytes = done * sizeof(int16_t);
                        }

                        decoded_buffer.resize(output_bytes);
                        int16_t* output = reinterpret_cast <int16_t*>(decoded_buffer.data());

                        size_t decoded = codecs::common::decode_ulaw_to_int16(buf, output, done);

                        // Copy decoded data back to buffer (now safe)
                        std::memcpy(buf, decoded_buffer.data(), decoded * sizeof(int16_t));
                        done = decoded * sizeof(int16_t); // Return byte count
                        v.size = ST_SIZE_WORD; // Output is 16-bit
                        break;
                    }

                    default:
                        THROW_RUNTIME("VOC decoder: unsupported codec in voc_read");
                }
            }

            return (uint32_t)done;
        }

        bool load_voc() {
            if (!m_rwops) {
                return false;
            }

            if (!check_header()) {
                return false;
            }

            voc_data v = {};
            v.rate = VOC_BAD_RATE;
            v.rest = 0;
            v.has_extended = 0;
            v.codec = VOC_CODEC_PCM_U8; // Default codec
            std::memset(&m_spec, 0, sizeof(m_spec));

            if (!get_block(v, m_spec)) {
                return false;
            }

            if (v.rate == VOC_BAD_RATE) {
                THROW_RUNTIME("VOC data had no sound!");
            }

            if (v.size == 0) {
                THROW_RUNTIME("VOC data had invalid word size!");
            }

            // Set format based on codec and size
            if (v.codec == VOC_CODEC_PCM_U8) {
                m_spec.format = audio_format::u8;
            } else {
                // All other codecs decode to 16-bit signed
                m_spec.format = audio_s16sys;
            }

            if (m_spec.channels == 0) {
                m_spec.channels = v.channels;
            }

            // Read all audio data
            std::vector <uint8_t> temp_buffer;
            // Reserve space to avoid reallocations (estimate based on typical VOC file size)
            temp_buffer.reserve(1024 * 1024); // 1MB initial reservation
            const size_t chunk_size = 16384; // Increased chunk size for better performance
            uint8_t chunk[chunk_size];

            size_t total_bytes_read = 0;
            while (true) {
                uint32_t read = voc_read(v, chunk, chunk_size, m_spec);
                if (read == 0) {
                    // Try to get next block
                    if (!get_block(v, m_spec)) {
                        break;
                    }
                    // Check if we still have no data after get_block
                    if (v.rest == 0) {
                        break;
                    }
                    continue;
                }

                total_bytes_read += read;

                // Update format if codec changed
                if (v.codec != VOC_CODEC_PCM_U8) {
                    m_spec.format = audio_s16sys;
                }

                // For codecs that expand data (ADPCM, A-law, μ-law), read is already the byte count
                // of the decoded data, not the original compressed data
                temp_buffer.insert(temp_buffer.end(), chunk, chunk + read);
            }

            // Calculate total sample count
            int samplesize = audio_format_byte_size(m_spec.format) * m_spec.channels;
            m_total_samples = temp_buffer.size() / samplesize;

            // Convert to S16 format while preserving original sample rate
            audio_spec src_spec = m_spec;
            audio_spec dst_spec;
            dst_spec.format = audio_format::s16le;
            dst_spec.channels = src_spec.channels;
            dst_spec.freq = src_spec.freq; // Preserve original sample rate

            m_buffer = convert_audio_samples_to_vector(
                src_spec, temp_buffer.data(), temp_buffer.size(), dst_spec);

            m_spec = dst_spec;
            m_converter = get_to_float_conveter(m_spec.format);

            return true;
        }
    };

    decoder_voc::decoder_voc()
        : m_pimpl(std::make_unique <impl>()) {
    }

    decoder_voc::~decoder_voc() = default;

    bool decoder_voc::accept(io_stream* rwops) {
        if (!rwops) return false;

        uint8_t buf[20];
        int64_t pos = rwops->tell();
        bool result = false;

        if (rwops->read(buf, sizeof(buf)) == sizeof(buf)) {
            if (std::memcmp(buf, "Creative Voice File\032", sizeof(buf)) == 0) {
                result = true;
            }
        }

        rwops->seek(pos, musac::seek_origin::set);
        return result;
    }

    const char* decoder_voc::get_name() const {
        return "VOC";
    }

    void decoder_voc::open(io_stream* rwops) {
        m_pimpl->m_rwops = rwops;
        if (!m_pimpl->load_voc()) {
            THROW_RUNTIME("Failed to load VOC file");
        }
        set_is_open(true);
    }

    channels_t decoder_voc::get_channels() const {
        return m_pimpl->m_spec.channels;
    }

    sample_rate_t decoder_voc::get_rate() const {
        return m_pimpl->m_spec.freq;
    }

    bool decoder_voc::rewind() {
        m_pimpl->m_consumed = 0;
        return true;
    }

    std::chrono::microseconds decoder_voc::duration() const {
        int64_t us = (m_pimpl->m_total_samples * 1000000LL) / m_pimpl->m_spec.freq;
        return std::chrono::microseconds(us);
    }

    bool decoder_voc::seek_to_time(std::chrono::microseconds pos) {
        size_t sample_pos = (pos.count() * m_pimpl->m_spec.freq) / 1000000;
        if (sample_pos >= m_pimpl->m_total_samples) {
            return false;
        }
        m_pimpl->m_consumed = sample_pos;
        return true;
    }

    size_t decoder_voc::do_decode(float* buf, size_t len, bool& call_again) {
        size_t remaining = m_pimpl->m_total_samples - m_pimpl->m_consumed;
        size_t to_decode = std::min(len, remaining);

        if (to_decode == 0) {
            call_again = false;
            return 0;
        }

        int samplesize = audio_format_byte_size(m_pimpl->m_spec.format);
        size_t offset = m_pimpl->m_consumed * samplesize;

        m_pimpl->m_converter(
            buf,
            m_pimpl->m_buffer.data() + offset,
            to_decode
        );

        m_pimpl->m_consumed += to_decode;
        call_again = (m_pimpl->m_consumed < m_pimpl->m_total_samples);

        return to_decode;
    }
} // namespace musac

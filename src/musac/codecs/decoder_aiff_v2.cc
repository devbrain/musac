/**
 * @file decoder_aiff_v2.cc
 * @brief Modern AIFF/AIFF-C decoder implementation using libiff
 */

#include <musac/codecs/decoder_aiff_v2.hh>
#include <musac/sdk/audio_format.hh>
#include <musac/sdk/audio_converter.hh>
#include <musac/sdk/samples_converter.hh>
#include <musac/error.hh>
#include <failsafe/failsafe.hh>

#include <iff/parser.hh>
#include <iff/handler_registry.hh>
#include <iff/chunk_reader.hh>
#include <iff/fourcc.hh>
#include <iff/endian.hh>

#include <algorithm>
#include <cstring>
#include <cmath>
#include <vector>
#include <map>
#include <fstream>
#include <sstream>
#include <iostream>

namespace musac {

// AIFF chunk identifiers using iff::fourcc
static const iff::fourcc FORM_ID("FORM");
static const iff::fourcc AIFF_ID("AIFF");
static const iff::fourcc AIFC_ID("AIFC");
static const iff::fourcc COMM_ID("COMM");
static const iff::fourcc SSND_ID("SSND");
static const iff::fourcc FVER_ID("FVER");
static const iff::fourcc MARK_ID("MARK");
static const iff::fourcc INST_ID("INST");
static const iff::fourcc COMT_ID("COMT");
static const iff::fourcc APPL_ID("APPL");

// Compression types for AIFF-C
static const iff::fourcc COMP_NONE("NONE");
static const iff::fourcc COMP_ULAW("ULAW");  // Uppercase variant
static const iff::fourcc COMP_ulaw("ulaw");  // Lowercase variant  
static const iff::fourcc COMP_ALAW("ALAW");  // Uppercase variant
static const iff::fourcc COMP_alaw("alaw");  // Lowercase variant
static const iff::fourcc COMP_FL32("fl32");
static const iff::fourcc COMP_FL64("fl64");
static const iff::fourcc COMP_IMA4("ima4");
static const iff::fourcc COMP_SOWT("sowt");  // Little-endian 16-bit (byte-swapped)

// IMA ADPCM tables
static const int16_t ima_step_table[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
    19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
    50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
    130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
    337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
    876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
    2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
    5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

static const int8_t ima_index_table[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

// G.711 µ-law/A-law decompression tables
static const int16_t ulaw_table[256] = {
    -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
    -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
    -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
    -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
     -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
     -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
     -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
     -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
     -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
     -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
      -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
      -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
      -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
      -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
      -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
       -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
     32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
     23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
     15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
     11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
      7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
      5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
      3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
      2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
      1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
      1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
       876,    844,    812,    780,    748,    716,    684,    652,
       620,    588,    556,    524,    492,    460,    428,    396,
       372,    356,    340,    324,    308,    292,    276,    260,
       244,    228,    212,    196,    180,    164,    148,    132,
       120,    112,    104,     96,     88,     80,     72,     64,
        56,     48,     40,     32,     24,     16,      8,      0
};

static const int16_t alaw_table[256] = {
     -5504,  -5248,  -6016,  -5760,  -4480,  -4224,  -4992,  -4736,
     -7552,  -7296,  -8064,  -7808,  -6528,  -6272,  -7040,  -6784,
     -2752,  -2624,  -3008,  -2880,  -2240,  -2112,  -2496,  -2368,
     -3776,  -3648,  -4032,  -3904,  -3264,  -3136,  -3520,  -3392,
    -22016, -20992, -24064, -23040, -17920, -16896, -19968, -18944,
    -30208, -29184, -32256, -31232, -26112, -25088, -28160, -27136,
    -11008, -10496, -12032, -11520,  -8960,  -8448,  -9984,  -9472,
    -15104, -14592, -16128, -15616, -13056, -12544, -14080, -13568,
      -344,   -328,   -376,   -360,   -280,   -264,   -312,   -296,
      -472,   -456,   -504,   -488,   -408,   -392,   -440,   -424,
       -88,    -72,   -120,   -104,    -24,     -8,    -56,    -40,
      -216,   -200,   -248,   -232,   -152,   -136,   -184,   -168,
     -1376,  -1312,  -1504,  -1440,  -1120,  -1056,  -1248,  -1184,
     -1888,  -1824,  -2016,  -1952,  -1632,  -1568,  -1760,  -1696,
      -688,   -656,   -752,   -720,   -560,   -528,   -624,   -592,
      -944,   -912,  -1008,   -976,   -816,   -784,   -880,   -848,
      5504,   5248,   6016,   5760,   4480,   4224,   4992,   4736,
      7552,   7296,   8064,   7808,   6528,   6272,   7040,   6784,
      2752,   2624,   3008,   2880,   2240,   2112,   2496,   2368,
      3776,   3648,   4032,   3904,   3264,   3136,   3520,   3392,
     22016,  20992,  24064,  23040,  17920,  16896,  19968,  18944,
     30208,  29184,  32256,  31232,  26112,  25088,  28160,  27136,
     11008,  10496,  12032,  11520,   8960,   8448,   9984,   9472,
     15104,  14592,  16128,  15616,  13056,  12544,  14080,  13568,
       344,    328,    376,    360,    280,    264,    312,    296,
       472,    456,    504,    488,    408,    392,    440,    424,
        88,     72,    120,    104,     24,      8,     56,     40,
       216,    200,    248,    232,    152,    136,    184,    168,
      1376,   1312,   1504,   1440,   1120,   1056,   1248,   1184,
      1888,   1824,   2016,   1952,   1632,   1568,   1760,   1696,
       688,    656,    752,    720,    560,    528,    624,    592,
       944,    912,   1008,    976,    816,    784,    880,    848
};

// Helper to convert 80-bit IEEE extended precision to double
static double convert_extended_to_double(const uint8_t* bytes) {
    // IEEE 754 80-bit extended precision format:
    // 1 bit sign | 15 bits exponent | 64 bits mantissa
    
    // Extract components
    bool sign = (bytes[0] & 0x80) != 0;
    uint16_t exponent = ((bytes[0] & 0x7F) << 8) | bytes[1];
    uint64_t mantissa = 0;
    
    // Read mantissa (big-endian)
    for (int i = 0; i < 8; ++i) {
        mantissa = (mantissa << 8) | bytes[2 + i];
    }
    
    // Handle special cases
    if (exponent == 0) {
        if (mantissa == 0) {
            return sign ? -0.0 : 0.0;  // Zero
        } else {
            // Denormalized number
            int exp = -16382;
            double result = ldexp(static_cast<double>(mantissa) / (1ULL << 63), exp);
            return sign ? -result : result;
        }
    } else if (exponent == 0x7FFF) {
        // Infinity or NaN
        if (mantissa == 0) {
            return sign ? -INFINITY : INFINITY;
        } else {
            return NAN;
        }
    }
    
    // Normal number
    // The mantissa in 80-bit extended has an explicit integer bit
    // Check if integer bit is set (it should be for normalized numbers)
    bool integer_bit = (mantissa & (1ULL << 63)) != 0;
    if (!integer_bit && exponent != 0) {
        // Unnormalized number (shouldn't happen in AIFF)
        return NAN;
    }
    
    // Convert to double
    // Adjust exponent from bias 16383 to bias 1023
    int exp = exponent - 16383;
    
    // The mantissa represents 1.fraction in normalized form
    // Convert to double precision
    double result = ldexp(static_cast<double>(mantissa) / (1ULL << 63), exp);
    
    return sign ? -result : result;
}

// Common sample rates lookup table for faster conversion
static double get_common_sample_rate(uint16_t exp, uint64_t mantissa) {
    // Check for common AIFF sample rates
    struct RateEntry {
        uint16_t exp;
        uint64_t mantissa;
        double rate;
    };
    
    static const RateEntry common_rates[] = {
        {0x400E, 0xAC44000000000000ULL, 44100.0},   // 44.1 kHz
        {0x400E, 0xBB80000000000000ULL, 48000.0},   // 48 kHz
        {0x400F, 0xBB80000000000000ULL, 96000.0},   // 96 kHz
        {0x400F, 0xAC44000000000000ULL, 88200.0},   // 88.2 kHz
        {0x400D, 0xAC44000000000000ULL, 22050.0},   // 22.05 kHz
        {0x400C, 0xAC44000000000000ULL, 11025.0},   // 11.025 kHz
        {0x400D, 0xFA00000000000000ULL, 32000.0},   // 32 kHz
        {0x400C, 0xFA00000000000000ULL, 16000.0},   // 16 kHz
        {0x400B, 0xFA00000000000000ULL, 8000.0},    // 8 kHz
        {0x4010, 0xBB80000000000000ULL, 192000.0},  // 192 kHz
        {0x4010, 0xAC44000000000000ULL, 176400.0},  // 176.4 kHz
        {0x400C, 0xBB80000000000000ULL, 12000.0},   // 12 kHz
        {0x400B, 0xBB80000000000000ULL, 6000.0},    // 6 kHz
    };
    
    for (const auto& entry : common_rates) {
        if (entry.exp == exp && entry.mantissa == mantissa) {
            return entry.rate;
        }
    }
    
    return 0.0;  // Not a common rate
}

// No need for custom swap_endian - use iff::swap functions

struct decoder_aiff_v2::impl {
    // Stream and parsing state
    io_stream* m_stream = nullptr;
    bool m_is_aifc = false;
    bool m_is_open = false;
    
    // Format information from COMM chunk
    uint16_t m_num_channels = 0;
    uint32_t m_num_sample_frames = 0;
    uint16_t m_sample_size = 0;
    double m_sample_rate = 0.0;
    iff::fourcc m_compression_type = COMP_NONE;
    std::string m_compression_name;
    
    // Sound data information
    uint64_t m_ssnd_offset = 0;        // Absolute file position of sound data
    uint64_t m_ssnd_size = 0;          // Size of actual audio data
    uint32_t m_ssnd_data_offset = 0;   // Offset within SSND chunk to audio data
    uint32_t m_ssnd_block_size = 0;    // Block alignment size
    bool m_has_ssnd = false;           // Whether we found SSND chunk
    std::vector<uint8_t> m_audio_data; // Store the entire audio data in memory
    size_t m_audio_read_pos = 0;       // Current read position in audio data
    
    // Markers
    struct Marker {
        uint16_t id;
        uint32_t position;
        std::string name;
    };
    std::map<uint16_t, Marker> m_markers;
    
    // Instrument data
    struct Loop {
        int16_t play_mode;      // 0=no loop, 1=forward, 2=forward/backward
        int16_t begin_loop;     // Marker ID for loop start
        int16_t end_loop;       // Marker ID for loop end
    };
    
    struct InstrumentData {
        int8_t base_note;       // MIDI note number (0-127, middle C = 60)
        int8_t detune;          // Fine tuning in cents (-50 to +50)
        int8_t low_note;        // Lowest playable note
        int8_t high_note;       // Highest playable note
        int8_t low_velocity;    // Minimum velocity (1-127)
        int8_t high_velocity;   // Maximum velocity (1-127)
        int16_t gain;           // Gain in decibels
        Loop sustain_loop;      // Sustain loop parameters
        Loop release_loop;      // Release loop parameters
    };
    
    bool m_has_instrument = false;
    InstrumentData m_instrument;
    
    // Decoding state
    size_t m_current_frame = 0;
    std::vector<uint8_t> m_decode_buffer;
    to_float_converter_func_t m_converter = nullptr;
    
    // Channel layout information
    enum ChannelLayout {
        LAYOUT_MONO = 1,
        LAYOUT_STEREO = 2,
        LAYOUT_3CH = 3,      // L, R, C
        LAYOUT_QUAD = 4,     // FL, FR, RL, RR  
        LAYOUT_5_1 = 6,      // L, C, R, LS, RS, LFE
        LAYOUT_7_1 = 8       // L, C, R, LS, RS, LB, RB, LFE
    };
    
    ChannelLayout get_channel_layout() const {
        return static_cast<ChannelLayout>(m_num_channels);
    }
    
    // Get readable channel layout name
    const char* get_channel_layout_name() const {
        switch (m_num_channels) {
            case 1: return "Mono";
            case 2: return "Stereo";
            case 3: return "3.0 (LCR)";
            case 4: return "Quad";
            case 5: return "5.0";
            case 6: return "5.1";
            case 7: return "6.1";
            case 8: return "7.1";
            default: return "Multi-channel";
        }
    }
    
    
    // Parse AIFF file using libiff
    void parse_file() {
        if (!m_stream) {
            THROW_RUNTIME("No stream to parse");
        }
        
        // Create a C++ stream wrapper for libiff
        m_stream->seek(0, seek_origin::set);
        
        // Set up handler registry
        iff::handler_registry handlers;
        
        // Handle COMM chunk for AIFF
        handlers.on_chunk_in_form(
            AIFF_ID,
            COMM_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_comm_chunk(event, false);
                }
            }
        );
        
        // Handle COMM chunk for AIFC
        handlers.on_chunk_in_form(
            AIFC_ID,
            COMM_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    m_is_aifc = true;
                    handle_comm_chunk(event, true);
                }
            }
        );
        
        // Handle SSND chunk for AIFF
        handlers.on_chunk_in_form(
            AIFF_ID,
            SSND_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_ssnd_chunk(event);
                }
            }
        );
        
        // Handle SSND chunk for AIFC
        handlers.on_chunk_in_form(
            AIFC_ID,
            SSND_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_ssnd_chunk(event);
                }
            }
        );
        
        // Handle FVER chunk (AIFC format version)
        handlers.on_chunk_in_form(
            AIFC_ID,
            FVER_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_fver_chunk(event);
                }
            }
        );
        
        // Handle MARK chunk for both AIFF and AIFC
        handlers.on_chunk(
            MARK_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_mark_chunk(event);
                }
            }
        );
        
        // Handle INST chunk for both AIFF and AIFC
        handlers.on_chunk(
            INST_ID,
            [this](const iff::chunk_event& event) {
                if (event.type == iff::chunk_event_type::begin) {
                    handle_inst_chunk(event);
                }
            }
        );
        
        // Create stream adapter for libiff
        StreamAdapter adapter(m_stream);
        
        try {
            iff::parse(adapter, handlers);
        } catch (const std::exception& e) {
            THROW_RUNTIME("Failed to parse AIFF file: ", e.what());
        }
        
        // Validate we have the required chunks
        if (m_num_channels == 0 || m_sample_rate == 0) {
            THROW_RUNTIME("Missing or invalid COMM chunk");
        }
        if (m_ssnd_size == 0) {
            THROW_RUNTIME("Missing or invalid SSND chunk");
        }
        
        // Set up converter based on format
        setup_converter();
    }
    
    void handle_comm_chunk(const iff::chunk_event& event, bool is_aifc) {
        // Standard AIFF COMM chunk is 18 bytes
        // AIFF-C COMM chunk has additional compression type info
        const size_t standard_comm_size = 18;
        
        
        if (event.header.size < standard_comm_size) {
            THROW_RUNTIME("COMM chunk too small: ", event.header.size);
        }
        
        // Read COMM chunk data (must be packed to avoid padding)
        #pragma pack(push, 1)
        struct {
            int16_t num_channels;
            uint32_t num_sample_frames;
            int16_t sample_size;
            uint8_t sample_rate[10];
        } comm;
        #pragma pack(pop)
        
        static_assert(sizeof(comm) == 18, "COMM structure must be 18 bytes");
        
        size_t bytes_read = event.reader->read(&comm, sizeof(comm));
        if (bytes_read < sizeof(comm)) {
            THROW_RUNTIME("Failed to read COMM chunk data, got ", bytes_read, " bytes");
        }
        
        // AIFF uses big-endian format - convert to native
        // Note: iff::swap16be/swap32be convert FROM big-endian TO native
        m_num_channels = iff::swap16be(comm.num_channels);
        m_num_sample_frames = iff::swap32be(comm.num_sample_frames);
        m_sample_size = iff::swap16be(comm.sample_size);
        
        // Try common rate lookup first
        uint16_t exp = ((comm.sample_rate[0] & 0x7F) << 8) | comm.sample_rate[1];
        uint64_t mantissa = 0;
        for (int i = 0; i < 8; ++i) {
            mantissa = (mantissa << 8) | comm.sample_rate[2 + i];
        }
        
        m_sample_rate = get_common_sample_rate(exp, mantissa);
        if (m_sample_rate == 0.0) {
            // Not a common rate, do full conversion
            m_sample_rate = convert_extended_to_double(comm.sample_rate);
        }
        
        // For AIFC, read compression type
        if (is_aifc && event.header.size > standard_comm_size) {
            char comp_type[4];
            event.reader->read(comp_type, sizeof(comp_type));
            m_compression_type = iff::fourcc(comp_type);
            
            // Read compression name (Pascal string)
            uint8_t name_len;
            if (event.reader->read(&name_len, 1) == 1 && name_len > 0) {
                std::vector<char> name(name_len + 1, 0);
                event.reader->read(name.data(), name_len);
                m_compression_name = name.data();
            }
        } else if (!is_aifc) {
            // Standard AIFF has no compression
            m_compression_type = COMP_NONE;
        }
    }
    
    void handle_ssnd_chunk(const iff::chunk_event& event) {
        // Read SSND header: offset and block size
        uint32_t offset, block_size;
        event.reader->read(&offset, sizeof(offset));
        event.reader->read(&block_size, sizeof(block_size));
        
        // Convert from big-endian
        offset = iff::swap32be(offset);
        block_size = iff::swap32be(block_size);
        
        // Store SSND parameters
        m_ssnd_data_offset = offset;  // Offset to skip before audio data
        m_ssnd_size = event.header.size - 8 - offset;  // Actual audio data size
        m_ssnd_block_size = block_size;
        m_has_ssnd = true;
        
        // Skip offset bytes if needed
        if (offset > 0) {
            event.reader->skip(offset);
        }
        
        // Read all audio data into memory using libiff's chunk reader
        // This is the proper way to handle AIFF with libiff
        m_audio_data.resize(m_ssnd_size);
        size_t bytes_read = event.reader->read(m_audio_data.data(), m_ssnd_size);
        if (bytes_read != m_ssnd_size) {
            // Handle partial read
            m_audio_data.resize(bytes_read);
            m_ssnd_size = bytes_read;
        }
        m_audio_read_pos = 0;
    }
    
    void handle_fver_chunk(const iff::chunk_event& event) {
        uint32_t version;
        event.reader->read(&version, sizeof(version));
        version = iff::swap32be(version);
        
        // AIFF-C version 1 is 0xA2805140
        if (version != 0xA2805140) {
            // Log warning but continue
        }
    }
    
    void handle_mark_chunk(const iff::chunk_event& event) {
        uint16_t num_markers;
        event.reader->read(&num_markers, sizeof(num_markers));
        num_markers = iff::swap16be(num_markers);
        
        for (int i = 0; i < num_markers; ++i) {
            Marker marker;
            event.reader->read(&marker.id, sizeof(marker.id));
            event.reader->read(&marker.position, sizeof(marker.position));
            
            marker.id = iff::swap16be(marker.id);
            marker.position = iff::swap32be(marker.position);
            
            // Read marker name (Pascal string)
            uint8_t name_len;
            event.reader->read(&name_len, sizeof(name_len));
            
            if (name_len > 0) {
                std::vector<char> name(name_len + 1, 0);
                event.reader->read(name.data(), name_len);
                marker.name = name.data();
            }
            
            // Skip padding byte if needed
            if (name_len % 2 == 0) {
                event.reader->skip(1);
            }
            
            m_markers[marker.id] = marker;
        }
    }
    
    void handle_inst_chunk(const iff::chunk_event& event) {
        // INST chunk structure (20 bytes):
        // int8_t baseNote
        // int8_t detune
        // int8_t lowNote
        // int8_t highNote
        // int8_t lowVelocity
        // int8_t highVelocity
        // int16_t gain
        // Loop sustainLoop (6 bytes)
        // Loop releaseLoop (6 bytes)
        
        // Read the fixed-size instrument data
        #pragma pack(push, 1)
        struct {
            int8_t base_note;
            int8_t detune;
            int8_t low_note;
            int8_t high_note;
            int8_t low_velocity;
            int8_t high_velocity;
            int16_t gain;
            struct {
                int16_t play_mode;
                int16_t begin_loop;
                int16_t end_loop;
            } sustain_loop;
            struct {
                int16_t play_mode;
                int16_t begin_loop;
                int16_t end_loop;
            } release_loop;
        } inst_data;
        #pragma pack(pop)
        
        static_assert(sizeof(inst_data) == 20, "INST structure must be 20 bytes");
        
        size_t bytes_read = event.reader->read(&inst_data, sizeof(inst_data));
        if (bytes_read != sizeof(inst_data)) {
            THROW_RUNTIME("Failed to read INST chunk data");
        }
        
        // Store instrument data (note: gain and loop fields are big-endian)
        m_instrument.base_note = inst_data.base_note;
        m_instrument.detune = inst_data.detune;
        m_instrument.low_note = inst_data.low_note;
        m_instrument.high_note = inst_data.high_note;
        m_instrument.low_velocity = inst_data.low_velocity;
        m_instrument.high_velocity = inst_data.high_velocity;
        m_instrument.gain = iff::swap16be(inst_data.gain);
        
        // Store loop data
        m_instrument.sustain_loop.play_mode = iff::swap16be(inst_data.sustain_loop.play_mode);
        m_instrument.sustain_loop.begin_loop = iff::swap16be(inst_data.sustain_loop.begin_loop);
        m_instrument.sustain_loop.end_loop = iff::swap16be(inst_data.sustain_loop.end_loop);
        
        m_instrument.release_loop.play_mode = iff::swap16be(inst_data.release_loop.play_mode);
        m_instrument.release_loop.begin_loop = iff::swap16be(inst_data.release_loop.begin_loop);
        m_instrument.release_loop.end_loop = iff::swap16be(inst_data.release_loop.end_loop);
        
        m_has_instrument = true;
    }
    
    void setup_converter() {
        // Determine audio format based on sample size and compression
        audio_format format;
        
        if (m_compression_type == COMP_NONE) {
            // Uncompressed PCM
            switch (m_sample_size) {
                case 8:
                    format = audio_format::s8;
                    break;
                case 12:
                    // 12-bit audio needs special handling (packed format)
                    m_converter = nullptr;
                    return;
                case 16:
                    format = audio_format::s16be;  // AIFF is big-endian
                    break;
                case 24:
                    // 24-bit audio needs special handling
                    // We'll handle it manually in do_decode
                    m_converter = nullptr;
                    return;
                case 32:
                    format = audio_format::s32be;
                    break;
                default:
                    THROW_RUNTIME("Unsupported sample size: ", m_sample_size);
            }
        } else if (m_compression_type == COMP_SOWT) {
            // Little-endian 16-bit (byte-swapped)
            if (m_sample_size != 16) {
                THROW_RUNTIME("sowt compression requires 16-bit samples");
            }
            format = audio_format::s16le;  // Little-endian!
        } else if (m_compression_type == COMP_FL32) {
            format = audio_format::f32be;
        } else if (m_compression_type == COMP_FL64) {
            // 64-bit float needs special handling
            m_converter = nullptr;
            return;
        } else if (m_compression_type == COMP_ULAW || m_compression_type == COMP_ulaw ||
                   m_compression_type == COMP_ALAW || m_compression_type == COMP_alaw) {
            // G.711 compression - these need special handling
            // The sample_size is typically 8 bits for compressed data
            m_converter = nullptr;
            return;
        } else if (m_compression_type == COMP_IMA4) {
            // IMA4 ADPCM compression - needs special handling
            // IMA4 encodes 64 samples in 34 bytes per channel
            m_converter = nullptr;
            return;
        } else {
            // Report the specific unsupported compression type
            char comp_str[5] = {0};
            uint32_t comp = iff::swap32be(m_compression_type.to_uint32());
            comp_str[0] = (comp >> 24) & 0xFF;
            comp_str[1] = (comp >> 16) & 0xFF;
            comp_str[2] = (comp >> 8) & 0xFF;
            comp_str[3] = comp & 0xFF;
            THROW_RUNTIME("Unsupported compression type: '", comp_str, "'");
        }
        
        m_converter = get_to_float_conveter(format);
        if (!m_converter) {
            THROW_RUNTIME("No converter for format");
        }
    }
    
    // Manual conversion for 12-bit PCM (packed: 2 samples in 3 bytes)
    void convert_12bit_to_float(float* dst, const uint8_t* src, size_t samples) {
        size_t src_idx = 0;
        for (size_t i = 0; i < samples; i += 2) {
            // First sample: byte0 + high nibble of byte1
            int32_t sample1 = (static_cast<int32_t>(src[src_idx]) << 24) |
                             (static_cast<int32_t>(src[src_idx + 1] & 0xF0) << 16);
            sample1 = sample1 >> 20;  // Shift to get 12-bit value with sign extension
            if (sample1 & 0x800) sample1 |= 0xFFFFF000;  // Sign extend
            dst[i] = static_cast<float>(sample1) / 2048.0f;
            
            if (i + 1 < samples) {
                // Second sample: low nibble of byte1 + byte2
                int32_t sample2 = (static_cast<int32_t>(src[src_idx + 1] & 0x0F) << 28) |
                                 (static_cast<int32_t>(src[src_idx + 2]) << 20);
                sample2 = sample2 >> 20;  // Shift to get 12-bit value with sign extension
                if (sample2 & 0x800) sample2 |= 0xFFFFF000;  // Sign extend
                dst[i + 1] = static_cast<float>(sample2) / 2048.0f;
            }
            src_idx += 3;
        }
    }
    
    // Manual conversion for 24-bit PCM
    void convert_24bit_to_float(float* dst, const uint8_t* src, size_t samples) {
        for (size_t i = 0; i < samples; ++i) {
            // Read 24-bit big-endian sample
            int32_t sample = (static_cast<int32_t>(src[i * 3]) << 24) |
                           (static_cast<int32_t>(src[i * 3 + 1]) << 16) |
                           (static_cast<int32_t>(src[i * 3 + 2]) << 8);
            // Sign extend and normalize to float
            dst[i] = static_cast<float>(sample) / 2147483648.0f;
        }
    }
    
    // Manual conversion for 64-bit float
    void convert_f64be_to_float(float* dst, const uint8_t* src, size_t samples) {
        for (size_t i = 0; i < samples; ++i) {
            // Read 64-bit big-endian double
            uint64_t bits = 0;
            for (int j = 0; j < 8; ++j) {
                bits = (bits << 8) | src[i * 8 + j];
            }
            
            // Convert from big-endian if on little-endian system
            if (iff::is_little_endian) {
                bits = iff::swap64(bits);
            }
            
            double value;
            std::memcpy(&value, &bits, sizeof(double));
            dst[i] = static_cast<float>(value);
        }
    }
    
    // G.711 µ-law decompression
    void convert_ulaw_to_float(float* dst, const uint8_t* src, size_t samples) {
        for (size_t i = 0; i < samples; ++i) {
            // µ-law uses the lookup table directly
            int16_t pcm_value = ulaw_table[src[i]];
            // Convert 16-bit PCM to float [-1.0, 1.0]
            dst[i] = static_cast<float>(pcm_value) / 32768.0f;
        }
    }
    
    // G.711 A-law decompression
    void convert_alaw_to_float(float* dst, const uint8_t* src, size_t samples) {
        for (size_t i = 0; i < samples; ++i) {
            // A-law uses the lookup table directly
            int16_t pcm_value = alaw_table[src[i]];
            // Convert 16-bit PCM to float [-1.0, 1.0]
            dst[i] = static_cast<float>(pcm_value) / 32768.0f;
        }
    }
    
    // IMA4 ADPCM decompression
    class IMA4Decoder {
        int32_t predictor = 0;
        int8_t step_index = 0;
        
        int16_t decode_sample(uint8_t nibble) {
            int step = ima_step_table[step_index];
            int diff = step >> 3;
            
            if (nibble & 4) diff += step;
            if (nibble & 2) diff += step >> 1;
            if (nibble & 1) diff += step >> 2;
            
            if (nibble & 8) {
                predictor -= diff;
            } else {
                predictor += diff;
            }
            
            // Clamp predictor to 16-bit range
            if (predictor > 32767) predictor = 32767;
            else if (predictor < -32768) predictor = -32768;
            
            // Update step index
            step_index += ima_index_table[nibble];
            if (step_index < 0) step_index = 0;
            else if (step_index > 88) step_index = 88;
            
            return static_cast<int16_t>(predictor);
        }
        
    public:
        void reset(int16_t initial_predictor, int8_t initial_index) {
            predictor = initial_predictor;
            step_index = initial_index;
        }
        
        // Decode IMA4 block (34 bytes for mono, 68 bytes for stereo per channel)
        void decode_block(const uint8_t* src, int16_t* dst, size_t channel_blocks) {
            for (size_t ch = 0; ch < channel_blocks; ++ch) {
                const uint8_t* block = src + ch * 34;
                int16_t* out = dst + ch * 64;
                
                // Read block header (big-endian)
                int16_t initial_sample = iff::swap16be(*reinterpret_cast<const uint16_t*>(block));
                int8_t initial_index = block[2] & 0x7F;  // Only 7 bits used
                
                reset(initial_sample, initial_index);
                out[0] = initial_sample;
                
                // Decode 63 samples (stored as nibbles)
                const uint8_t* data = block + 4;
                for (int i = 0; i < 31; ++i) {
                    uint8_t byte = data[i];
                    // High nibble first (big-endian nibble order)
                    out[i * 2 + 1] = decode_sample((byte >> 4) & 0x0F);
                    out[i * 2 + 2] = decode_sample(byte & 0x0F);
                }
                // Last byte has only one sample
                out[63] = decode_sample((data[31] >> 4) & 0x0F);
            }
        }
    };
    
    // Convert IMA4 to float
    void convert_ima4_to_float(float* dst, const uint8_t* src, size_t samples, size_t channels) {
        IMA4Decoder decoder;
        std::vector<int16_t> pcm_buffer(64 * channels);
        
        size_t frames = samples / channels;  // Convert samples to frames
        size_t frames_processed = 0;
        size_t src_offset = 0;
        
        while (frames_processed < frames) {
            // Decode one block (64 frames/samples per channel)
            size_t block_size = 34 * channels;
            decoder.decode_block(src + src_offset, pcm_buffer.data(), channels);
            
            // Convert to float and interleave channels
            size_t frames_in_block = std::min(size_t(64), frames - frames_processed);
            for (size_t f = 0; f < frames_in_block; ++f) {
                for (size_t ch = 0; ch < channels; ++ch) {
                    dst[(frames_processed + f) * channels + ch] = 
                        pcm_buffer[ch * 64 + f] / 32768.0f;
                }
            }
            frames_processed += frames_in_block;
            
            src_offset += block_size;
        }
    }
    
    // Stream adapter for libiff
    class StreamAdapter : public std::istream {
        struct StreamBuf : public std::streambuf {
            io_stream* m_stream;
            
            StreamBuf(io_stream* stream) : m_stream(stream) {}
            
            int_type underflow() override {
                char ch;
                if (m_stream->read(&ch, 1) == 1) {
                    setg(&ch, &ch, &ch + 1);
                    return traits_type::to_int_type(ch);
                }
                return traits_type::eof();
            }
            
            pos_type seekoff(off_type off, std::ios_base::seekdir dir, 
                           std::ios_base::openmode) override {
                seek_origin origin;
                switch (dir) {
                    case std::ios_base::beg: origin = seek_origin::set; break;
                    case std::ios_base::cur: origin = seek_origin::cur; break;
                    case std::ios_base::end: origin = seek_origin::end; break;
                    default: return pos_type(off_type(-1));
                }
                return m_stream->seek(off, origin);
            }
            
            pos_type seekpos(pos_type pos, std::ios_base::openmode) override {
                return m_stream->seek(pos, seek_origin::set);
            }
        };
        
        StreamBuf m_buf;
        
    public:
        StreamAdapter(io_stream* stream) : std::istream(&m_buf), m_buf(stream) {}
    };
};

// Public interface implementation

decoder_aiff_v2::decoder_aiff_v2() : m_pimpl(std::make_unique<impl>()) {}

decoder_aiff_v2::~decoder_aiff_v2() = default;

bool decoder_aiff_v2::accept(io_stream* rwops) {
    if (!rwops) return false;
    
    auto pos = rwops->tell();
    if (pos < 0) return false;
    
    // Check for FORM chunk
    char chunk_id[4];
    uint32_t size;
    char type_id[4];
    
    if (rwops->read(chunk_id, 4) != 4) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    if (iff::fourcc(chunk_id) != FORM_ID) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    // Skip size
    if (rwops->read(&size, 4) != 4) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    // Check for AIFF or AIFC type
    if (rwops->read(type_id, 4) != 4) {
        rwops->seek(pos, seek_origin::set);
        return false;
    }
    
    iff::fourcc form_type(type_id);
    bool is_aiff = (form_type == AIFF_ID || form_type == AIFC_ID);
    
    rwops->seek(pos, seek_origin::set);
    return is_aiff;
}

const char* decoder_aiff_v2::get_name() const {
    return m_pimpl->m_is_aifc ? "AIFF-C" : "AIFF";
}

void decoder_aiff_v2::open(io_stream* rwops) {
    m_pimpl->m_stream = rwops;
    m_pimpl->parse_file();
    m_pimpl->m_is_open = true;
    set_is_open(true);
}

channels_t decoder_aiff_v2::get_channels() const {
    return m_pimpl->m_num_channels;
}

sample_rate_t decoder_aiff_v2::get_rate() const {
    return static_cast<sample_rate_t>(m_pimpl->m_sample_rate);
}

bool decoder_aiff_v2::rewind() {
    m_pimpl->m_current_frame = 0;
    return true;
}

std::chrono::microseconds decoder_aiff_v2::duration() const {
    if (!is_open() || m_pimpl->m_sample_rate == 0) {
        return std::chrono::microseconds(0);
    }
    
    double duration_seconds = static_cast<double>(m_pimpl->m_num_sample_frames) / 
                             m_pimpl->m_sample_rate;
    
    return std::chrono::microseconds(
        static_cast<int64_t>(duration_seconds * 1'000'000)
    );
}

bool decoder_aiff_v2::seek_to_time(std::chrono::microseconds pos) {
    if (!is_open() || m_pimpl->m_sample_rate == 0) {
        return false;
    }
    
    double seconds = pos.count() / 1'000'000.0;
    size_t target_frame = static_cast<size_t>(
        seconds * m_pimpl->m_sample_rate
    );
    
    // Clamp to valid range
    if (target_frame >= m_pimpl->m_num_sample_frames) {
        target_frame = m_pimpl->m_num_sample_frames;
    }
    
    // Check if we have a marker near the target position
    // This could be optimized for faster seeking
    int64_t closest_marker = -1;
    uint16_t closest_id = 0;
    for (const auto& [id, marker] : m_pimpl->m_markers) {
        if (marker.position <= target_frame) {
            if (closest_marker < 0 || 
                (target_frame - marker.position) < (target_frame - closest_marker)) {
                closest_marker = marker.position;
                closest_id = id;
            }
        }
    }
    
    // Update current position
    m_pimpl->m_current_frame = target_frame;
    
    // Clear any cached decode buffer
    m_pimpl->m_decode_buffer.clear();
    
    return true;
}

bool decoder_aiff_v2::is_compressed() const {
    return m_pimpl->m_is_aifc && m_pimpl->m_compression_type != COMP_NONE;
}

uint32_t decoder_aiff_v2::get_compression_type() const {
    return m_pimpl->m_compression_type.to_uint32();
}

int64_t decoder_aiff_v2::get_marker_position(uint16_t marker_id) const {
    auto it = m_pimpl->m_markers.find(marker_id);
    if (it != m_pimpl->m_markers.end()) {
        return it->second.position;
    }
    return -1;
}

bool decoder_aiff_v2::has_instrument_data() const {
    return m_pimpl->m_has_instrument;
}

int decoder_aiff_v2::get_base_note() const {
    if (!m_pimpl->m_has_instrument) {
        return -1;
    }
    return m_pimpl->m_instrument.base_note;
}

int decoder_aiff_v2::get_detune() const {
    if (!m_pimpl->m_has_instrument) {
        return 0;
    }
    return m_pimpl->m_instrument.detune;
}

int decoder_aiff_v2::get_gain_db() const {
    if (!m_pimpl->m_has_instrument) {
        return 0;
    }
    return m_pimpl->m_instrument.gain;
}

bool decoder_aiff_v2::get_sustain_loop(int& mode, uint16_t& start_marker_id, uint16_t& end_marker_id) const {
    if (!m_pimpl->m_has_instrument) {
        return false;
    }
    
    mode = m_pimpl->m_instrument.sustain_loop.play_mode;
    start_marker_id = m_pimpl->m_instrument.sustain_loop.begin_loop;
    end_marker_id = m_pimpl->m_instrument.sustain_loop.end_loop;
    
    // Check if loop is valid (mode > 0 and valid marker IDs)
    return mode > 0 && start_marker_id > 0 && end_marker_id > 0;
}

bool decoder_aiff_v2::get_release_loop(int& mode, uint16_t& start_marker_id, uint16_t& end_marker_id) const {
    if (!m_pimpl->m_has_instrument) {
        return false;
    }
    
    mode = m_pimpl->m_instrument.release_loop.play_mode;
    start_marker_id = m_pimpl->m_instrument.release_loop.begin_loop;
    end_marker_id = m_pimpl->m_instrument.release_loop.end_loop;
    
    // Check if loop is valid (mode > 0 and valid marker IDs)
    return mode > 0 && start_marker_id > 0 && end_marker_id > 0;
}

std::vector<uint16_t> decoder_aiff_v2::get_marker_ids() const {
    std::vector<uint16_t> ids;
    ids.reserve(m_pimpl->m_markers.size());
    
    for (const auto& [id, marker] : m_pimpl->m_markers) {
        ids.push_back(id);
    }
    
    return ids;
}

std::string decoder_aiff_v2::get_marker_name(uint16_t marker_id) const {
    auto it = m_pimpl->m_markers.find(marker_id);
    if (it != m_pimpl->m_markers.end()) {
        return it->second.name;
    }
    return "";
}

size_t decoder_aiff_v2::do_decode(float* buf, size_t len, bool& call_again) {
    if (!is_open() || m_pimpl->m_current_frame >= m_pimpl->m_num_sample_frames) {
        call_again = false;
        return 0;
    }
    
    // Check if we have audio data loaded
    if (m_pimpl->m_audio_data.empty()) {
        THROW_RUNTIME("No audio data available - SSND chunk not found or empty");
    }
    
    // Calculate frames to decode
    size_t frames_remaining = m_pimpl->m_num_sample_frames - m_pimpl->m_current_frame;
    size_t frames_to_decode = std::min(len / m_pimpl->m_num_channels, frames_remaining);
    size_t samples_to_decode = frames_to_decode * m_pimpl->m_num_channels;
    
    // Calculate bytes to read (special cases for 12-bit and IMA4)
    size_t bytes_per_sample = (m_pimpl->m_sample_size + 7) / 8;
    size_t bytes_to_read;
    if (m_pimpl->m_sample_size == 12) {
        // 12-bit samples are packed: 2 samples in 3 bytes
        bytes_to_read = (samples_to_decode * 3 + 1) / 2;
    } else if (m_pimpl->m_compression_type == COMP_IMA4) {
        // IMA4: 64 samples per channel in 34 bytes per channel
        // For IMA4, we need to decode in 64-sample blocks
        // Adjust frames_to_decode to block boundary
        size_t blocks_per_channel = (frames_to_decode + 63) / 64;
        frames_to_decode = std::min(blocks_per_channel * 64, frames_remaining);
        samples_to_decode = frames_to_decode * m_pimpl->m_num_channels;
        
        // CRITICAL: Ensure we don't exceed the buffer size
        if (samples_to_decode > len) {
            // Reduce by one block if we would overflow
            if (blocks_per_channel > 0) {
                blocks_per_channel--;
                frames_to_decode = blocks_per_channel * 64;
                samples_to_decode = frames_to_decode * m_pimpl->m_num_channels;
            }
            if (samples_to_decode > len) {
                // Still too big? This shouldn't happen with reasonable buffer sizes
                samples_to_decode = 0;
                frames_to_decode = 0;
                bytes_to_read = 0;
            } else {
                bytes_to_read = blocks_per_channel * 34 * m_pimpl->m_num_channels;
            }
        } else {
            bytes_to_read = blocks_per_channel * 34 * m_pimpl->m_num_channels;
        }
    } else {
        bytes_to_read = samples_to_decode * bytes_per_sample;
    }
    
    // Check if we have enough data
    size_t bytes_available = m_pimpl->m_audio_data.size() - m_pimpl->m_audio_read_pos;
    if (bytes_to_read > bytes_available) {
        bytes_to_read = bytes_available;
        samples_to_decode = bytes_to_read / bytes_per_sample;
        frames_to_decode = samples_to_decode / m_pimpl->m_num_channels;
    }
    
    if (bytes_to_read == 0) {
        call_again = false;
        return 0;
    }
    
    // Get pointer to audio data at current position
    const uint8_t* audio_ptr = m_pimpl->m_audio_data.data() + m_pimpl->m_audio_read_pos;
    
    // Convert to float
    if (m_pimpl->m_converter) {
        m_pimpl->m_converter(buf, audio_ptr, samples_to_decode);
    } else if (m_pimpl->m_sample_size == 12) {
        // Handle 12-bit PCM manually (packed format)
        m_pimpl->convert_12bit_to_float(buf, audio_ptr, samples_to_decode);
    } else if (m_pimpl->m_sample_size == 24) {
        // Handle 24-bit PCM manually
        m_pimpl->convert_24bit_to_float(buf, audio_ptr, samples_to_decode);
    } else if (m_pimpl->m_compression_type == COMP_FL64) {
        // Handle 64-bit float manually
        m_pimpl->convert_f64be_to_float(buf, audio_ptr, samples_to_decode);
    } else if (m_pimpl->m_compression_type == COMP_ULAW || m_pimpl->m_compression_type == COMP_ulaw) {
        // Handle µ-law decompression (both uppercase and lowercase)
        m_pimpl->convert_ulaw_to_float(buf, audio_ptr, samples_to_decode);
    } else if (m_pimpl->m_compression_type == COMP_ALAW || m_pimpl->m_compression_type == COMP_alaw) {
        // Handle A-law decompression (both uppercase and lowercase)
        m_pimpl->convert_alaw_to_float(buf, audio_ptr, samples_to_decode);
    } else if (m_pimpl->m_compression_type == COMP_IMA4) {
        // Handle IMA4 ADPCM decompression
        m_pimpl->convert_ima4_to_float(buf, audio_ptr, samples_to_decode, m_pimpl->m_num_channels);
    } else {
        THROW_RUNTIME("No converter available for format");
    }
    
    // Update position
    m_pimpl->m_audio_read_pos += bytes_to_read;
    m_pimpl->m_current_frame += frames_to_decode;
    call_again = (m_pimpl->m_current_frame < m_pimpl->m_num_sample_frames);
    
    return samples_to_decode;
}

} // namespace musac
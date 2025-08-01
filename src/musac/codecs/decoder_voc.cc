//
// Created by igor on 3/18/25.
//

#include <musac/codecs/decoder_voc.hh>
#include <musac/sdk/samples_converter.hh>
#include <SDL3/SDL_endian.h>
#include <failsafe/failsafe.hh>
#include <vector>
#include <cstring>
#include <algorithm>

namespace musac {

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

static constexpr Uint32 VOC_BAD_RATE = ~((Uint32)0);

struct vocstuff {
    Uint32 rest;           // bytes remaining in current block
    Uint32 rate;           // rate code (byte) of this chunk
    int silent;            // sound or silence?
    Uint32 srate;          // rate code (byte) of silence
    Uint32 blockseek;      // start of current output block
    Uint32 samples;        // number of samples output
    Uint32 size;           // word length of data
    Uint8 channels;        // number of sound channels
    int has_extended;      // Has an extended block been read?
};

struct decoder_voc::impl {
    SDL_IOStream* m_rwops = nullptr;
    SDL_AudioSpec m_spec = {};
    std::vector<Uint8> m_buffer;
    size_t m_total_samples = 0;
    size_t m_consumed = 0;
    to_float_converter_func_t m_converter = nullptr;
    
    bool check_header() {
        // VOC magic header
        Uint8 signature[20];  // "Creative Voice File\032"
        Uint16 datablockofs;
        
        SDL_SeekIO(m_rwops, 0, SDL_IO_SEEK_SET);
        
        if (SDL_ReadIO(m_rwops, signature, sizeof(signature)) != sizeof(signature)) {
            return false;
        }
        
        if (SDL_memcmp(signature, "Creative Voice File\032", sizeof(signature)) != 0) {
            THROW_RUNTIME("Unrecognized file type (not VOC)");
        }
        
        // get the offset where the first datablock is located
        if (SDL_ReadIO(m_rwops, &datablockofs, sizeof(Uint16)) != sizeof(Uint16)) {
            return false;
        }
        
        datablockofs = SDL_Swap16LE(datablockofs);
        
        if (SDL_SeekIO(m_rwops, datablockofs, SDL_IO_SEEK_SET) != datablockofs) {
            return false;
        }
        
        return true;
    }
    
    bool get_block(vocstuff& v, SDL_AudioSpec& spec) {
        Uint8 bits24[3];
        Uint8 uc, block;
        Uint32 sblen;
        Uint16 new_rate_short;
        Uint32 new_rate_long;
        Uint8 trash[6];
        Uint16 period;
        
        v.silent = 0;
        while (v.rest == 0) {
            if (SDL_ReadIO(m_rwops, &block, sizeof(block)) != sizeof(block)) {
                return true;  // assume that's the end of the file.
            }
            
            if (block == VOC_TERM) {
                return true;
            }
            
            if (SDL_ReadIO(m_rwops, bits24, sizeof(bits24)) != sizeof(bits24)) {
                return true;  // assume that's the end of the file.
            }
            
            // Size is an 24-bit value. Ugh.
            sblen = (Uint32)((bits24[0]) | (bits24[1] << 8) | (bits24[2] << 16));
            
            switch(block) {
                case VOC_DATA:
                    if (SDL_ReadIO(m_rwops, &uc, sizeof(uc)) != sizeof(uc)) {
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
                        spec.freq = (Uint16)(1000000.0/(256 - v.rate));
                        v.channels = 1;
                    }
                    
                    if (SDL_ReadIO(m_rwops, &uc, sizeof(uc)) != sizeof(uc)) {
                        return false;
                    }
                    
                    if (uc != 0) {
                        THROW_RUNTIME("VOC decoder only interprets 8-bit data");
                    }
                    
                    v.has_extended = 0;
                    v.rest = sblen - 2;
                    v.size = ST_SIZE_BYTE;
                    return true;
                    
                case VOC_DATA_16:
                    if (SDL_ReadIO(m_rwops, &new_rate_long, sizeof(new_rate_long)) != sizeof(new_rate_long)) {
                        return false;
                    }
                    new_rate_long = SDL_Swap32LE(new_rate_long);
                    if (new_rate_long == 0) {
                        THROW_RUNTIME("VOC Sample rate is zero?");
                    }
                    if ((v.rate != VOC_BAD_RATE) && (new_rate_long != v.rate)) {
                        THROW_RUNTIME("VOC sample rate codes differ");
                    }
                    v.rate = new_rate_long;
                    spec.freq = (int)new_rate_long;
                    
                    if (SDL_ReadIO(m_rwops, &uc, sizeof(uc)) != sizeof(uc)) {
                        return false;
                    }
                    
                    switch (uc) {
                        case 8:  v.size = ST_SIZE_BYTE; break;
                        case 16: v.size = ST_SIZE_WORD; break;
                        default:
                            THROW_RUNTIME("VOC with unknown data size");
                    }
                    
                    if (SDL_ReadIO(m_rwops, &v.channels, sizeof(Uint8)) != sizeof(Uint8)) {
                        return false;
                    }
                    
                    if (SDL_ReadIO(m_rwops, trash, 6) != 6) {
                        return false;
                    }
                    
                    v.rest = sblen - 12;
                    return true;
                    
                case VOC_CONT:
                    v.rest = sblen;
                    return true;
                    
                case VOC_SILENCE:
                    if (SDL_ReadIO(m_rwops, &period, sizeof(period)) != sizeof(period)) {
                        return false;
                    }
                    period = SDL_Swap16LE(period);
                    
                    if (SDL_ReadIO(m_rwops, &uc, sizeof(uc)) != sizeof(uc)) {
                        return false;
                    }
                    if (uc == 0) {
                        THROW_RUNTIME("VOC silence sample rate is zero");
                    }
                    
                    // Some silence-packed files have gratuitously
                    // different sample rate codes in silence.
                    // Adjust period.
                    if ((v.rate != VOC_BAD_RATE) && (uc != v.rate))
                        period = (Uint16)((period * (256 - uc))/(256 - v.rate));
                    else
                        v.rate = uc;
                    v.rest = period;
                    v.silent = 1;
                    return true;
                    
                case VOC_LOOP:
                case VOC_LOOPEND:
                    for (unsigned int i = 0; i < sblen; i++) { // skip repeat loops.
                        if (SDL_ReadIO(m_rwops, trash, sizeof(Uint8)) != sizeof(Uint8)) {
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
                    if (SDL_ReadIO(m_rwops, &new_rate_short, sizeof(new_rate_short)) != sizeof(new_rate_short)) {
                        return false;
                    }
                    new_rate_short = SDL_Swap16LE(new_rate_short);
                    if (new_rate_short == 0) {
                        THROW_RUNTIME("VOC sample rate is zero");
                    }
                    if ((v.rate != VOC_BAD_RATE) && (new_rate_short != v.rate)) {
                        THROW_RUNTIME("VOC sample rate codes differ");
                    }
                    v.rate = new_rate_short;
                    
                    if (SDL_ReadIO(m_rwops, &uc, sizeof(uc)) != sizeof(uc)) {
                        return false;
                    }
                    
                    if (uc != 0) {
                        THROW_RUNTIME("VOC decoder only interprets 8-bit data");
                    }
                    
                    if (SDL_ReadIO(m_rwops, &uc, sizeof(uc)) != sizeof(uc)) {
                        return false;
                    }
                    
                    if (uc) // Stereo
                        spec.channels = 2;
                    else
                        spec.channels = 1;
                    // Needed number of channels before finishing
                    // compute for rate
                    spec.freq = (256000000L / (65536L - v.rate)) / spec.channels;
                    // An extended block must be followed by a data
                    // block to be valid so loop back to top so it
                    // can be grabed.
                    continue;
                    
                case VOC_MARKER:
                    if (SDL_ReadIO(m_rwops, trash, 2) != 2) {
                        return false;
                    }
                    // fallthrough
                    
                default:  // text block or other krapola.
                    for (unsigned int i = 0; i < sblen; i++) {
                        if (SDL_ReadIO(m_rwops, trash, sizeof(Uint8)) != sizeof(Uint8)) {
                            return false;
                        }
                    }
                    
                    if (block == VOC_TEXT) {
                        continue;    // get next block
                    }
            }
        }
        
        return true;
    }
    
    Uint32 voc_read(vocstuff& v, Uint8* buf, Uint32 buf_size, SDL_AudioSpec& spec) {
        Sint64 done = 0;
        Uint8 silence = 0x80;
        
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
            Uint32 to_fill = (v.rest < buf_size) ? v.rest : buf_size;
            SDL_memset(buf, silence, to_fill);
            done = to_fill;
            v.rest -= to_fill;
        }
        else {
            Uint32 to_read = (v.rest < buf_size) ? v.rest : buf_size;
            done = SDL_ReadIO(m_rwops, buf, to_read);
            if (done <= 0) {
                return 0;
            }
            
            v.rest = (Uint32)(v.rest - done);
            if (v.size == ST_SIZE_WORD) {
                #if (SDL_BYTEORDER == SDL_BIG_ENDIAN)
                Uint16 *samples = (Uint16 *)buf;
                size_t sample_count = done / 2;
                for (size_t i = 0; i < sample_count; i++) {
                    samples[i] = SDL_Swap16LE(samples[i]);
                }
                #endif
                done >>= 1;
            }
        }
        
        return (Uint32)done;
    }
    
    bool load_voc() {
        if (!m_rwops) {
            return false;
        }
        
        if (!check_header()) {
            return false;
        }
        
        vocstuff v;
        SDL_zero(v);
        v.rate = VOC_BAD_RATE;
        v.rest = 0;
        v.has_extended = 0;
        SDL_zero(m_spec);
        
        if (!get_block(v, m_spec)) {
            return false;
        }
        
        if (v.rate == VOC_BAD_RATE) {
            THROW_RUNTIME("VOC data had no sound!");
        }
        
        if (v.size == 0) {
            THROW_RUNTIME("VOC data had invalid word size!");
        }
        
        m_spec.format = ((v.size == ST_SIZE_WORD) ? SDL_AUDIO_S16 : SDL_AUDIO_U8);
        if (m_spec.channels == 0) {
            m_spec.channels = v.channels;
        }
        
        // Read all audio data
        std::vector<Uint8> temp_buffer;
        const size_t chunk_size = 4096;
        Uint8 chunk[chunk_size];
        
        while (true) {
            Uint32 read = voc_read(v, chunk, chunk_size, m_spec);
            if (read == 0) {
                // Try to get next block
                if (!get_block(v, m_spec)) {
                    break;
                }
                // Check if we still have no data after get_block
                if (v.rest == 0) {
                    break;  // No more data available
                }
                continue;
            }
            
            // voc_read returns samples for 16-bit, bytes for 8-bit
            size_t bytes_read = (v.size == ST_SIZE_WORD) ? read * 2 : read;
            temp_buffer.insert(temp_buffer.end(), chunk, chunk + bytes_read);
        }
        
        // Don't return a buffer that isn't a multiple of samplesize
        int samplesize = ((m_spec.format & 0xFF)/8) * m_spec.channels;
        size_t total_size = temp_buffer.size() & ~(samplesize - 1);
        temp_buffer.resize(total_size);
        
        // Convert to standard format (S16 mono 44100Hz)
        SDL_AudioSpec dst_spec = { SDL_AUDIO_S16, 1, 44100 };
        Uint8* dst_data = nullptr;
        int dst_len = 0;
        
        if (!temp_buffer.empty()) {
            if (!SDL_ConvertAudioSamples(&m_spec, temp_buffer.data(), (int)temp_buffer.size(), 
                                         &dst_spec, &dst_data, &dst_len)) {
                THROW_RUNTIME("Failed to convert audio samples: ", SDL_GetError());
            }
        } else {
            THROW_RUNTIME("No audio data read from VOC file");
        }
        
        // Store converted data
        m_buffer.assign(dst_data, dst_data + dst_len);
        SDL_free(dst_data);
        
        m_spec = dst_spec;
        m_total_samples = dst_len / sizeof(int16_t);
        m_converter = get_to_float_conveter(m_spec.format);
        
        return true;
    }
};

decoder_voc::decoder_voc() : m_pimpl(std::make_unique<impl>()) {}

decoder_voc::~decoder_voc() = default;

bool decoder_voc::open(SDL_IOStream* rwops) {
    m_pimpl->m_rwops = rwops;
    
    try {
        if (!m_pimpl->load_voc()) {
            return false;
        }
        set_is_open(true);
        m_pimpl->m_consumed = 0;
        return true;
    } catch (const std::exception& e) {
        // Return false on error instead of throwing
        return false;
    }
}

unsigned int decoder_voc::get_channels() const {
    return m_pimpl->m_spec.channels;
}

unsigned int decoder_voc::get_rate() const {
    return m_pimpl->m_spec.freq;
}

bool decoder_voc::rewind() {
    m_pimpl->m_consumed = 0;
    return true;
}

std::chrono::microseconds decoder_voc::duration() const {
    return std::chrono::microseconds(0);
}

bool decoder_voc::seek_to_time(std::chrono::microseconds) {
    return false;
}

unsigned int decoder_voc::do_decode(float* buf, unsigned int len, bool& call_again) {
    auto remains = m_pimpl->m_total_samples - m_pimpl->m_consumed;
    auto take = std::min(remains, (size_t)len);
    
    if (take > 0) {
        // m_buffer contains int16_t samples stored as bytes
        const Uint8* sample_ptr = m_pimpl->m_buffer.data() + (m_pimpl->m_consumed * sizeof(int16_t));
        m_pimpl->m_converter(buf, sample_ptr, take);
        m_pimpl->m_consumed += take;
    }
    
    call_again = false;
    return take;
}

} // namespace musac
/**
 * @file decoder_mml.hh
 * @brief Music Macro Language (MML) decoder
 * @ingroup codecs
 */

#ifndef MUSAC_CODECS_DECODER_MML_HH
#define MUSAC_CODECS_DECODER_MML_HH

#include <musac/sdk/decoder.hh>
#include <musac/codecs/export_musac_codecs.h>
#include <musac/sdk/mml_parser.hh>
#include <deque>
#include <memory>
#include <mutex>

namespace musac {

/*!
 * \brief Music Macro Language (MML) decoder
 * \ingroup codecs
 * 
 * MML is a text-based music description language originally developed by Microsoft
 * for their BASIC implementations (GW-BASIC, BASICA, QBASIC). It allows programming
 * of musical sequences using simple ASCII commands, originally designed for PC Speaker
 * output in early personal computers.
 * 
 * ## Historical Context
 * MML was introduced in the early 1980s for Japanese personal computers and became
 * the standard for PC-compatible computers through GW-BASIC. It was commonly used in:
 * - MS-DOS games and educational software
 * - BASIC programming tutorials
 * - Early computer music composition
 * - Sound effects in business applications
 * 
 * ## Command Reference
 * 
 * ### Note Commands
 * - **A-G**: Play note (A through G)
 * - **A-G[n]**: Play note with specific length (e.g., C8 = eighth note)
 * - **#** or **+**: Sharp modifier (follows note)
 * - **-**: Flat modifier (follows note)
 * - **R** or **P**: Rest/Pause
 * 
 * ### Note Lengths
 * - **1**: Whole note
 * - **2**: Half note
 * - **4**: Quarter note (default)
 * - **8**: Eighth note
 * - **16**: Sixteenth note
 * - **32**: Thirty-second note
 * - **64**: Sixty-fourth note
 * - **.**: Dotted note (1.5x length)
 * - **..**: Double-dotted (1.75x length)
 * 
 * ### Octave Control
 * - **O[n]**: Set octave (O0-O6, middle C at start of O3)
 * - **<**: Down one octave (before note)
 * - **>**: Up one octave (before note)
 * 
 * ### Tempo and Timing
 * - **T[n]**: Set tempo (32-255 BPM, default 120)
 * - **L[n]**: Set default note length (1-64, default 4)
 * 
 * ### Volume and Articulation
 * - **V[n]**: Set volume (0-15, default 10)
 * - **ML**: Legato mode (notes play full length)
 * - **MN**: Normal mode (notes play 7/8 length, default)
 * - **MS**: Staccato mode (notes play 3/4 length)
 * 
 * ### Advanced Commands
 * - **N[n]**: Play note by number (0-84, 0=rest)
 * - **MF**: Music Foreground mode
 * - **MB**: Music Background mode (for BASIC compatibility)
 * 
 * ## Wave Generation
 * This decoder generates authentic PC Speaker-style square wave audio:
 * - Monophonic square wave synthesis
 * - 44100 Hz sample rate output
 * - Stereo output (duplicated channels for compatibility)
 * - Authentic articulation gaps between notes
 * 
 * ## File Format
 * MML files are plain text files, typically with .mml extension:
 * - ASCII text encoding
 * - Commands are case-insensitive
 * - Whitespace is ignored
 * - No formal file header
 * 
 * ## Example MML Strings
 * 
 * **Mary Had a Little Lamb:**
 * ```
 * T120 L4 E D C D E E E2 D D D2 E G G2
 * ```
 * 
 * **Ode to Joy:**
 * ```
 * O2 T120 E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 E8. D16 D4
 * ```
 * 
 * **Scale with Articulation:**
 * ```
 * T180 L8 V12 MS C D E F G A B >C< ML C D E F G A B >C
 * ```
 * 
 * ## Limitations
 * - Monophonic only (single voice)
 * - Square wave synthesis only
 * - Limited dynamic range
 * - No effects or modulation
 * - Commands limited to original BASIC implementation
 * 
 * ## Example Usage
 * \code{.cpp}
 * auto io = musac::io_from_file("melody.mml");
 * musac::decoder_mml decoder;
 * decoder.open(io.get());
 * 
 * std::cout << "Channels: " << decoder.get_channels() << "\n";  // Always 2
 * std::cout << "Sample rate: " << decoder.get_rate() << " Hz\n";  // 44100
 * 
 * // MML files have finite duration
 * auto duration = decoder.duration();
 * std::cout << "Duration: " << duration.count() / 1000000.0 << " seconds\n";
 * 
 * // Check for parse warnings
 * auto warnings = decoder.get_warnings();
 * for (const auto& warning : warnings) {
 *     std::cerr << "Warning: " << warning << "\n";
 * }
 * 
 * // Decode and play
 * float buffer[4096];
 * bool more_data = true;
 * while (more_data) {
 *     size_t decoded = decoder.decode(buffer, 4096, more_data);
 *     // Process decoded samples...
 * }
 * \endcode
 * 
 * \note MML format is ideal for retro-style music, chiptune compositions,
 * and authentic PC Speaker sound recreation.
 * 
 * \see pc_speaker_stream For real-time MML playback
 * \see decoder Base class for all decoders
 * \see decoders_registry For automatic format detection
 */
class MUSAC_CODECS_EXPORT decoder_mml : public decoder {
public:
    decoder_mml();
    ~decoder_mml() override;
    
    /*!
     * \brief Check if a stream contains MML data
     * \param stream The stream to check
     * \return true if the stream contains MML commands
     * 
     * Performs heuristic detection by looking for MML command patterns
     * in the text. Checks for common commands like T (tempo), O (octave),
     * L (length), and note letters A-G. The stream position is restored
     * after checking.
     */
    [[nodiscard]] static bool accept(io_stream* stream);
    
    /*!
     * \brief Get the decoder name
     * \return "MML" string
     */
    [[nodiscard]] const char* get_name() const override;
    
    /*!
     * \brief Open an MML file for decoding
     * \param stream The stream containing MML text
     * \throws musac::decoder_error if the file cannot be parsed
     * 
     * Reads the entire MML text, parses it into musical events,
     * and prepares the tone queue for playback. Parse warnings
     * are collected and can be retrieved via get_warnings().
     */
    void open(io_stream* stream) override;
    
    /*!
     * \brief Get the number of audio channels
     * \return Always returns 2 (stereo with duplicated channels)
     * 
     * MML generates monophonic square waves that are duplicated
     * to both channels for stereo compatibility.
     */
    [[nodiscard]] channels_t get_channels() const override;
    
    /*!
     * \brief Get the sample rate
     * \return Always returns 44100 Hz
     */
    [[nodiscard]] sample_rate_t get_rate() const override;
    
    /*!
     * \brief Get the total duration of the MML sequence
     * \return Duration in microseconds
     * 
     * Calculates the total duration based on all notes, rests,
     * and tempo changes in the MML sequence.
     */
    [[nodiscard]] std::chrono::microseconds duration() const override;
    
    /*!
     * \brief Reset playback to the beginning
     * \return Always returns true
     * 
     * Resets the tone queue and playback position to the start
     * of the MML sequence.
     */
    bool rewind() override;
    
    /*!
     * \brief Seek to a specific time position
     * \param pos Target position in microseconds
     * \return true on success, false if position is out of range
     * 
     * Seeks to the specified position in the MML sequence.
     * Due to the pre-generated tone queue, seeking is efficient.
     */
    bool seek_to_time(std::chrono::microseconds pos) override;
    
    /*!
     * \brief Get parse warnings from MML processing
     * \return Vector of warning messages
     * 
     * Returns any warnings generated during MML parsing, such as:
     * - Unrecognized commands
     * - Out-of-range parameters
     * - Deprecated command usage
     */
    [[nodiscard]] std::vector<std::string> get_warnings() const;
    
protected:
    /*!
     * \brief Decode audio samples
     * \param buf Output buffer for decoded samples
     * \param len Number of samples to decode
     * \param call_again Set to true if more data is available
     * \return Number of samples actually decoded
     * 
     * Generates square wave audio from the pre-parsed tone queue.
     * Handles tone transitions, articulation gaps, and end-of-sequence.
     */
    size_t do_decode(float buf[], size_t len, bool& call_again) override;
    
private:
    struct tone {
        float frequency_hz;
        size_t duration_samples;
        size_t start_sample;
    };
    
    // MML data
    std::string m_mml_content;
    std::vector<mml_event> m_events;
    std::deque<tone> m_tone_queue;
    std::vector<std::string> m_warnings;
    
    // Playback state
    size_t m_current_sample = 0;
    size_t m_total_samples = 0;
    float m_phase_accumulator = 0.0f;
    size_t m_current_tone_sample = 0;
    
    // Audio parameters
    sample_rate_t m_sample_rate = 44100;
    channels_t m_channels = 2;
    
    // Parse MML and populate tone queue
    void parse_mml();
    void events_to_tones();
    
    // Generate square wave
    void generate_square_wave(float* buffer, size_t samples, float frequency);
    void generate_silence(float* buffer, size_t samples);
};

} // namespace musac

#endif // MUSAC_CODECS_DECODER_MML_HH
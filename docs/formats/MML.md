# Microsoft BASIC MML Format Specification

## Table of Contents
1. [Overview](#overview)
2. [History](#history)
3. [Basic Syntax](#basic-syntax)
4. [Command Reference](#command-reference)
5. [Advanced Features](#advanced-features)
6. [Wave Generation (C++)](#wave-generation-c)
7. [Examples](#examples)
8. [Implementation Notes](#implementation-notes)

## Overview

Music Macro Language (MML) is a text-based music description language originally developed by Microsoft for their BASIC implementations (GW-BASIC, BASICA, QBASIC). It allows programming of musical sequences using simple ASCII commands through the PLAY statement.

### Key Characteristics
- Single-channel monophonic output (PC Speaker)
- Text-based command strings
- Note-based sequencing similar to sheet music
- Tempo and octave control
- Simple articulation support

## History

MML was introduced in Microsoft BASIC for Japanese personal computers in the early 1980s. The implementation in GW-BASIC became the standard for PC-compatible computers, using the PC Speaker for audio output. The format was later extended for multi-voice systems like the Tandy 1000 and MSX computers.

## Basic Syntax

### PLAY Statement
```basic
PLAY "<command string>"
```

The command string contains a sequence of MML commands that are executed sequentially.

### Additional Statements

#### BEEP Statement
```basic
BEEP
```
Plays an 800 Hz tone for 0.25 seconds. Implementation varies:
- BASICA: Same as SOUND 800, 4 (affected by background mode)
- GW-BASIC: Same as SOUND 800, 4.55 (forced foreground)
- QuickBASIC: Same as PRINT CHR$(7) (not affected by background mode)

#### SOUND Statement
```basic
SOUND frequency, duration
```
- frequency: 37-32767 Hz (0 = silence in GW-BASIC/QuickBASIC)
- duration behavior varies by implementation:

| Duration | BASICA | GW-BASIC | QuickBASIC |
|----------|---------|----------|------------|
| < 0 | Continuous tone | Continuous tone | Ignored |
| 0 | Stops any tone | Stops any tone | Stops any tone |
| 0.016-0.022 | duration/18.2 seconds | duration/17.7 seconds | duration/18.2 seconds |
| ≥ 1986/2048 | 1:52 max | 1:52 max | 1:52 max |

### Multiple Channels (Extended versions)
```basic
PLAY "<channel 1>", "<channel 2>", "<channel 3>"
```

## Command Reference

### Note Commands

| Command | Description | Example |
|---------|-------------|---------|
| `A`-`G` | Play note (A through G) | `C` plays C note |
| `A`-`G[n]` | Play note with specific length | `C8` plays C eighth note |
| `#` or `+` | Sharp modifier (follows note) | `C#` or `C+` plays C-sharp |
| `-` | Flat modifier (follows note) | `B-` plays B-flat |
| `R` or `P` | Rest/Pause | `R4` plays quarter rest |

### Note Length

Note lengths are specified by appending a number after the note:

| Number | Length | Musical Value |
|--------|--------|---------------|
| `1` | Whole note | 𝅝 |
| `2` | Half note | 𝅗𝅥 |
| `4` | Quarter note | ♩ |
| `8` | Eighth note | ♪ |
| `16` | Sixteenth note | ♬ |
| `32` | Thirty-second note | 𝅘𝅥𝅰 |
| `64` | Sixty-fourth note | 𝅘𝅥𝅱 |

Example: `C4` plays a C quarter note

### Length Modifiers

| Modifier | Description | Example |
|----------|-------------|---------|
| `.` | Dotted note (1.5x length) | `C4.` plays dotted quarter |
| `..` | Double-dotted (1.75x length) | `C4..` |

### Octave Commands

| Command | Description | Range |
|---------|-------------|-------|
| `O<n>` | Set octave | `O0` to `O6` (middle C at start of O3) |
| `<` | Down one octave (before note) | Added in GW-BASIC 2.0 |
| `>` | Up one octave (before note) | Added in GW-BASIC 2.0 |

### Tempo and Timing

| Command | Description | Example |
|---------|-------------|---------|
| `T<n>` | Set tempo | `T120` (32-255 BPM, default 120) |
| `L<n>` | Set default note length | `L4` (1-64, default 4 = quarter note) |

Note: In GW-BASIC and QuickBASIC, tempo runs 3% slower than specified.

### Volume and Articulation

| Command | Description | Example |
|---------|-------------|---------|
| `V<n>` | Set volume | `V15` (range: 0-15) |
| `ML` | Legato mode | Notes play full length |
| `MN` | Normal mode | Notes play 7/8 length (default) |
| `MS` | Staccato mode | Notes play 3/4 length |

### Advanced Commands

| Command | Description | Example |
|---------|-------------|---------|
| `MF` | Music Foreground mode | PLAY statements complete before next BASIC statement |
| `MB` | Music Background mode | PLAY statements go to buffer (32 commands, 38 in QB4.x) |
| `X<string>;` | Execute substring | `X"CDEF";` or `X` + VARPTR$(var$) in QB |
| `N<n>` | Play note by number | `N37` (0-84, 0=rest) |
| `=<variable>` | Use variable value | `T=A` where A contains tempo value |

## Advanced Features

### Background Music Mode
```basic
PLAY "MB"  ' Enable background mode
PLAY "T120 L4 CDEFGAB>C"  ' Music plays while program continues
```

The background buffer holds 32 commands (38 in QuickBASIC 4.x). If a PLAY statement exceeds this buffer, BASIC halts further execution until space is available.

### PLAY(n) Function
The PLAY(n) function returns the number of commands in the background music buffer:

```basic
result = PLAY(n)  ' n should be 0
```

Example of continuous background music:
```basic
10 DIM MUSIC$(4)
20 MUSIC$(1) = "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 E8. D16 D4"
30 MUSIC$(2) = "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 D8. C16 C4"
40 MUSIC$(3) = "D8 D8 E8 C8 D8 E16 F16 E8 C8 D8 E16 F16 E8 D8 C8 D8 O1 G4 O2"
50 MUSIC$(4) = "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 D8. C16 C4"
60 PLAY "MB O2 T120"
70 MUSICPART = 0
500 K$ = INKEY$
510 IF LEN(K$) > 0 THEN PRINT K$;
520 IF PLAY(0) < 4 THEN GOSUB 1000
530 IF K$ = CHR$(27) THEN END
540 GOTO 500
1000 MUSICPART = MUSICPART + 1
1010 IF MUSICPART > 4 THEN MUSICPART = 1
1020 PLAY MUSIC$(MUSICPART)
1030 RETURN
```

### ON PLAY(n) Event Trap
Creates an event trap for background music management:

```basic
ON PLAY(n) GOSUB linenumber
PLAY action  ' action = "ON", "OFF", or "STOP"
```

Example with event-driven playback:
```basic
10 DIM MUSIC$(4)
20 MUSIC$(1) = "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 E8. D16 D4"
30 MUSIC$(2) = "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 D8. C16 C4"
40 MUSIC$(3) = "D8 D8 E8 C8 D8 E16 F16 E8 C8 D8 E16 F16 E8 D8 C8 D8 O1 G4 O2"
50 MUSIC$(4) = "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 D8. C16 C4"
60 MUSICPART = 0
70 PLAY "MB O2 T120"
80 ON PLAY(1) GOSUB 1000
90 PLAY ON
100 GOSUB 1000
500 K$ = INKEY$
510 IF LEN(K$) > 0 THEN PRINT K$;
520 IF K$ = CHR$(27) THEN END
530 GOTO 500
1000 MUSICPART = MUSICPART + 1
1010 IF MUSICPART > 4 THEN MUSICPART = 1
1020 PLAY MUSIC$(MUSICPART)
1030 RETURN 500
```

### Variable Integration
```basic
A = 60
PLAY "T=A C D E F"  ' Tempo set to value of variable A
```

### Substring Execution
```basic
' BASICA/GW-BASIC syntax:
A$ = "CDEF"
PLAY "X" + A$ + ";"  ' Execute contents of A$

' QuickBASIC/QBASIC syntax:
A$ = "CDEF"
PLAY "X" + VARPTR$(A$) + " G4"  ' Use VARPTR$ for compatibility
```

## Wave Generation (C++)

### Basic Square Wave Generator

```cpp
#include <cmath>
#include <cstdint>
#include <vector>

class MMLWaveGenerator {
private:
    static constexpr double SAMPLE_RATE = 44100.0;
    static constexpr double PI = 3.14159265358979323846;
    
    // Note frequencies (A4 = 440Hz)
    const double noteFrequencies[12] = {
        261.63,  // C
        277.18,  // C#
        293.66,  // D
        311.13,  // D#
        329.63,  // E
        349.23,  // F
        369.99,  // F#
        392.00,  // G
        415.30,  // G#
        440.00,  // A
        466.16,  // A#
        493.88   // B
    };
    
    int currentOctave = 4;
    int tempo = 120;
    int defaultLength = 4;
    double volume = 0.5;
    
public:
    // Generate square wave for a note
    std::vector<int16_t> generateSquareWave(int noteIndex, int octave, double duration) {
        double frequency = noteFrequencies[noteIndex] * pow(2, octave - 4);
        int numSamples = static_cast<int>(duration * SAMPLE_RATE);
        std::vector<int16_t> samples(numSamples);
        
        double period = SAMPLE_RATE / frequency;
        
        for (int i = 0; i < numSamples; i++) {
            double phase = fmod(i, period) / period;
            samples[i] = static_cast<int16_t>(
                (phase < 0.5 ? 1.0 : -1.0) * volume * 32767
            );
        }
        
        return samples;
    }
    
    // Convert note length to duration in seconds
    double noteLengthToDuration(int length, bool dotted = false) {
        double beatDuration = 60.0 / tempo;
        double noteDuration = (4.0 / length) * beatDuration;
        
        if (dotted) {
            noteDuration *= 1.5;
        }
        
        return noteDuration;
    }
    
    // Parse MML note command
    int parseNote(char note) {
        switch (toupper(note)) {
            case 'C': return 0;
            case 'D': return 2;
            case 'E': return 4;
            case 'F': return 5;
            case 'G': return 7;
            case 'A': return 9;
            case 'B': return 11;
            default: return -1;
        }
    }
};
```

### Advanced Wave Synthesis

```cpp
class AdvancedMMLSynth : public MMLWaveGenerator {
private:
    enum WaveType { SQUARE, SINE, TRIANGLE, SAWTOOTH, NOISE };
    WaveType waveType = SQUARE;
    
    // ADSR envelope parameters
    struct Envelope {
        double attack = 0.01;    // seconds
        double decay = 0.1;      // seconds
        double sustain = 0.7;    // level (0-1)
        double release = 0.2;    // seconds
    } envelope;
    
public:
    // Generate wave with envelope
    std::vector<int16_t> generateNote(int noteIndex, int octave, double duration) {
        double frequency = noteFrequencies[noteIndex] * pow(2, octave - 4);
        int numSamples = static_cast<int>(duration * SAMPLE_RATE);
        std::vector<int16_t> samples(numSamples);
        
        for (int i = 0; i < numSamples; i++) {
            double t = i / SAMPLE_RATE;
            double phase = fmod(t * frequency, 1.0);
            double sample = 0.0;
            
            // Generate base waveform
            switch (waveType) {
                case SQUARE:
                    sample = phase < 0.5 ? 1.0 : -1.0;
                    break;
                    
                case SINE:
                    sample = sin(2 * PI * phase);
                    break;
                    
                case TRIANGLE:
                    sample = phase < 0.5 
                        ? 4 * phase - 1 
                        : 3 - 4 * phase;
                    break;
                    
                case SAWTOOTH:
                    sample = 2 * phase - 1;
                    break;
                    
                case NOISE:
                    sample = (rand() / (double)RAND_MAX) * 2 - 1;
                    break;
            }
            
            // Apply envelope
            double envelopeLevel = calculateEnvelope(t, duration);
            samples[i] = static_cast<int16_t>(
                sample * envelopeLevel * volume * 32767
            );
        }
        
        return samples;
    }
    
    // Calculate ADSR envelope
    double calculateEnvelope(double t, double totalDuration) {
        double noteOffTime = totalDuration - envelope.release;
        
        if (t < envelope.attack) {
            // Attack phase
            return t / envelope.attack;
        } 
        else if (t < envelope.attack + envelope.decay) {
            // Decay phase
            double decayProgress = (t - envelope.attack) / envelope.decay;
            return 1.0 - decayProgress * (1.0 - envelope.sustain);
        }
        else if (t < noteOffTime) {
            // Sustain phase
            return envelope.sustain;
        }
        else {
            // Release phase
            double releaseProgress = (t - noteOffTime) / envelope.release;
            return envelope.sustain * (1.0 - releaseProgress);
        }
    }
};
```

### MML Parser Implementation

```cpp
#include <string>
#include <cctype>

class MMLParser {
private:
    AdvancedMMLSynth synth;
    std::vector<int16_t> outputBuffer;
    
    struct ParserState {
        size_t position = 0;
        int octave = 4;
        int defaultLength = 4;
        int tempo = 120;
        double volume = 0.7;
        bool legato = false;
        bool staccato = false;
    } state;
    
public:
    // Main parsing function
    std::vector<int16_t> parse(const std::string& mml) {
        outputBuffer.clear();
        state = ParserState();
        
        while (state.position < mml.length()) {
            char cmd = toupper(mml[state.position]);
            
            switch (cmd) {
                case 'A': case 'B': case 'C': case 'D': 
                case 'E': case 'F': case 'G':
                    parseNote(mml);
                    break;
                    
                case 'R': case 'P':
                    parseRest(mml);
                    break;
                    
                case 'O':
                    parseOctave(mml);
                    break;
                    
                case '<':
                    state.octave = std::max(0, state.octave - 1);
                    state.position++;
                    break;
                    
                case '>':
                    state.octave = std::min(8, state.octave + 1);
                    state.position++;
                    break;
                    
                case 'T':
                    parseTempo(mml);
                    break;
                    
                case 'L':
                    parseDefaultLength(mml);
                    break;
                    
                case 'V':
                    parseVolume(mml);
                    break;
                    
                case 'M':
                    parseMode(mml);
                    break;
                    
                case ' ': case '\t': case '\n': case '\r':
                    state.position++;
                    break;
                    
                default:
                    state.position++;
            }
        }
        
        return outputBuffer;
    }
    
private:
    // Parse note with modifiers and length
    void parseNote(const std::string& mml) {
        char noteChar = mml[state.position++];
        int noteIndex = synth.parseNote(noteChar);
        
        // Check for sharp/flat
        if (state.position < mml.length()) {
            char modifier = mml[state.position];
            if (modifier == '#' || modifier == '+') {
                noteIndex++;
                state.position++;
            } else if (modifier == '-') {
                noteIndex--;
                state.position++;
            }
        }
        
        // Normalize note index
        while (noteIndex < 0) noteIndex += 12;
        while (noteIndex >= 12) noteIndex -= 12;
        
        // Parse length
        int length = parseNumber(mml, state.defaultLength);
        bool dotted = false;
        
        // Check for dots
        while (state.position < mml.length() && mml[state.position] == '.') {
            dotted = true;
            state.position++;
        }
        
        // Generate and append samples
        double duration = synth.noteLengthToDuration(length, dotted);
        
        // Apply articulation
        if (!state.legato) {
            duration *= state.staccato ? 0.75 : 0.875;
        }
        
        auto samples = synth.generateNote(noteIndex, state.octave, duration);
        outputBuffer.insert(outputBuffer.end(), samples.begin(), samples.end());
        
        // Add gap for non-legato
        if (!state.legato) {
            int gapSamples = static_cast<int>(
                (state.staccato ? 0.25 : 0.125) * 
                synth.noteLengthToDuration(length, dotted) * 44100
            );
            outputBuffer.insert(outputBuffer.end(), gapSamples, 0);
        }
    }
    
    // Parse numeric value
    int parseNumber(const std::string& mml, int defaultValue) {
        int value = 0;
        bool foundDigit = false;
        
        while (state.position < mml.length() && isdigit(mml[state.position])) {
            value = value * 10 + (mml[state.position] - '0');
            state.position++;
            foundDigit = true;
        }
        
        return foundDigit ? value : defaultValue;
    }
    
    // Additional parsing methods...
    void parseRest(const std::string& mml) {
        state.position++; // Skip 'R' or 'P'
        int length = parseNumber(mml, state.defaultLength);
        bool dotted = false;
        
        while (state.position < mml.length() && mml[state.position] == '.') {
            dotted = true;
            state.position++;
        }
        
        double duration = synth.noteLengthToDuration(length, dotted);
        int numSamples = static_cast<int>(duration * 44100);
        outputBuffer.insert(outputBuffer.end(), numSamples, 0);
    }
    
    void parseOctave(const std::string& mml) {
        state.position++; // Skip 'O'
        state.octave = parseNumber(mml, state.octave);
    }
    
    void parseTempo(const std::string& mml) {
        state.position++; // Skip 'T'
        state.tempo = parseNumber(mml, state.tempo);
        synth.setTempo(state.tempo);
    }
    
    void parseDefaultLength(const std::string& mml) {
        state.position++; // Skip 'L'
        state.defaultLength = parseNumber(mml, state.defaultLength);
    }
    
    void parseVolume(const std::string& mml) {
        state.position++; // Skip 'V'
        int vol = parseNumber(mml, 10);
        state.volume = vol / 15.0;
        synth.setVolume(state.volume);
    }
    
    void parseMode(const std::string& mml) {
        state.position++; // Skip 'M'
        if (state.position < mml.length()) {
            char mode = toupper(mml[state.position++]);
            switch (mode) {
                case 'L': // Legato
                    state.legato = true;
                    state.staccato = false;
                    break;
                case 'N': // Normal
                    state.legato = false;
                    state.staccato = false;
                    break;
                case 'S': // Staccato
                    state.legato = false;
                    state.staccato = true;
                    break;
            }
        }
    }
};
```

## Examples

### Simple Melodies

**Mary Had a Little Lamb**
```basic
PLAY "T120 L4 E D C D E E E2 D D D2 E G G2"
```

**Twinkle Twinkle Little Star**
```basic
PLAY "T100 L4 C C G G A A G2 F F E E D D C2"
```

**Ode to Joy (Complete)**
```basic
10 PLAY "O2 T120 E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 E8. D16 D4"
20 PLAY "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 D8. C16 C4"
30 PLAY "D8 D8 E8 C8 D8 E16 F16 E8 C8 D8 E16 F16 E8 D8 C8 D8 O1 G4 O2"
40 PLAY "E8 E8 F8 G8 G8 F8 E8 D8 C8 C8 D8 E8 D8. C16 C4"
```

### Advanced Example

**Bach Invention (fragment)**
```basic
PLAY "T140 O4 L16 ML"
PLAY "E G F# G E G B G E G F# G E G B G"
PLAY "C E D E C E G E C E D E C E G E"
```

### Using Variables
```basic
10 TEMPO = 180
20 NOTE$ = "CDEFGAB>C"
30 PLAY "T=TEMPO L8 X" + NOTE$ + ";"
```

### Background Music with User Input
```basic
10 INPUT "Enter tempo (32-255): ", T
20 PLAY "MB T=T O3 L4"
30 PLAY "C E G > C < G E C"
40 PRINT "Music playing in background!"
50 INPUT "What's your name? ", NAME$
60 PRINT "Hello, "; NAME$
```

## Implementation Notes

### PC Speaker Limitations
- Single channel monophonic output only
- Square wave synthesis only
- Limited frequency accuracy due to timer resolution (37-32767 Hz)
- No actual volume control on PC Speaker (V command ignored)
- Higher tempo notes may not play due to clock interrupt rate
- PLAY interrupts SOUND statements with duration < 0.022

### Timing Considerations
- Tempo accuracy depends on system timer resolution (18.2 Hz)
- Background mode (MB) requires interrupt handling
- Note durations may vary slightly due to processing overhead
- Buffer limitations: 32 commands (38 in QuickBASIC 4.x)

### Timing Considerations
- Tempo accuracy depends on system timer resolution
- Background mode (MB) requires interrupt handling
- Note durations may vary slightly due to processing overhead

### Compatibility Notes
- Octave numbering: Middle C is at the beginning of octave 3
- `<` and `>` commands added in GW-BASIC 2.0 (abort in BASICA/GW-BASIC 1.0)
- X command syntax differs between implementations (use VARPTR$ in QB)
- Extended versions (Tandy, MSX) support multiple channels
- Modern implementations may support additional waveforms
- BEEP implementation varies between BASIC versions

### Best Practices
1. Always set tempo and octave at the beginning
2. Use L command to set default length for cleaner code
3. Test on target hardware due to timing variations
4. Keep strings under 255 characters (BASIC string limit)
5. Use variables for repeated patterns

## References
- Microsoft GW-BASIC User's Manual
- IBM BASIC Manual (BASICA)
- QBasic Documentation
- Music Macro Language Wikipedia Entry
- Various MML implementations and extensions
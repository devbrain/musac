# Face The Music (FTM) format documentation

## General notes

FTM is an 8-channel module format originating from the Amiga music software Face The Music.
The first two and last two channels are panned left, the rest is panned right (like normal LRRL Amiga panning, but every hardware channel mixes two software channels).
Portamentos use linear frequency scaling (similar to various PC trackers but with only 8 pitch units per note).

## File structure

All multi-byte integers are in big-endian format.

### File header

Offset | Type       | Size | Comment
-------|------------|------|----------------------------------------------------
0      | `char[4]`  | 4    | "FTMN" magic bytes
4      | `uint8`    | 1    | Always 3; probably version number
5      | `uint8`    | 1    | Number of samples (0...63)
6      | `uint16`   | 2    | Number of measures
8      | `uint16`   | 2    | Initial tempo (0x1000...0x4FFF). BPM = 1776125 / tempo (125 BPM = 14209)
10     | `uint8`    | 1    | Tonality; Not relevant for playback (0 = C/a, 1 = Db/bb, etc.)
11     | `uint8`    | 1    | Channel mute status; For each bit, if it is 0, the channel is muted (no effects or notes are processed); bit 0 = first channel.
12     | `uint8`    | 1    | Global volume (0...63)
13     | `uint8`    | 1    | Flags; 1 = module (embedded samples) if set, song (external samples) otherwise, 2 = enable LED filter
14     | `uint8`    | 1    | Initial ticks per row; Should always be 96 / rows per measure
15     | `uint8`    | 1    | Rows per measure;  Should always be 96 / ticks per row
16     | `char[32]` | 32   | Song title, null-terminated
48     | `char[32]` | 32   | Artist name, null-terminated
80     | `uint8`    | 1    | Number of SEL effects (0...64)
81     | `uint8`    | 1    | Probably a padding byte, always 0

Total size: 82 bytes

### Sample names

After the file header, sample names follow, as many as indicated by the file header.
Each name is a 31-byte null-terminated char array, followed by a byte indicating which octave to load when reading a multi-octave 8SVX sample (only relevant for songs, not modules).
Garbage characters will often appear after the first null terminator (e.g. leftovers from previous sample names) and need to be ignored.
In practice, Face The Music does not appear to support names longer than 29 characters, so maybe the second-to-last byte also has an unknown meaning, or it just serves as padding.

If the file is a song (lowest flag bit is not set), these names indicate the filenames that will be attempted to be loaded.

### SEL effects

After the sample names, SEL effects (scripts) follow, as many as indicated by the file header.
Each SEL effect starts with a `uint16` indicating the number of script lines (up to 512) and another `uint16` with the actual effect index (0...63).

After that, for each script line, a 4-byte struct follows.
The first byte indicates the effect type (see table), and the following three bytes are interpreted depending on the effect type.

All numbers in the following table are hexadecimal.

Type | Name                | Parameters | Description
-----|---------------------|------------|---------------------------------------
0    |                     | `......`   | Nothing.
1    | `WAIT t TICKS`      | `..tttt`   | Interrupts script execution until `t` ticks have passed. All modified parameters become audible. If `t` is 0, the script is paused indefinitely (interrupts are still processed).
2    | `GOTO LINE l`       | `...lll`   | Continues execution of the script at line `l`. Jumping past the end of the script stops its execution.
3    | `LOOP x TIMES TO l` | `xxxlll`   | Jumps to script line `l` as many times as specified by `x`. Loops cannot be nested.
4    | `GOTO EFF e LINE l` | `eeelll`   | Continues execution in script `e` at line `l`.
5    | `END OF EFFECT`     | `......`   | Ends script execution. All parameters modified since the last `WAIT` become audible.
6    | `IF PITCH=p GOTO l` | `ppplll`   | Jumps to script line `l` if note pitch matches `p` (`000`...`21E`). Note: Unlike documented and like with other commands, there is actually a resolution of 16 pitch units per note here (maybe as a result of an off-by-one bit shift). 
7    | `IF PITCH<p GOTO l` | `ppplll`   | Jumps to script line `l` if note pitch is below `p` (`000`...`21E`). Ditto.
8    | `IF PITCH>p GOTO l` | `ppplll`   | Jumps to script line `l` if note pitch is above `p` (`000`...`21E`). Ditto. 
9    | `IF VOLUM=v GOTO l` | `vvvlll`   | Jumps to script line `l` if note volume matches `v` (`000`...`040`). 
10   | `IF VOLUM<v GOTO l` | `vvvlll`   | Jumps to script line `l` if note volume is below `v` (`000`...`040`). 
11   | `IF VOLUM>v GOTO l` | `vvvlll`   | Jumps to script line `l` if note volume is above v` (`000`...`040`).
12   | `ON NEWPIT. GOTO l` | `...lll`   | Sets up "interrupt handler" to jump to script line `l` if a pitch change occurs due to pattern data (not script data). 
13   | `ON NEWVOL. GOTO l` | `...lll`   | Sets up "interrupt handler" to jump to script line `l` if a volume change occurs due to pattern data (not script data). 
14   | `ON NEWSAM. GOTO l` | `...lll`   | Sets up "interrupt handler" to jump to script line `l` if a sample change occurs due to pattern data (not script data). 
15   | `ON RELEASE GOTO l` | `...lll`   | Sets up "interrupt handler" to jump to script line `l` if a "REL" note is encountered. 
16   | `ON PB GOTO l`      | `...lll`   | Sets up "interrupt handler" to jump to script line `l` if a "PB" pitch bend command is encountered. 
17   | `ON VD GOTO l`      | `...lll`   | Sets up "interrupt handler" to jump to script line `l` if a "VD" volume down command is encountered. 
18   | `PLAY CURR. SAMPLE` | `......`   | Restarts playback of the current sample from the beginning. 
19   | `PLAY QUIET SAMPLE` | `......`   | Plays the "silent" sample. 
20   | `PLAYPOSITION =`    | `.ppppp`   | Continues playing the current sample from position `p`. Value is in words.
21   | `PLAYPOSITION ADD`  | `.ppppp`   | Continues playing the current sample from current position + `p`.
22   | `PLAYPOSITION SUB`  | `.ppppp`   | Continues playing the current sample from current position - `p`.
23   | `PITCH =`           | `...ppp`   | Sets the sample pitch to `p` (`000` = C0, `008` = C1, ..., `10F` = A2).
24   | `DETUNE =`          | `...ddd`   | Detunes the two voices of a hardware channel against each other by `d` (`000`...`FFF`). The first two channels are the first hardware voice, etc.
25   | `DETUNE/PITCH ADD`  | `dddppp`   | Adds `d` to the current detune and `p` to the current pitch.
26   | `DETUNE/PITCH SUB`  | `dddppp`   | Subtracts `d` from the current detune and `p` from the current pitch.
27   | `VOLUME =`          | `....vv`   | Sets note volume using the full Amiga volume range (`00`...`40`).
28   | `VOLUME ADD`        | `....vv`   | Adds `v` to the note volume.
29   | `VOLUME SUB`        | `....vv`   | Subtracts `v` from the note volume.
30   | `CURRENT SAMPLE =`  | `....ss`   | Loads the sample `s` (`00`...`3E`) but doesn't play it yet.
31   | `SAMPLESTART =`     | `.sssss`   | Redefines the sample start point to `s`.
32   | `SAMPLESTART ADD`   | `.sssss`   | Adds `s` to the sample start point.
33   | `SAMPLESTART SUB`   | `.sssss`   | Subtracts `s` from the sample start point.
34   | `ONESHOTLENGTH =`   | `..oooo`   | Changes the one-shot length (part before loop start) to `o`. Value is in words. Repeat length is not affected.
35   | `ONESHOTLENGTH ADD` | `..oooo`   | Adds `o` words to the one-shot length.
36   | `ONESHOTLENGTH SUB` | `..oooo`   | Subtracts `o` words from the one-shot length.
37   | `REPEATLENGTH =`    | `..rrrr`   | Changes the repeat length to `r`. Value is in samples.
38   | `REPEATLENGTH ADD`  | `..rrrr`   | Adds `r` to the repeat length.
39   | `REPEATLENGTH SUB`  | `..rrrr`   | Subtracts `r` from the repeat length.
40   | `GET PIT. OF TRACK` | `.....t`   | Sets pitch to that of track `t` (`0`...`7`)
41   | `GET VOL. OF TRACK` | `.....t`   | Sets volume to that of track `t` (`0`...`7`)
42   | `GET SAM. OF TRACK` | `.....t`   | Sets sample to that of track `t` (`0`...`7`)
43   | `CLONE TRACK`       | `.....t`   | Copies over all information of track `t` (`0`...`7`): Pitch, volume, sample, play position, LFOs, running pattern effects, etc.
44   | `1ST LFO START`     | `mfssdd`   | Sets modulation target to `m` (see below), LFO shape to `f` (see below), LFO speed to `s` (`00`...`BD` repetitions per measure, 0 = stop) and depth `d` (`00`...`7F`).
45   | `1ST LFO SP/DE ADD` | `..ssdd`   | Adds `s` to the LFO 1 speed and `d` to the LFO 1 depth.
46   | `1ST LFO SP/DE SUB` | `..ssdd`   | Subtracts `s` from the LFO 1 speed and `d` from the LFO 1 depth.
47   | `2ND LFO START`     | `mfssdd`   | See 1st LFO.
48   | `2ND LFO SP/DE ADD` | `..ssdd`   | See 1st LFO.
49   | `2ND LFO SP/DE SUB` | `..ssdd`   | See 1st LFO.
50   | `3RD LFO START`     | `mfssdd`   | See 1st LFO.
51   | `3RD LFO SP/DE ADD` | `..ssdd`   | See 1st LFO.
52   | `3RD LFO SP/DE SUB` | `..ssdd`   | See 1st LFO.
53   | `4TH LFO START`     | `mfssdd`   | See 1st LFO.
54   | `4TH LFO SP/DE ADD` | `..ssdd`   | See 1st LFO.
55   | `4TH LFO SP/DE SUB` | `..ssdd`   | See 1st LFO.
56   | `WORK ON TRACK t`   | `.....t`   | All following events operate on track `t` (`0`...`7`).
57   | `WORKTRACK ADD`     | `.....t`   | Adds `t` to the current work track. Result is clamped to `0`...`7`.
58   | `GLOBAL VOLUME =`   | `....vv`   | Sets the global volume to `v` (`00`...`40`).
59   | `GLOBAL SPEED =`    | `..ssss`   | Sets the tempo to `s` (`1000`...`4FFF`).
60   | `TICKS PER LINE =`  | `....tt`   | Sets the ticks per row to `tt` (`01`...`FF`).
61   | `JUMP TO SONGLINE`  | `..llll`   | Sets the next row of the song to be played to `l` (`0000`...`FFFF`).

#### Notes on "interrupt handler" effects

- Interrupts (`ON ... GOTO`) are reset when the interrupt is triggered, or when the script is restarted / new script is triggered.
- If the script is stopped (due command `END OF EFFECT` or reaching the end of the script), interrupts are deactivated as well.

#### LFO modulation targets

- 0: Nothing
- 1: LFO 1 speed
- 2: LFO 2 speed
- 3: LFO 3 speed
- 4: LFO 4 speed
- 5: LFO 1 depth
- 6: LFO 2 depth
- 7: LFO 3 depth
- 8: LFO 4 depth
- 9: Unused
- A: Track amplitude
- B: Unused
- C: Unused
- D: Unused
- E: Unused
- F: Track frequency

#### LFO modulation shapes

- 0: Sine, looped
- 1: Square, looped
- 2: Triangle, looped 
- 3: Sawtooth up, looped
- 4: Sawtooth down, looped
- 5: Unused
- 6: Unused
- 7: Unused
- 8: Sine, one-shot
- 9: Square, one-shot
- A: Triangle, one-shot
- B: Sawtooth up, one-shot
- C: Sawtooth down, one-shot
- D: Unused
- E: Unused
- F: Unused

### Pattern data

After the SEL effects, the pattern data follows if the number of measures as indicated by the file header is non-zero.

Unlike most module formats, the data is stored in column-major order, i.e. the entire contents of the first channel are stored as one continuous stream (no division into individual patterns), then the entire contents of the second channel, etc.

For each of the 8 channels, the following structure is repeated.

First a `uint16` indicates the default spacing to be added between events.
After that, a `uint32` indicates how many bytes of pattern events for this channel follow.

Every event consists of two bytes, encoded as follows (most significant bit goes first):

```
First byte | Second byte
tttt pppp  | ppnn nnnn
```

To extract:

```C
type = firstByte >> 4;
parameter = ((firstByte & 0x0F) << 2) | (secondByte >> 6);
note = secondByte & 0x3F;
```

Command types:

- 0: No effect, parameter: instrument number
- 1: Volume 0, parameter: instrument number
- 2: Volume 1, parameter: instrument number
- 3: Volume 2, parameter: instrument number
- 4: Volume 3, parameter: instrument number
- 5: Volume 4, parameter: instrument number
- 6: Volume 5, parameter: instrument number
- 7: Volume 6, parameter: instrument number
- 8: Volume 7, parameter: instrument number
- 9: Volume 8, parameter: instrument number
- A: Volume 9, parameter: instrument number
- B: SEL effect, parameter: effect index
- C: Pitch bend, parameter: number of rows over which the portamento should occur
- D: Volume down, parameter: number of rows over which the fade-out should occur
- E: Pattern loop, parameter: 0 = loop start, other values = repeat how many times
- F: Skip a number of empty rows. Note that in this case, the parameter and note bits are combined into one single parameter. (`n` = low bits, `p` = high bits)

Pattern notes are indicated by note values 1...34 (C0...A2), 35 is the "REL" note, 0 = no note.

Note that the interaction between the default spacing and command F is a bit peculiar:
The spacing is applied before the next note is read; this is also applied before the very first pattern event.
So in the case where the default spacing is non-zero, if you want to place any commands on the first row, the first command in the channel needs to be `F0 00`, as this will overwrite the spacing to be applied to the following event. 

### Sample data

After the pattern data, the sample data follows if the flags indicate that the file is a module with embedded samples.
Sample slots with an empty name are skipped entirely.

For each sample with a name, a `uint16` indicates the loop start in words (multiply by two to get the loop start in samples), followed by another `uint16` for the loop length (also in words).
The total sample length is loop start + loop length. For samples without a loop, the loop length is 0 and the loop start is the total sample length.

Samples are in 8-bit mono signed PCM format.

If the file is a song with external samples, loop information is inferred from the sample file (for 8SVX samples; raw samples are also supported but obviously cannot contain any loop information).

## Document information

Document revision 6 (2024-05-29).

Reversed by Saga Musix. I bought a copy of Face The Music on classifieds just so that I could read the printed manual.  
https://sagamusix.de/

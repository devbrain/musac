Audio Files for Testing
========================

Place your audio files in this directory to test the Musac audio player.

Supported formats:
- Standard: WAV, MP3, OGG, FLAC, AIFF
- Tracker: MOD, XM, S3M, IT
- MIDI: MID, MIDI
- Retro: VGM, VGZ, CMF, DRO, IMF
- Other: VOC, MML

The player will automatically detect all supported audio files in this directory.

Platform Support:
- Desktop: Full Musac codec support for all formats
- Android: Full Musac codec support via NDK
- HTML5/Web: Falls back to libGDX for standard formats (WAV, MP3, OGG)

Note: For HTML5, only standard web-compatible formats will work unless
you compile Musac to WebAssembly.
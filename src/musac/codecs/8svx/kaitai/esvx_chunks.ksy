meta:
  id: esvx_chunks  # Use esvx instead of 8svx as identifiers can't start with numbers
  title: IFF 8SVX VHDR chunk definition
  application: IFF 8SVX audio format
  file-extension:
    - 8svx
    - iff
  license: MIT
  endian: be
  
types:
  vhdr_chunk:
    doc: Voice Header - contains playback parameters
    seq:
      - id: one_shot_hi_samples
        type: u4
        doc: Number of samples in the one-shot (non-repeating) part
      - id: repeat_hi_samples
        type: u4
        doc: Number of samples in the repeating part
      - id: samples_per_hi_cycle
        type: u4
        doc: Number of samples per cycle in highest octave
      - id: samples_per_sec
        type: u2
        doc: Playback sampling rate in Hz
      - id: octave_count
        type: u1
        doc: Number of octaves of waveforms
      - id: compression
        type: u1
        enum: compression_type
        doc: Compression algorithm used (0=none, 1=fibonacci-delta)
      - id: volume
        type: u4
        doc: Playback volume in 16.16 fixed-point format
        
enums:
  compression_type:
    0: none
    1: fibonacci_delta
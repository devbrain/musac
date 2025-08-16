meta:
  id: aiff_chunks
  title: AIFF Chunk Contents Parser
  license: MIT
  endian: be
  
# This file defines parsers for the CONTENTS of AIFF chunks only.
# The IFF container parsing is handled by libiff.

types:
  comm_chunk_aiff:
    doc: Common chunk for AIFF (no compression fields)
    seq:
      - id: num_channels
        type: u2
        doc: Number of audio channels
      - id: num_sample_frames
        type: u4
        doc: Number of sample frames
      - id: sample_size
        type: u2
        doc: Number of bits per sample
      - id: sample_rate
        type: extended_float
        doc: Sample rate in Hz (80-bit IEEE extended)
        
  comm_chunk_aifc:
    doc: Common chunk for AIFC (with compression fields)
    seq:
      - id: num_channels
        type: u2
        doc: Number of audio channels
      - id: num_sample_frames
        type: u4
        doc: Number of sample frames
      - id: sample_size
        type: u2
        doc: Number of bits per sample
      - id: sample_rate
        type: extended_float
        doc: Sample rate in Hz (80-bit IEEE extended)
      - id: compression_type
        type: str
        size: 4
        encoding: ASCII
        doc: Compression type fourcc
      - id: compression_name
        type: pstring
        doc: Human-readable compression name
        
  ssnd_chunk:
    doc: Sound data chunk header (not the data itself)
    seq:
      - id: offset
        type: u4
        doc: Offset to start of audio data (usually 0)
      - id: block_size
        type: u4
        doc: Block size for block-aligned data (usually 0)
      # Note: audio_data is NOT parsed here - handled by libiff streaming
        
  mark_chunk:
    doc: Marker chunk - defines positions in audio
    seq:
      - id: num_markers
        type: u2
      - id: markers
        type: marker
        repeat: expr
        repeat-expr: num_markers
        
  marker:
    seq:
      - id: marker_id
        type: u2
        doc: Unique marker ID
      - id: position
        type: u4
        doc: Frame position of marker
      - id: marker_name
        type: pstring
        doc: Name of the marker
        
  inst_chunk:
    doc: Instrument chunk - MIDI instrument info
    seq:
      - id: base_note
        type: u1
        doc: MIDI note number
      - id: detune
        type: u1
        doc: Pitch shift in cents
      - id: low_note
        type: u1
      - id: high_note
        type: u1
      - id: low_velocity
        type: u1
      - id: high_velocity
        type: u1
      - id: gain
        type: u2
        doc: Gain in dB
      - id: sustain_loop
        type: loop_info
      - id: release_loop
        type: loop_info
        
  loop_info:
    seq:
      - id: play_mode
        type: u2
        enum: loop_mode
      - id: begin_loop
        type: u2
        doc: Marker ID for loop start
      - id: end_loop
        type: u2
        doc: Marker ID for loop end
        
  comt_chunk:
    doc: Comments chunk
    seq:
      - id: num_comments
        type: u2
      - id: comments
        type: comment
        repeat: expr
        repeat-expr: num_comments
        
  comment:
    seq:
      - id: timestamp
        type: u4
        doc: Time of comment creation
      - id: marker_id
        type: u2
        doc: Related marker ID (0 if none)
      - id: text
        type: pstring
        
  fver_chunk:
    doc: Format version chunk (AIFC only)
    seq:
      - id: timestamp
        type: u4
        doc: Format version timestamp
        
  appl_chunk:
    doc: Application-specific chunk
    seq:
      - id: application_signature
        type: str
        size: 4
        encoding: ASCII
      - id: data
        size-eos: true
        
  extended_float:
    doc: 80-bit IEEE 754 extended precision float
    seq:
      - id: exponent
        type: u2
      - id: mantissa
        type: u8
    # Conversion to double handled in C++ code
        
  pstring:
    doc: Pascal-style string (length byte followed by characters)
    seq:
      - id: length
        type: u1
      - id: text
        type: str
        size: length
        encoding: ASCII
      - id: padding
        size: (length + 1) % 2
        doc: Pascal strings are padded to even length
        
enums:
  loop_mode:
    0: no_loop
    1: forward_loop
    2: forward_backward_loop
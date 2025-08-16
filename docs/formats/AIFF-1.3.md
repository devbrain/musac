 Audio Interchange File Format: "AIFF" 

 A Standard for Sampled Sound Files Version 1.3 Apple Computer, Inc. 

______________________________________________________________________________ 

Modification History 

Version 1.1 January 21, 1988 Original version. Version 1.2 June 17, 1988 Corrected typographical error in illustration on page 4. Version 1.3 January 4, 1989 Apple II modifications. Changed Apple II file type from 0xCB to 0xD8 on page 4. Defined convention for using _applicationSignature_ field in _Application Specific Chunk_ for Apple II on page 15. ______________________________________________________________________________ 

The Audio Interchange File Format (Audio IFF) provides a standard for storing sampled sounds. The format is quite flexible, allowing for the storage of monaural or multichannel sampled sounds at a variety of sample rates and sample widths. 

Audio IFF conforms to the " _EA IFF 85" Standard for Interchange Format Files_ developed by Electronic Arts. 

Audio IFF is primarily an **interchange** format, although application designers should find it flexible enough to use as a data storage format as well. If an application does choose to use a different storage format, it should be able to convert to and from the format defined in this document. This will facilitate the sharing of sound data between applications. 

Audio IFF is the result of several meetings held with music developers over a period of ten months in 1987-88. Apple Computer greatly appreciates the comments and cooperation provided by all developers who helped define this standard. 


Another "EA IFF 85" sound storage format is _"8SVX" IFF 8-bit Sampled Voice_ , by Electronic Arts. "8SVX", which handles 8-bit monaural samples, is intended mainly for storing sound for playback on personal computers. Audio IFF is intended for use with a larger variety of computers, sampled sound instruments, sound software applications, and high fidelity recording devices. 

**Data types** 

A C-like language will be used to describe data structures in this document. The data types used are listed below: 

char: 8 bits, signed. A char can contain more than just ASCII characters. It can contain any number from -128 to 127 (inclusive). unsigned char: 8 bits, unsigned. Contains any number from zero to 255 (inclusive). short: 16 bits, signed. Contains any number from -32,768 to 32,767 (inclusive). unsigned short: 16 bits, unsigned. Contains any number from zero to 65,535 (inclusive). long: 32 bits, signed. Contains any number from -2,147,483,648 to 2,147,483,647 (inclusive). unsigned long: 32 bits, unsigned. Contains any number from zero to 4,294,967,295 (inclusive). extended: 80 bit IEEE Standard 754 floating point number (Standard Apple Numeric Environment [SANE] data type _Extended_ ). pstring: Pascal-style string, a one byte count followed by text bytes. The total number of bytes in this data type should be even. A pad byte can be added at the end of the text to accomplish this. This pad byte is not reflected in the count. ID: 32 bits, the concatenation of four printable ASCII character in the range ' ' (SP, 0x20) through '~' (0x7E). Spaces (0x20) cannot precede printing characters; trailing spaces are allowed. Control characters are forbidden. OSType: 32 bits. A concatenation of four characters, as defined in _Inside Macintosh, vol II._ 

_Constants_ 

Decimal values are referred to as a string of digits, for example 123, 0, 100 are all decimal numbers. Hexadecimal values are preceded by a 0x e.g. 0x0A12, 0x1, 0x64. 


_Data Organization_ 

All data is stored in Motorola 68000 format. Data is organized as follows: 

 7 6 5 4 3 2 1 0 char msb lsb 

 msb lsb 

 15 1413121110 9 87 6 5 4 3 2 1 0 short byte 0 byte 1 

 7 6 5 4 3 2 1 0 msb 

long (^) lsb byte 0 byte 1 byte 2 byte 3 15 1413121110 9 8 **Referring to Audio IFF** The official name for this standard is _Audio Interchange File Format._ If an application program needs to present the name of this format to a user, such as in a "Save as…" dialog box, the name can be abbreviated to _Audio IFF_. 


**File Structure** 

The " _EA IFF 85" Standard for Interchange Format Files_ defines an overall structure for storing data in files. Audio IFF conforms to the "EA IFF 85" standard. This document will describe those portions of "EA IFF 85" that are germane to Audio IFF. For a more complete discussion of "EA IFF 85", please refer to the document _"EA IFF 85" Standard for Interchange Format Files_. 

An "EA IFF 85" file is made up of a number of _chunks_ of data. Chunks are the building blocks of "EA IFF 85" files. A chunk consists of some header information followed by data: 

 A chunk. 

 ckID ckSize 

 data 

# }^ header info 

A chunk can be represented using our C-like language in the following manner: 

typedef struct { 

 ID ckID; /* chunk ID */ long ckSize; /* chunk Size */ 

 char ckData[]; /* data */ 

} Chunk; 

_ckID_ describes the format of the _data_ portion a chunk. A program can determine how to interpret the chunk data by examining _ckID._ 

_ckSize_ is the size of the data portion of the chunk, in bytes. It does not include the 8 bytes used by _ckID_ and _ckSize._ 

_ckData_ contains the data stored in the chunk. The format of this data is determined by _ckID_. If the data is an odd number of bytes in length, a zero pad byte must be added at the end. The pad byte is not included in _ckSize_. 


Note that an array with no size specification (e.g. char ckData[];) indicates a variable-sized array in our C-like language. This differs from standard C. 

An Audio IFF file is a collection of a number of different types of chunks. There is a _Common Chunk_ which contains important parameters describing the sampled sound, such as it's length and sample rate. There is a _Sound Data Chunk_ that contains the actual audio samples. There are several other optional chunks that define markers, list instrument parameters, store applicationspecific information, etc. All of these chunks are described in detail in later sections of this document. 

The chunks in a Audio IFF file are grouped together in a container chunk. "EA IFF 85" defines a number of container chunks, but the one used by Audio IFF is called a FORM. A FORM has the following format: 

typedef struct { 

 ID ckID; long ckSize; 

 ID formType; char chunks []; 

} Chunk; 

_ckID_ is always 'FORM'. This indicates that this is a FORM chunk. 

_ckSize_ contains the size of data portion of the 'FORM' chunk. Note that the data portion has been broken into two parts, _formType_ and _chunks_ []. 

_formType_ describes what's in the 'FORM' chunk. For Audio IFF files, _formType_ is always 'AIFF'. This indicates that the chunks within the FORM pertain to sampled sound. A FORM chunk of _formType_ 'AIFF' is called a _FORM AIFF_. 

_chunks_ are the chunks contained within the FORM. These chunks are called _local chunks_. A FORM AIFF along with its local chunks make up an Audio IFF file. 

Here is an example of a simple Audio IFF file. It consists of a file containing single FORM AIFF which contains two local chunks, a Common Chunk and a Sound Data Chunk. 


 ckID = 'FORM' 

 ckID = 'COMM' 

 Common Chunk 

 ckID = 'SSND' 

 Sound Data Chunk 

 FORM AIFF Chunk formType = 'AIFF' 

There are no restrictions on the ordering of local chunks within a FORM AIFF. 

On an Apple II, the FORM AIFF is stored in a ProDOS file. The file type is 0xD8 and the aux type is 0x0000. AIFF versions 1.2 and earlier used file type 0xCB, which is incorrect. Please see the Apple II File Type Note for file type 0xD8 and aux type 0x0000 for strategies on dealing with this inconsistency. 

On a Macintosh, the FORM AIFF is stored in the data fork of an Audio IFF file. The Macintosh file type of an Audio IFF file is 'AIFF'. This is the same as the _formType_ of the FORM AIFF. 


Macintosh or Apple II applications should not store any information in Audio IFF file's resource fork, as this information may not be preserved by all applications. Applications can use the _Application Specific Chunk_ , defined later in this document, to store extra information specific to their application. 

On an operating system that uses file extensions, such as MS-DOS or UNIX, it is recommended that Audio IFF file names have a ".AIF" extension. 

A more detailed example of an Audio IFF file can be found in Appendix A. Please refer to this example as often as necessary while reading the remainder of this document. 

**Local Chunk Types** 

The formats of the different local chunk types found within a FORM AIFF are described in the following sections. The _ckID_ s for each chunk are also defined. 

There are two types of chunks, those that are required and those that are optional. The Common Chunk is required. The Sound Data chunk is required if the sampled sound has greater than zero length. All other chunks are optional. All applications that use FORM AIFF must be able to read the required chunks, and can choose to selectively ignore the optional chunks. A program that copies a FORM AIFF should copy all of the chunks in the FORM AIFF. 

To insure that this standard remains usable by all developers, only Apple Computer, Inc. should define new chunk types for FORM AIFF. If you have suggestions for new chunk types, Apple is happy to listen! Please refer to Appendix B for instructions on how to send comments to Apple. 

**Common Chunk** 

The Common Chunk describes fundamental parameters of the sampled sound. 

#define CommonID 'COMM' /* ckID for Common Chunk */ 

typedef struct { 

 ID ckID; long ckSize; 


 short numChannels; unsigned long numSampleFrames; short sampleSize; extended sampleRate; 

} CommonChunk; 

_ckID_ is always 'COMM'. _ckSize_ is the size of the data portion of the chunk, in bytes. It does not include the 8 bytes used by _ckID_ and _ckSize._ For the Common Chunk, _ckSize_ is always 18. 

_numChannels_ contains the number of audio channels for the sound. A value of 1 means monophonic sound, 2 means stereo, and 4 means four channel sound, etc. Any number of audio channels may be represented. 

The actual sound samples are stored in another chunk, the _Sound Data Chunk,_ which will be described shortly_._ For multichannel sounds, single sample points from each channel are interleaved. A set of interleaved sample points is called a _sample frame._ This is illustrated below for the stereo case. 

 frame 0 frame 1 frame N ch 1 ch 2 ch 1 ch 2 •^ •^ • ch 1 ch 2 

 = one sample point 

 sample sample sample 

For monophonic sound, a sample frame is a single sample point. 

For multichannel sounds, the following conventions should be observed: 

 stereo 3 channel 

 4 channel 6 channel 

 channel 

 left right left right center 

 center right surround 

 center right 

 1 2 3 4 5 6 

left left^ center (^) centerright surround left quad frontleft frontright rearleft^ rightrear 


_numSampleFrames_ contains the number of sample frames in the _Sound Data Chunk_. Note that _numSampleFrames_ is the number of sample frames, not the number of bytes nor the number of sample points in the _Sound Data Chunk._ The total number of sample points in the file is _numSampleFrames_ times _numChannels._ 

_sampleSize_ is the number of bits in each sample point. It can be any number from 1 to 32. The format of a sample point will be described in the next section, the _Sound Data Chunk._ 

_sampleRate_ is the sample rate at which the sound is to be played back, in _sample frames_ per second. 

One and only one Common Chunk is required in every FORM AIFF. 


**Sound Data Chunk** 

The Sound Data Chunk contains the actual sample frames. 

#define SoundDataID 'SSND' /* ckID for Sound Data Chunk */ 

typedef struct { 

 ID ckID; long ckSize; 

 unsigned long offset; unsigned long blockSize; unsigned char soundData[]; 

} SoundDataChunk; 

_ckID_ is always 'SSND'. _ckSize_ is the size of the data portion of the chunk, in bytes. It does not include the 8 bytes used by _ckID_ and _ckSize._ 

_offset_ determines where the first sample frame in the _soundData_ starts. _offset_ is in bytes. Most applications won't use _offset_ and should set it to zero. Use for a non-zero _offset_ is explained in the _Block-Aligning Sound Data_ section below. 

_blockSize_ is used in conjunction with _offset_ for block-aligning sound data. It contains the size in bytes of the blocks that sound data is aligned to. As with _offset_ , most applications won't use _blockSize_ and should set it to zero. More information on _blockSize_ is in the _Block-Aligning Sound Data_ section below. 

_soundData_ contains the sample frames that make up the sound. The number of sample frames in the _soundData_ is determined by the _numSampleFrames_ parameter in the _Common Chunk_. 

_Sample Points_ 

Each sample point in a sample frame is a linear, 2's complement value. The sample points are from 1 to 32 bits wide, as determined by the _sampleSize_ parameter in the _Common Chunk_. Sample points are stored in an integral number of contiguous bytes. One to 8 bit wide sample points are stored in one byte, 9 to 16 bit wide sample points are stored in two bytes, 17 to 24 bit wide sample points are stored in 3 bytes, and 25 to 32 bit wide samples are stored in 4 bytes. When the width of a sample point is less than a multiple of 8 bits, the sample point data is left 


justified, with the remaining bits zeroed. An example case is illustrated below. A 12 bit sample point, binary 101000010111, is stored left justified in two bytes. The remaining bits are set to zero. 

 12 bit sample point is left justified 4 bits are zero padded 

 right most 

 1 0 1 0 0 0 0 1 0 1 1 1 0 0 0 0 

_Sample Frames_ 

Sample frames are stored contiguously in order of increasing time. The sample points within a sample frame are packed together, there are no unused bytes between them. Likewise, the sample frames are packed together with no pad bytes. 

_Block-Aligning Sound Data_ 

There may be some applications that, to insure real time recording and playback of audio, wish to align sampled sound data with fixed-size blocks. This can be accomplished with the _offset_ and _blockSize_ parameters, as shown below. 

 Block-aligned sound data 

 numSampleFrames sample frames 

 block N 1 block N block N + 1 block N + 2 

 unused sample frames unused offset bytes 

 soundData[] 

 blockSize bytes 

In the above figure, the first sample frame starts at the beginning of block N. This is accomplished by skipping the first _offset_ bytes of the _soundData._ Note too that the _soundData_ array can extend beyond valid sample frames, allowing the _soundData_ array to end on a block boundary. 

_blockSize_ specifies the size in bytes of the block that is to be aligned to. A _blockSize_ of zero 


indicates that the sound data does not need to be block-aligned. Applications that don't care about block alignment should set _blockSize_ and _offset_ to zero when writing Audio IFF files. Applications that write block-aligned sound data should set _blockSize_ to the appropriate block size. Applications that modify an existing Audio IFF file should try to preserve alignment of the sound data, although this is not required. If an application doesn't preserve alignment, it should set _blockSize_ and _offset_ to zero. If an application needs to realign sound data to a different sized block, it should update _blockSize_ and _offset_ accordingly. 

The Sound Data Chunk is required unless the _numSampleFrames_ field in the _Common Chunk_ is zero. A maximum of one Sound Data Chunk can appear in a FORM AIFF. 


**Marker Chunk** 

The Marker Chunk contains markers that point to positions in the sound data. Markers can be used for whatever purposes an application desires. The _Instrument Chunk_ , defined later in this document, uses markers to mark loop beginning and end points, for example. 

_Markers_ 

A marker has the following format. 

typedef short MarkerId; 

typedef struct { 

 MarkerId id; unsigned long position; pstring markerName; 

} Marker; 

_id_ is a number that uniquely identifies the marker within a FORM AIFF. The id can be any positive non-zero integer, as long as no other marker within the same FORM AIFF has the same id. 

The marker's position in the sound data is determined by _position_. Markers conceptually fall between two sample frames. A marker that falls before the first sample frame in the sound data is at position zero, while a marker that falls between the first and second sample frame in the sound data is at position 1. Note that the units for _position_ are sample frames, not bytes nor sample points. 

 position 0 position 5 position 12 

 Sample Frames 

_markerName_ is a Pascal-style text string containing the name of the mark. 

Note: Some "EA IFF 85" files store strings as C-strings (text bytes followed by a null terminating 


character) instead of Pascal-style strings. Audio IFF uses pstrings because they are more efficiently skipped over when scanning through chunks. Using pstrings, a program can skip over a string by adding the string count to the address of the first character. C strings require that each character in the string be examined for the null terminator. 


_Marker Chunk Format_ 

The format for the data within a Marker Chunk is shown below. 

#define MarkerID 'MARK' /* ckID for Marker Chunk */ 

typedef struct { 

 ID ckID; long ckSize; 

 unsigned short numMarkers; Marker Markers[]; 

} MarkerChunk; 

_ckID_ is always 'MARK'. _ckSize_ is the size of the data portion of the chunk, in bytes. It does not include the 8 bytes used by _ckID_ and _ckSize._ 

_numMarkers_ is the number of markers in the Marker Chunk. 

_numMarkers_ , if non-zero, it is followed by the markers themselves. Because all fields in a marker are an even number of bytes in length, the length of any marker will always be even. Thus, markers are packed together with no unused bytes between them. The markers need not be ordered in any particular manner. 

The Marker Chunk is optional. No more than one Marker Chunk can appear in a FORM AIFF. 


**Instrument Chunk** 

The Instrument Chunk defines basic parameters that an instrument, such as a sampler, could use to play back the sound data. 

_Looping_ 

Sound data can be looped, allowing a portion of the sound to be repeated in order to lengthen the sound. The structure below describes a loop: 

typedef struct { 

 short playMode; MarkerId beginLoop; MarkerId endLoop; 

} Loop; 

A loop is marked with two points, a begin position and an end position. There are two ways to play a loop, forward looping and forward/backward looping. In the case of forward looping, playback begins at the beginning of the sound, continues past the begin position and continues to the end position, at which point playback restarts again at the begin position. The segment between the begin and end positions, called the _loop segment_ , is played over and over again, until interrupted by something, such as the release of a key on a sampling instrument, for example. 

 begin position end position 

 sample frames loop segment 

With forward/backward looping, the loop segment is first played from the begin position to the end position, and then played _backwards_ from the end position back to the begin position. This flip-flop pattern is repeated over and over again until interrupted. 

_playMode_ specifies which type of looping is to be performed. 

#define NoLooping 0 #define ForwardLooping 1 #define ForwardBackwardLooping 2 


If NoLooping is specified, then the loop points are ignored during playback. 

_beginLoop_ is a the marker id that marks the begin position of the loop segment. 

_endLoop_ marks the end position of a loop. The begin position must be less than the end position_._ If this is not the case, then the loop segment has zero or negative length and no looping takes place. 


_Instrument Chunk Format_ 

The format of the data within an Instrument Chunk is described below. 

#define InstrumentID 'INST' /* ckID for Instrument Chunk */ 

typedef struct { 

 ID ckID; long ckSize; 

 char baseNote; char detune; char lowNote; char highNote; char lowVelocity; char highVelocity; short gain; Loop sustainLoop; Loop releaseLoop; 

} InstrumentChunk; 

_ckID_ is always 'INST'. _ckSize_ is the size of the data portion of the chunk, in bytes. For the Instrument Chunk, _ckSize_ is always 20. 

_baseNote_ is the note at which the instrument plays back the sound data without pitch modification. Units are MIDI (MIDI is an acronym for Musical Instrument Digital Interface) note numbers, and are in the range 0 through 127. Middle C is 60. 

_detune_ determines how much the instrument should alter the pitch of the sound when it is played back. Units are in _cents_ (1/100 of a semitone) and range from -50 to +50. Negative numbers mean that the pitch of the sound should be lowered, while positive numbers mean that it should be raised. 

_lowNote_ and _highNote_ specify the suggested range on a keyboard for playback of the sound data. The sound data should be played if the instrument is requested to play a note between the low and high notes, inclusive. The base note does not have to be within this range. Units for _lowNote_ and _highNote_ are MIDI note values. 

_lowVelocity_ and _highVelocity_ specify the suggested range of velocities for playback of the sound data. The sound data should be played if the note-on velocity is is between low and high velocity, 


inclusive. Units are MIDI velocity values, 1 (lowest velocity) through 127 (highest velocity). 

_gain_ is the amount by which to change the gain of the sound when it is played. Units are decibels. For example, 0 db means no change, 6 db means double the value of each sample point, while -6 db means halve the value of each sample point. 

_sustainLoop_ specifies a loop that is to be played when an instrument is sustaining a sound. 

_releaseLoop_ specifies a loop that is to be played when an instrument is in the release phase of playing back a sound. The release phase usually occurs after a key on an instrument is released. 

The Instrument Chunk is optional. No more than one Instrument Chunk can appear in a FORM AIFF. 

**MIDI Data Chunk** 

The MIDI Data Chunk can be used to store MIDI data (please refer to _Musical Instrument Digital Interface Specification 1.0_ , available from the International MIDI Association, for more details on MIDI). 

The primary purpose of this chunk is to store MIDI System Exclusive messages, although other types of MIDI data can be stored in this block as well. As more instruments come on the market, they will likely have parameters that have not been included in the Audio IFF specification. The MIDI System Exclusive messages for these instruments may contain many parameters that are not included in the _Instrument Chunk_. For example, a new sampling instrument may have more than the two loops defined in the _Instrument Chunk_. These loops will likely be represented in the MIDI System Exclusive message for the new machine. This MIDI System Exclusive message can be stored in the MIDI Data Chunk. 

#define MIDIDataID 'MIDI' /* ckID for MIDI Data Chunk */ 

typedef struct { 

 ID ckID; long ckSize; 

 unsigned char MIDIdata[]; 

} MIDIDataChunk; 


_ckID_ is always ' MIDI'. _ckSize_ is the size of the data portion of the chunk, in bytes. It does not include the 8 bytes used by _ckID_ and _ckSize._ 

_MIDIData_ contains a stream of MIDI data. 

The MIDI Data Chunk is optional. Any number of MIDI Data Chunks may exist in a FORM AIFF. If MIDI System Exclusive messages for several instruments are to be stored in a FORM AIFF, it is better to use one MIDI Data Chunk per instrument than one big MIDI Data Chunk for all of the instruments. 


**Audio Recording Chunk** 

The Audio Recording Chunk contains information pertinent to audio recording devices. 

#define AudioRecordingID 'AESD' /* ckID for Audio Recording */ /* Chunk. */ 

typedef struct { 

 ID ckID; long ckSize; 

 unsigned char AESChannelStatusData[24]; 

} AudioRecordingChunk; 

_ckID_ is always 'AESD'. _ckSize_ is the size of the data portion of the chunk, in bytes. For the Audio Recording Chunk, _ckSize_ is always 24. 

The 24 bytes of _AESChannelStatusData_ are specified in the _AES Recommended Practice for Digital Audio Engineering Serial Transmission Format for Linearly Represented Digital Audio Data,_ section 7.1, Channel Status Data. That document describes a format for real-time digital transmission of digital audio between audio devices. This information is duplicated in the Audio Recording Chunk for convenience. Of general interest would be bits 2, 3, and 4 of byte 0, which describe recording emphasis. 

The Audio Recording Chunk is optional. No more than one Audio Recording Chunk may appear in a FORM AIFF. 


**Application Specific Chunk** 

The Application Specific Chunk can be used for any purposes whatsoever by manufacturers of applications. For example, an application that edits sounds might want to use this chunk to store editor state parameters such as magnification levels, last cursor position, and the like. 

#define ApplicationSpecificID 'APPL' /* ckID for Application */ /* Specific Chunk. */ 

typedef struct { 

 ID ckID; long ckSize; 

 OSType applicationSignature; char data[]; 

} ApplicationSpecificChunk; 

_ckID_ is always 'APPL'. _ckSize_ is the size of the data portion of the chunk, in bytes. It does not include the 8 bytes used by _ckID_ and _ckSize._ 

_applicationSignature_ identifies a particular application. For Macintosh applications, this will be the application's four character signature. For Apple II applications, _applicationSignature_ should always be 'pdos', or the hexadecimal bytes 0x70646F73. If _applicationSignature_ is 'pdos', the beginning of the _data_ area is defined to be a Pascal-style string (a length byte followed by ASCII string bytes) containing the name of the application. This is necessary because Apple II applications do not have a four-byte signature as do Macintosh applications. 

_data_ is the data specific to the application. 

The Application Specific Chunk is optional. Any number of Application Specific Chunks may exist in a single FORM AIFF. 


**Comments Chunk** 

The Comments Chunk is used to store comments in the FORM AIFF. "EA IFF 85" has an _Annotation Chunk_ that can be used for comments, but the Comments Chunk has two features not found in the "EA IFF 85" chunk. They are: 1) a timestamp for the comment; and 2) a link to a marker. 

_Comment_ 

A comment consists of a time stamp, marker id, and a text count followed by text. 

typedef struct { 

 unsigned long timeStamp; MarkerID marker; unsigned short count; char text; 

} Comment; 

_timeStamp_ indicates when the comment was created. Units are the number of seconds since January 1, 1904. (This time convention is the one used by the Macintosh. For procedures that manipulate the time stamp, see The Operating System Utilities chapter in _Inside Macintosh, vol II_ ). For a routine that will convert this to an Apple II GS/OS format time, please see Apple II File Type Note for filetype 0xD8, aux type 0x0000. 

A comment can be linked to a marker. This allows applications to store long descriptions of markers as a comment. If the comment is referring to a marker, then _marker_ is the ID of that marker. Otherwise, _marker_ is zero, indicating that this comment is not linked to a marker. 

_count_ is the length of the text that makes up the comment. This is a 16 bit quantity, allowing much longer comments than would be available with a pstring. 

_text_ contains the comment itself. This text must be padded with a byte at the end to insure that it is an even number of bytes in length. This pad byte, if present, is not included in _count_. 

_Comments Chunk Format_ 


#define CommentID 'COMT' /* ckID for Comments Chunk. */ 

typedef struct { 

 ID ckID; long ckSize; 

 unsigned short numComments; Comment comments[]; 

} CommentsChunk; 

_ckID_ is always ' COMT'. _ckSize_ is the size of the data portion of the chunk, in bytes. It does not include the 8 bytes used by _ckID_ and _ckSize._ 

_numComments_ contains the number of comments in the Comments Chunk. This is followed by the comments themselves. Comments are always an even number of bytes in length, so there is no padding between comments in the Comments Chunk. 

The Comments Chunk is optional. No more than one Comments Chunk may appear in a single FORM AIFF. 


**Text Chunks Name, Author, Copyright, Annotation** 

These four chunks are included in the definition of every "EA IFF 85" file. All are text chunks; their data portion consists solely of text. Each of these chunks is optional. 

#define NameID 'NAME' /* ckID for Name Chunk. */ #define AuthorID 'AUTH' /* ckID for Author Chunk. */ #define CopyrightID '(c) ' /* ckID for Copyright Chunk. */ #define AnnotationID 'ANNO' /* ckID for Annotation Chunk. */ 

typedef struct { 

 ID ckID; long ckSize; 

 char text[]; 

} TextChunk; 

_ckID_ is either ' NAME', ' AUTH', '(c) ', or ' ANNO', depending on whether the chunk as a Name Chunk, Author Chunk, Copyright Chunk, or Annotation Chunk, respectively. For the Copyright Chunk, the 'c' is lowercase and there is a space (0x20) after the close parenthesis. 

_ckSize_ is the size of the data portion of the chunk, in this case the _text._ 

_text_ contains pure ASCII characters. It is not a pstring nor a C string. The number of characters in _text_ is determined by _ckSize_. The contents of _text_ depend on the chunk, as described below: 

_Name Chunk_ 

_text_ contains the name of the sampled sound. The Name Chunk is optional. No more than one Name Chunk may exist within a FORM AIFF. 

_Author Chunk_ 

_text_ contains one or more author names. An author in this case is the creator of a sampled sound. The Author Chunk is optional. No more than one Author Chunk may exist within a FORM AIFF. 

_Copyright Chunk_ 


The Copyright Chunk contains a copyright notice for the sound. _text_ contains a date followed by the copyright owner. The chunk ID '(c) ' serves as the copyright characters '©'. For example, a Copyright Chunk containing the text "1988 Apple Computer, Inc." means "© 1988 Apple Computer, Inc." 

The Copyright Chunk is optional. No more than one Copyright Chunk may exist within a FORM AIFF. 

_Annotation Chunk_ 

_text_ contains a comment. Use of this chunk is discouraged within FORM AIFF. The more powerful _Comments Chunk_ should be used instead. The Annotation Chunk is optional. Many Annotation Chunks may exist within a FORM AIFF. 

**Chunk Precedence** 

Several of the local chunks for FORM AIFF may contain duplicate information. For example, the _Instrument Chunk_ defines loop points and MIDI system exclusive data in the _MIDI Data Chunk_ may also define loop points. What happens if these loop points are different? How is an application supposed to loop the sound? 

Such conflicts are resolved by defining a precedence for chunks: 


 Common Chunk 

 Sound Data Chunk Marker Chunk 

 Instrument Chunk 

 Highest Precedence 

 Application Specific Chunk Lowest Precedence 

 MIDI Data Chunk 

 Audio Recording Chunk 

 Comment Chunk 

 Name Chunk 

 Author Chunk 

 Copyright Chunk 

 Annotation Chunk 

The _Common Chunk_ has the highest precedence, while the _Application Specific Chunk_ has the lowest. Information in the _Common Chunk_ always takes precedence over conflicting information in any other chunk. The _Application Specific Chunk_ always loses in conflicts with other chunks. By looking at the chunk hierarchy, for example, one sees that the loop points in the _Instrument Chunk_ take precedence over conflicting loop points found in the _MIDI Data Chunk_. 

It is the responsibility of applications that write data into the lower precedence chunks to make sure that the higher precedence chunks are updated accordingly. 


**Appendix A An Example** 

Illustrated below is an example of a FORM AIFF. An Audio IFF file is simply a file containing a single FORM AIFF. On a Macintosh, the FORM AIFF is stored in the data fork of a file and the file type is 'AIFF'. 

## FORM AIFF 

 Common Chunk 

 Marker Chunk 

 Instrument Chunk 

 Sound Data Chunk 

 numSampleFrames 

 numMarkers position markerName id position markerName 

 id 

8 'e' 'n' 'd' (^) ' ' 'l' 'o' 'o' 'p' 0 2 88200 2 8 'b' 'e' 'g' ' ' 'l' 'o' 'o' 'p' 0 1 44100 numChannels sampleSize sampleRate 2 88200 16 44100.00 ckID ckSize 'COMM' 18 ckID ckSize 'MARK' 34 ckID ckSize 'INST' 20 releaseLoop.playMode releaseLoop.beginLoop sustainLoop.playMode sustainLoop.beginLoop releaseLoop.endLoop sustainLoop.endLoop baseNote detune lowNote highNote lowVelocity highVelocity gain 0 1 60 -3 57 63 1 127 6 1 2 

- 

- ckID ckSize 

 'SSND' 176408 

 ckID ckSize formType 

 'FORM' 'AIFF' 

 176516 

 soundData • • • 

first sample frame (^) 88200th sample frame ch 1 ch 2 ch 1 ch 2 offset blockSize 0 0 



**Appendix B Sending comments to Apple Computer, Inc.** 

If you have suggestions for new chunks to be added to the Audio Interchange File Format, please describe the chunk in as much detail as possible, and give an example of its use. Suggestions for new FORMs, ways to group FORM AIFF's into a bank, and new local chunks are welcome. When sending in suggestions, be sure to mention that your comment refers to the _Audio Interchange File Format: "AIFF"_ document version 1.3. 

Send comments to: 

Developer Technical Support (Apple II or Macintosh) Apple Computer, Inc. 20525 Mariani Avenue, MS: 51-T Cupertino, CA 95014 USA 


**References** 

_AES Recommended Practice for Digital Audio Engineering Serial Transmission Format for Linearly Represented Digital Audio Data,_ Audio Engineering Society, 60 East 42nd Street, New York, New York 10165 

_MIDI: Musical Instrument Digital Interface, Specification 1.0_ , the International MIDI Association. 

_"EA IFF 85" Standard for Interchange Format Files._ Electronic Arts. 

_"8SVX" IFF 8-Bit Sampled Voice._ Electronic Arts. 

_Inside Macintosh, Volume II._ Apple Computer, Inc., Addison Wesley Publishing Company, Inc., 1986. 

_Apple® Numerics Manual_ , Addison Wesley Publishing Company, Inc., 1986. 

_Apple II File Type Note for File Type $D8, Aux Type $0000_ , Apple Computer, Inc., 1989. 



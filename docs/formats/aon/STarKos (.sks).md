# STarKos Tracker Format Reversing Notes

*Version 2024-04-23, initial*

Legend:

* All values mentioned here are hex. All 2+byte values are little-endian.
* `u8` is "unsigned 8-bit value", `s16` is "signed 16-bit value", `ch` is "ASCII character".

The file starts with the header:

| Offset | Value  | Notes  |
| ------ | ------------ | -- |
| 00     | `STK1.0SONG` | Signature |
| 0A     | `ch`[0A] | Author (space-padded) |
| 14     | `ch`[20] | Comments (space-padded) |
| 34     | `u8` | Sample channel (possible values: `1`, `2`, `3`) |
| 35     | `u8` | Orderlist position to loop until (counting from 0) |
| 36     | `u8` | Position to loop from |
| 37     | `s8` | Global transpose (in semitones) |
| 38     | `u8` | Speed at the beginning (`0`～`3F`)                                                |
| 39     | `u8` | Repeat frequency (`0`=13Hz, `1`=25Hz, `2`=50Hz, `3`=100Hz, `4`=150Hz, `5`=300Hz) |
| 3A     | `u8` | Last orderlist position: `ord-1` |
| 3B     | `u8` | ??? |

At offset `3C`, the order list of (ord) elements follows.

Each element has 8 bytes:

| Offset | Value | Notes |
|:------:|:-----:| -- |
| +00    | `u8` | Pattern ("track" in STarKos terms) number for channel 1 - the lowest 8 bits |
| +01    | `s8` | A bit field: `tttt tttn` (`t` = pattern transpose for ch.1, `n`  = the MSB for the pattern number) |
| +02～3  | | Same for channel 2 |
| +04～5  | | Same for channel 3 |
| +06    | `u8` | Height of the pattern: `rows-1`, ie. the number of the last row |
| +07    | `u8` | Special pattern number |

After those, instrument descriptions begin.

Instruments
----

Each instrument structure contains (names as in the tracker UI):

| Offset | Value, name | Notes |
|:--:|:-- | -- |
| +00 | `u16`     | Instrument number (`1`～`FF`). Stop reading the instrument here if it's `FFFF`. |
| +02 | `u16`     | Instrument record's size in bytes (this value included) |
| +04 | `u16`     | Relative loop address (`0` if the instrument is not looped) |
| +06 | `u8` SPD | Speed modifiier (higher is slower) |
| +07 | `u8` RETRIG?  | Retrigger flag (`0` or `1`) |
| +08 | `u8` INSTR END   | Last line number of the instrument, count from 0 |
| +09 | `u8` INSTR LOOP | The instrument lines loop from this one |
| +0A | `u8` LOOP?    | Looped flag |
| +0B | `ch`[8] INST NAME | Instrument name, space-padded |

Individual lines follow, each starts with a byte of flags:

| Value | Notes |
|:--:| -- |
| `u8` | `x`, a bit field where `Hard` flag = 80 and the rest are defined by that |

Then the flags are checked in this way (the structure is flag-defined). 

* If `x` = `00`, skip this instrument line
* If `x` = `FF`, this is the last byte in the pattern (just another way to know where to stop)
* If `Hard` is set:
  * `x` is interpreted as this bit field: `HRSh NPAM` (`R`ST=40, `h`ardFreq=20, `S`ndFreq=10, `N`oise=8, `P`itch shift=4, `A`rp=2, `M`ute=1)
  * `u8` `y`, a bit field: `HFxs sshh` (`H`ardSync=80, `F`Tune=40, `S`FT=7-`sss`, `H`ARD-1=`hh`)  
  Get the values of `SND` = 1-`Mute`, hard envelope, shift, and hardsync & `y`;
  * if `Noise` & `x` is set:
    * `u8` noise
  * if `FTune` & `y` is set (surprisingly out of sequence, yes):
    * `u8` finetune
  * if `Arp` & `x` is set:
    * `s8` arpeggio
  * if `Pitch` & `x` is set:
    * `s16` pitch shift
  * if `sndFreq` & `x` is set:
    * `u16` sound freq
  * if `hardFreq` & `x` is set:
    * `u16` hard freq 
* Otherwise (if `Hard` is *not* set):
  * `x` is interpreted as this bit field: `HPAN vvvv` (`P`itch=40 `A`rp=20 `N`oise_etc=10 vol=`v`)  
  Get volume from `v`
  * If Noise_etc is set:
    * `u8` `y`, a bit field: `xHSn nnnn` (`H`ard = 40, `S`ND = 20, noise=`n`)
      (Hard flag exists here too because it's possible to have noise and `Hard` (just for the snd freq) at once, and we would've missed noise otherwise.)  
    Get noise from `y`.
    * If `SND` & `y` is set:
      * SND is on
    * If `Hard` & `y` is set:
      * `u16` hardcoded sound frequency (0..FFF)
    * Otherwise:
      * If `x` & `0xE0` is non-zero, SND is on
      * If `Arp` & `x` is set:
        * `s8` arp (`-5F`～`+5F`)
      * If `Pitch` & `x` is set:
        * `s16` pitch shift (`-FFF`～`+FFF`). The negative value, ie. the tracker's `-0FFF` is `0FFF` in the file.

Here's the (quasi-C) code that reads and counts all the instruments.

```js
for (ins = 0; !EOF(); ins++) {
  insn = readU16();
  if(insn == 0xFFFF) break;
  isz = readU16(); loopp = readU16();
  ispd = readU8(); ifretrig = readU8(); iend = 1+readU8(); iloop = readU8(); iflooped = readU8();
  iname = readASCII(8);
  for (l = 0; l < iend; l++) {
    lsnd = lvol = lnoise = larp = lpitch = lhard = lsfreq = lhfreq = lshift = lrst = lhsync = lftune = 0;
    x = readU8();
    if(!x) continue; // Mute is unset here but it's the same as snd is 0
    if((x & 0x80)) { // Hard
      lsnd = 1 - (x & 1); if (x & 0x40) lrst = 1;
      y = readU8();
      lhard = 1 + (y & 3); lshift = 7 - ((y>>2) & 7); if (y & 0x80) lhsync = 1;
      if (x & 8) lnoise = readU8();
      if (y & 0x40) lftune = readU8();
      if (x & 2) larp = readS8(); // signed BCD!
      if (x & 4) lpitch = - readS16();
      if (x & 0x10) lsfreq = readU16();
      if (x & 0x20) { lhfreq = readU16(p); p += 2; }
    } else { // common PSG
      lvol = x & 0xF;
      if (x & 0x10) {
        y = File.readU8();
        lnoise = y & 0x1F; lsnd = (y & 0x20) ? 1 : 0;
        if(y & 0x40) lsfreq = readU16(p);
      } else
        if (x & 0xE0) lsnd = 1;
      if (x & 0x20) larp = readS8(); // signed BCD!
      if (x & 0x40) lpitch = - readS16();
    }
  } //line cycle
} //instrument cycle
```

Special patterns begin right after the instruments.

Special Patterns
-----

These declare either the use of a sampled instrument ("digidrum"), or an in-place speed change. Each position in the orderlist may refer to a special pattern in its "special" value.

Each special pattern is built like this.

| Value | Notes |
|:--:| -- |
| `u16` | Pattern number. Like with the instruments, break off reading if it's `FFFF`. |
| `u8` | How many bytes the pattern is (including this value). |

Repeating until the pattern ends:
* `u8` `x`, a bit field: `EDvv vvvv` (`E`mpty=80, `D`igidrum=40, value=`v`)
* If `x` = `FF`, this is the end of the special pattern.
* If `Empty` is set:  
`x` & `7F` is how many empty lines are being skipped besides this one
* Otherwise:
  * If `Digidrum` is set:  
  sample number = `v`
  * Otherwise:  
  speed = `v`

After the special patterns, the common ones start. They're a bit more involved.

Patterns
-----

These are the usual tracker patterns, but each one only holds one channel ("track").

Each pattern starts with:

| Offset | Value | Notes |
|:------:|:-----:| -- |
| +00 | `u16` | Pattern number. Again, stop when it's `FFFF`. |
| +02 | `u16` | How many bytes the pattern is (including this value). |

Then come pattern lines, their length varying based on the flags.

`u8` `x`, a value interpreted as follows.
* If `x` = `FF`, this is the end of the pattern.
* If `80` & `x` is set, this line is empty, and `x & 7F` is how many empty lines follow besides this one.
* Otherwise, if `x` >= `60`, this line has no PSG note, but proceed as follows.
* Otherwise, if `x` = `60`, the volume is set on this line explicitly:
  * `u8` volume. Stored so that the actual volume = `F` - volume.
* Otherwise, if `x` = `61`, the pitch shift is set on this line:  
  * `u8` pitch (negative value, like in the instruments).
* Otherwise, if `x` = `62`, both the volume and the pitch are set:  
  * `u8`  volume
  * `u8` pitch
* Otherwise, if `x` = `63`, insert a cutoff ("reset" in STarKos terms).
* Otherwise, if `x` = `64`, play a sample:
  * `u8` sample (`1`～`1F`)
* Otherwise (x <= 5F), a PSG note is present, provided by the value of `x`. `x` = 00 for `C-0`, `x` = 01 for `C#0`, `x` = 0C for `C-1` etc., and the following value is read.
  * `u8` `y`, a bit field: `xVIPvvvv` (`V`olSkip=40, `I`nstrSkip=20, `P`itch=10, vol=`v`)
  * If `VolSkip` is not set, take the new track volume from `v` as F - (`y` & F).
  * If the instrument has not yet been defined, **OR** `InstrSkip` is not set:
    * `u8` instrument
  * If `Pitch` is set:
    * `u8` pitch (negative value)

Again, here's the code that handles all this. It's sequential and doesn't read the end marker (see below).

```js
const nsymb = ['C-','C#','D-','D#','E-','F-','F#','G-','G#','A-','A#','B-'];
for (i = 0; !EoF() && i <= ptn; i++) {
  var curptn = readU16();
  if (curptn == 0xFFFF) break;
  var psz = readU16();
  var pvol = lncnt = 0, pins = -1 /* instrument undefined */;
  while(!EoF()) {
    var ppitch = 0, pfnote = pfvol = pfpitch = false;
    x = readU8();
    if (x == 0xFF) break;
    else if (x & 0x80)
      lncnt += x & 0x7F;
    else if (x >= 0x60)
      switch(x & 0xF) {
      case 0: pfvol = true;
        pvol = 0xF - readU8();
        break;
      case 1: pfpitch = true;
        ppitch = - read_U8();
        break;
      case 2: pfvol = pfpitch = true;
        pvol = 0xF - readU8();
        ppitch = - readU8();
        break;
      case 3: pfnote = true;
        pnote = "rst";
        break;
      case 4: pfnote = true;
        pnote = "spl";
        pins = readU8();
      }
    else {
      pfnote = true;
      pnote = nsymb[x % 12] + Math.floor(x / 12);
      y = readU8();
      pfvol = !(y & 0x40);
      if (pfvol)
        pvol = 0xF - (y & 0xF);
      if (pins < 0 || !(y & 0x20))
        pins = readU8();
      pfpitch = y & 0x10;
        if (pfpitch)
          ppitch = - readU8();
    }
    lncnt++;
  }
}
```

Footer
------

| Value | Notes |
|:--:| -- |
| `u16` | Equals `1AFF`. Included in the last pattern's size value. |

This field is useful because the CPC only stores filesize in blocks, so garbage tends to be present in the exported files.

------

Kaens (TG `@kaens`)
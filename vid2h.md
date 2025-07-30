# vid2h - convert videos to .h / .c files or binary files

## *NOTE THAT THIS IS WIP!* Currently video compression ratio is rather bad, compression is slow, quality is so-so and decompression is slow too. You have been warned!

## General usage

Call vid2h like this: ```vid2h IMG FORMAT [IMG CONVERSION] [IMG COMPRESSION] [DATA COMPRESSION] [AUDIO CONVERSION] [OPTIONS] INFILE OUTNAME```

* ```IMG FORMAT``` is mandatory and means the color format to convert the input frame to:
  * ```--blackwhite=T``` - Convert frame to b/w paletted image with two colors according to a brightness threshold T [0, 1].
  * ```--paletted=N``` - Convert frame to paletted image with specified number of colors N [2, 256].
  * ```--truecolor=F``` - Convert frame to true-color image format F ["RGB888", "RGB565" or "RGB555"].
  * ```--outformat=F``` - Set final output color format F ["RGB888", "RGB565", "RGB555", "BGR888", "BGR565" or "BGR555"].
* ```IMG CONVERSION``` is optional and means the type of conversion to be done:
  * [```--addcolor0=COLOR```](#adding-a-color-to-index--0-in-the-palette) - Add COLOR at palette index #0 and increase all other color indices by 1.
  * [```--movecolor0=COLOR```](#moving-a-color-to-index--0-in-the-palette) - Move COLOR to palette index #0 and move all other colors accordingly.
  * [```--shift=N```](#shifting-index-values) - Increase image index values by N, keeping index #0 at 0.
  * [```--prune=N```](#pruning-index-values) - Reduce depth of image index values to 1,2 or 4 bit, depending on N.
  * [```--sprites=W,H```](#generating-sprites) - Cut data into sprites of size W x H and store spritewise. You might want to add ```--tiles```.
  * [```--tiles```](#generating-8x8-tiles-for-tilemaps) - Cut data into 8x8 tiles and store data tile-wise.
  * ```--deltaimage``` - Pixel-wise delta encoding between successive images.
* ```IMG COMPRESSION``` is optional, mutually exclusive:
  * ```--dxtg``` - Use DXT1-ish RGB555 intra-frame compression on video.
  * ```--dxtv=QUALITY``` - Use DXT1-ish RGB555 intra- and inter-frame compression on video. QUALITY [0, 100] is a quality factor where higher values == better quality, but worse compression.
* ```DATA COMPRESSION``` is optional:
  * [```--delta8```](#compressing-data) - 8-bit delta encoding ["Diff8bitUnFilter"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--delta16```](#compressing-data) - 16-bit delta encoding ["Diff16bitUnFilter"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--rle```](#compressing-data) - Use RLE compression ["RLUnComp"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--lz10```](#compressing-data) - Use LZ77 compression ["LZ77UnComp variant 10"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--vram```](#compressing-data) - Structure LZ-compressed data safe to decompress directly to VRAM.  
  Valid combinations are e.g. ```--diff8 --lz10``` or ```--lz10 --vram```.
* ```AUDIO CONVERSION``` options are optional:
  * ```channelformat=``` - Audio channel format ["mono" or "stereo"].
  * ```sampleformat=``` - Audio sample format ["s8p" (signed 8-bit), "u8p" (unsigned 8-bit), "s16p" (signed 16-bit), "u16p" (unsigned 16-bit), "f32p" (32-bit floating-point)]. All sample formats are planar (non-interleaved).
  * ```samplerate=``` - Audio sample rate in Hz [4000, 48000].
  Note that all input audio is currently converted to signed 16-bit data and multiple channels are reduced to stereo.
* ```OPTIONS``` are optional:
  * ```--dryrun``` - Process data, but do not write output files.
  * ```--dumpimage``` - Process video data and dump results to *result\*.png* files.
  * ```--dumpaudio``` - Process audio data and dump result to *result.wav* file.
* ```INFILE``` specifies the input video file. Must be readable with FFmpeg.
* ```OUTNAME``` is the (base)name of the output file and also the name of the prefix for #defines and variable names generated. "abc" will generate "abc.h", "abc.c" and #defines / variables names that start with "ABC_". Binary output will be written as "abc.bin".

The order of the operations performed is: Read input file ➜ image format ➜ addcolor0 ➜ movecolor0 ➜ shift ➜ prune ➜ sprites ➜ tiles ➜ dxtg / dxtv ➜ diff8 / diff16 ➜ rle ➜ lz10 ➜ Write output

Some general information:

* Some combinations of options make no sense, but vid2h will not check that.
* All image, color map and audio data stored to output files will be aligned to 4 bytes and padded to 4 bytes. Zero bytes will be added if necessary.

## Binary file storage format

vid2h will store binary file header fields and frame header fields (see [vid2hstructs.h](src/io/vid2hstructs.h), [vid2hio.h](src/io/vid2hio.h) and [vid2hio.cpp](src/io/vid2hio.cpp)):

| Field                                          | Size    |                                                                                                                  |
| ---------------------------------------------- | ------- | ---------------------------------------------------------------------------------------------------------------- |
| **File header**                                |
| Magic bytes "v2h0"                             | 4 bytes | To identify the file type and version (currently "0")                                                            |
| File content type                              | 1 byte  | Bitfield: Audio 0x01, Video 0x02                                                                                 |
| Number of video frames in file                 | 3 bytes |
| Frames / s                                     | 4 bytes | Frames / s in 16:16 fixed-point format                                                                           |
| Frame width in pixels                          | 2 bytes |
| Frame height in pixels                         | 2 bytes |
| Image data bits / pixel                        | 1 byte  | Can be 1, 2, 4, 8, 15, 16, 24                                                                                    |
| Color map data bits / color                    | 1 byte  | Can be 0 (no color map), 15, 16, 24                                                                              |
| Color map entries M                            | 1 byte  | Color map stored if M > 0                                                                                        |
| Number of color map frames in file             | 3 byte  |
| Red-blue channel swap flag                     | 1 byte  | If != 0 red and blue channel are swapped                                                                         |
| Max. intermediate video memory needed          | 3 bytes | Max. intermediate memory needed to decompress a video frame. If 0 size should be determined by video frame size  |
| ---                                            | 1 byte  |
| Number of audio frames in file                 | 3 bytes |
| Number of audio samples per channel            | 4 bytes |
| Audio sample rate                              | 2 bytes | Audio data sample rate in Hz                                                                                     |
| Number of audio channels                       | 1 byte  | Currently only 1 or 2 allowed                                                                                    |
| Audio sample bit depth                         | 1 byte  | Audio data bit depth                                                                                             |
| Audio sample offset                            | 2 bytes | Signed offset of audio relative to video in number of samples                                                    |
| Audio sample offset                            | 2 bytes | Signed offset of audio relative to video in number of samples                                                    |
| Max. intermediate audio memory needed          | 2 bytes | Max. intermediate memory needed to decompress an audio frame. If 0 size should be determined by audio frame size |
|                                                |         |
| **Frame header #0**                            |
| &emsp; Frame data type                         | 1 byte  | Enum: Pixels 0x01, Color map 0x02, Audio 0x03                                                                    |
| &emsp; Frame size                              | 3 bytes | Size of frame data                                                                                               |
| ... one of ...                                 |
| &emsp; **Pixel data chunk #0**                 |
| &emsp; &emsp; Processing type                  | 1 byte  | See following table and [processingtypes.h](src/image/processingtypes.h)                                         |
| &emsp; &emsp; Uncompressed pixel data size     | 3 bytes |
| &emsp; &emsp; Pixel data                       | N bytes | Padded to multiple of 4 (might have multiple layered chunks inside)                                              |
| &emsp; **Color map data chunk #0**             |
| &emsp; &emsp; ---                              | 1 byte  |
| &emsp; &emsp; Uncompressed color map data size | 3 bytes |
| &emsp; &emsp; Color map data                   | N bytes | Padded to multiple of 4 (might have multiple layered chunks inside)                                              |
| &emsp; **Audio data chunk #0**                 |
| &emsp; &emsp; Processing type                  | 1 byte  | See following table and [processingtypes.h](src/audio/processingtypes.h)                                         |
| &emsp; &emsp; Uncompressed audio data size     | 3 bytes |
| &emsp; &emsp; Audio data                       | N bytes | Padded to multiple of 4 (might have multiple layered chunks inside)                                              |
|                                                |         |
| **Frame header #1**                            |
| ...                                            |

Note that (if the file header is aligned to 4 bytes) every *Frame* and every *Data chunk* in the file will be aligned to 4 bytes. If you use aligned memory as a scratchpad when decoding, again, every *Chunk data* will be aligned too.

Image processing type meaning (see also [processingtypes.h](src/image/processingtypes.h)):

| Processing type byte | Meaning                                                         |
| -------------------- | --------------------------------------------------------------- |
| 0                    | Verbatim copy                                                   |
| 50                   | Image data is 8-bit deltas                                      |
| 51                   | Image data is 16-bit deltas                                     |
| 55                   | Image data is signed pixel difference between successive images |
| 60                   | Image data is compressed using LZ77 variant 10                  |
| 65                   | Image data is compressed using run-length-encoding              |
| 70                   | Image data is compressed using DXT                              |
| 71                   | Image data is compressed using DXTV(ideo)                       |
| 128 (ORed w/ type)   | Final compression / processing step on data                     |

Thus a processing chain could be `50, 65, 188` meaning `8-bit deltas, RLE, LZ77 10 (final step)`. A chain of DXVT + LZ10 is a good fit for video.

## Decompression on PC

Use [vid2hplay](src/vid2hplay.cpp) to play back vid2h files.

## Decompression on GBA

An example for a small video player can be found in the [gba](gba) subdirectory.

## Todo

* Much faster DXTV decompression
* VQ-based compression using YCgCoR. Should yield better compression ratio and decompress faster
* Better image / video preview (in + out)

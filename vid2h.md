# vid2h - convert videos to .h / .c files or binary files

## *NOTE THAT THIS IS WIP!* Currently video compression ratio is rather bad, compression is slow, quality is so-so and decompression is slow too. You have been warned!

## General usage

Call vid2h like this: ```vid2h FORMAT [CONVERSION] [IMAGE COMPRESSION] [DATA COMPRESSION] [OPTIONS] INFILE OUTNAME```

* ```FORMAT``` is mandatory and means the color format to convert the input frame to:
  * ```--blackwhite``` - Convert frame to b/w paletted image with two colors according to a brightness threshold.
  * ```--paletted``` - Convert frame to paletted image with specified number of colors.
  * ```--commonpalette``` - Convert all frames to images using a common palette with specified number of colors.
  * ```--truecolor``` - Convert frame to RGB555 / RGB565 / RGB888 true-color image.
* ```CONVERSION``` is optional and means the type of conversion to be done:
  * [```--addcolor0=COLOR```](#adding-a-color-to-index--0-in-the-palette) - Add COLOR at palette index #0 and increase all other color indices by 1.
  * [```--movecolor0=COLOR```](#moving-a-color-to-index--0-in-the-palette) - Move COLOR to palette index #0 and move all other colors accordingly.
  * [```--shift=N```](#shifting-index-values) - Increase image index values by N, keeping index #0 at 0.
  * [```--prune=N```](#pruning-index-values) - Reduce depth of image index values to 1,2 or 4 bit, depending on N.
  * [```--sprites=W,H```](#generating-sprites) - Cut data into sprites of size W x H and store spritewise. You might want to add ```--tiles```.
  * [```--tiles```](#generating-8x8-tiles-for-tilemaps) - Cut data into 8x8 tiles and store data tile-wise.
  * ```--deltaimage``` - Pixel-wise delta encoding between successive images.
* ```IMAGE COMPRESSION``` is optional, mutually exclusive:
  * ```--dxtg``` - Use DXT1-ish RGB555 intra-frame compression on video.
  * ```--dxtv=KEYFRAME_INTERVAL,ALLOWED_ERROR``` - Use DXT1-ish RGB555 intra- and inter-frame compression on video. KEYFRAME_INTERVAL is the interval at which key frames are inserted [0, 60]. 0 means no key frames. ALLOWED_ERROR is a quality factor where higher values mean higher allowed error == worse quality, but better compression [0.01, 1].
* ```DATA COMPRESSION``` is optional:
  * [```--delta8```](#compressing-data) - 8-bit delta encoding ["Diff8bitUnFilter"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--delta16```](#compressing-data) - 16-bit delta encoding ["Diff16bitUnFilter"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--rle```](#compressing-data) - Use RLE compression ["RLUnComp"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--lz10```](#compressing-data) - Use LZ77 compression ["LZ77UnComp variant 10"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
    * [```--vram```](#compressing-data) - Structure LZ-compressed data safe to decompress directly to VRAM.  
  Valid combinations are e.g. ```--diff8 --lz10``` or ```--lz10 --vram```.
* ```OPTIONS``` are optional:
  * ```--dryrun``` - Process data, but do not write output files.
* ```INFILE``` specifies the input video file. Must be readable with FFmpeg.
* ```OUTNAME``` is the (base)name of the output file and also the name of the prefix for #defines and variable names generated. "abc" will generate "abc.h", "abc.c" and #defines / variables names that start with "ABC_". Binary output will be written as "abc.bin".

The order of the operations performed is: Read input file ➜ addcolor0 ➜ movecolor0 ➜ shift ➜ prune ➜ sprites ➜ tiles ➜ dxtg / dxtv ➜ diff8 / diff16 ➜ rle ➜ lz10 ➜ Write output

Some general information:

* Some combinations of options make no sense, but vid2h will not check that.
* All image and color map data stored to output files will be aligned to 4 bytes and padded to 4 bytes. Zero bytes will be added if necessary.

## Binary file storage format

vid2h will store binary file header fields and frame header fields:

| Field                                      | Size     |                                                                               |
| ------------------------------------------ | -------- | ----------------------------------------------------------------------------- |
| *File / Video*                             |
| Magic bytes "v2h_"                         | 4 bytes  | To identify the file type
| Number of frames in file                   | 4 bytes  |
| Frame width in pixels                      | 2 bytes  |
| Frame height in pixels                     | 2 bytes  |
| Frames / s                                 | 1 byte   | No fractions allowed here                                                     |
| Image data bits / pixel                    | 1 byte   | Can be 1, 2, 4, 8, 15, 16, 24                                                 |
| Color map data bits / color                | 1 byte   | Can be 0 (no color map), 15, 16, 24                                           |
| Color map entries M                        | 1 byte   | Color map stored if M > 0                                                     |
| Max. intermediate memory needed            | 4 bytes  | Maximum intermediate storage needed for decompression (for ALL frames)        |
| *Frame #0*                                 |
| &emsp; Frame data chunk size               | 4 bytes  | Padded size of frame data chunk (NOT including the color map size)            |
| &emsp; *Frame data chunk #0*               |
| &emsp; &emsp; Processing type              | 1 byte   | See following table and [processingtypes.h](src/processing/processingtypes.h) |
| &emsp; &emsp; Uncompressed frame data size | 3 bytes  |
| &emsp; &emsp; Frame data                   | N bytes  | Padded to multiple of 4 (might have multiple layered chunks inside)           |
| &emsp; Color map data                      | M colors | Only if M > 0. Padded to multiple of 4                                        |
| *Frame #1*                                 |
| ...                                        |

Note that (if the file header is aligned to 4 bytes) every *Frame* and every *Data chunk* in the file will be aligned to 4 bytes. If you use aligned memory as a scratchpad when decoding, again, every *Chunk data* will be aligned too.

Processing type meaning (see also [processingtypes.h](src/processing/processingtypes.h)):

| Processing type byte | Meaning                                                         |
| -------------------- | --------------------------------------------------------------- |
| 0                    | Verbatim copy                                                   |
| 50                   | Image data is 8-bit deltas                                      |
| 51                   | Image data is 16-bit deltas                                     |
| 55                   | Image data is signed pixel difference between successive images |
| 60                   | Image data is compressed using LZ77 variant 10                  |
| 61                   | Image data is compressed using LZ77 variant 11                  |
| 65                   | Image data is compressed using run-length-encoding              |
| 70                   | Image data is compressed using DXT                              |
| 71                   | Image data is compressed using DXTV(ideo)                       |
| 128 (ORed w/ type)   | Final compression / processing step on data                     |

Thus a processing chain could be `50, 65, 188` meaning `8-bit deltas, RLE, LZ77 10 (final step)`. A chain of DXVT + LZ10 is a good fit for video.

## Decompression on PC

Use [vid2hplay](src/vid2hplay.cpp) to play back vid2h files.

## Decompression on GBA

An example for a small video player (no audio) can be found in the [gba](gba) subdirectory.

## Todo

* Much faster DXTV decompression
* Improve DXTV compression (Cluster-fit DXT block compression + still 2-3 unused bits per block)
* VQ-based compression using YCgCoR. Should yield better compression ratio and decompress faster
* Clean up SDLWindow class
* Better image / video preview (in + out)

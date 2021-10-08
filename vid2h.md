# vid2h - convert videos to .h / .c files or binary files

## General usage

Call vid2h like this: ```vid2h FORMAT [CONVERSION] [IMAGE COMPRESSION] [DATA COMPRESSION] INFILE OUTNAME```

* ```FORMAT``` is mandatory and means the color format to convert the input frame to:
  * ```--binary``` - Convert frame to black / white paletted image with two colors according to a brightness threshold.
  * ```--paletted``` - Convert frame to paletted image with specified number of colors.
  * ```--truecolor``` - Convert frame to RGB555 / RGB565 / RGB888 true-color image.
* ```CONVERSION``` is optional and means the type of conversion to be done:
  * [```--addcolor0=COLOR```](#adding-a-color-to-index--0-in-the-palette) - Add COLOR at palette index #0 and increase all other color indices by 1.
  * [```--movecolor0=COLOR```](#moving-a-color-to-index--0-in-the-palette) - Move COLOR to palette index #0 and move all other colors accordingly.
  * [```--shift=N```](#shifting-index-values) - Increase image index values by N, keeping index #0 at 0.
  * [```--prune```](#pruning-index-values) - Reduce depth of image index values to 4 bit.
  * [```--sprites=W,H```](#generating-sprites) - Cut data into sprites of size W x H and store spritewise. You might want to add ```--tiles```.
  * [```--tiles```](#generating-8x8-tiles-for-tilemaps) - Cut data into 8x8 tiles and store data tile-wise.
  * ```--deltaimage``` - Pixel-wise delta encoding between successive images.
* ```IMAGE COMPRESSION``` is optional, mutually exclusive:
  * ```--dxt1``` - Use DXT1 RGB565 compression on frame.
* ```DATA COMPRESSION``` is optional:
  * [```--delta8```](#compressing-data) - 8-bit delta encoding ["Diff8"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--delta16```](#compressing-data) - 16-bit delta encoding ["Diff16"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--rle```](#compressing-data) - Use RLE compression (http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--lz10```](#compressing-data) - Use LZ77 compression ["variant 10"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--lz11```](#compressing-data) - Use LZ77 compression ["variant 11"](http://problemkaputt.de/gbatek.htm#biosdecompressionfunctions).
  * [```--vram```](#compressing-data) - Structure LZ-compressed data safe to decompress directly to VRAM.  
  Valid combinations are e.g. ```--diff8 --lz10``` or ```--lz10 --vram```.
* ```INFILE``` specifies the input video file. Must be readable with FFmpeg.
* ```OUTNAME``` is the (base)name of the output file and also the name of the prefix for #defines and variable names generated. "abc" will generate "abc.h", "abc.c" and #defines / variables names that start with "ABC_". Binary output will be written as "abc.bin".

The order of the operations performed is: Read all input files ➜ addcolor0 ➜ movecolor0 ➜ shift ➜ prune ➜ sprites ➜ tiles ➜ dxt1 ➜ diff8 / diff16 ➜ rle ➜ lz10 / lz11 ➜ Write output

Some general information:

* Some combinations of options make no sense, but vid2h will not check that.
* All data stored to output files will be aligned to 4 bytes and padded to 4 bytes. Zero bytes will be added if necessary.

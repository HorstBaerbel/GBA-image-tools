# GBA / NDS image and video conversion tools

This folder contains some tools to convert / compress images and videos to GBA and NDS formats:  

* [colormap555](colormap555.cpp) - Generate the file [colormap555.png](colormap555.png) that can be used to convert images directly to the GBA RGB555 color-space with good quality.
* [gimppalette555](gimppalette555.cpp) - Generate the file [GBA.gpl](GBA.gpl) for using / editing / painting with GBA colors in Gimp.
* [img2h](img2h.cpp) - Convert an image (that can be read with [ImageMagick](https://imagemagick.org/index.php)) to a .h / .c file to compile it into your program. Can convert images to a tile- or sprite-compatible format ("1D mapping" order).
* [vid2h](vid2h.cpp) - Compress a list of images using the external tool GBALZSS (included in devKitPro) and convert the compressed data to a .h and .c file, so you can compile it into your program.

If you find a bug or make an improvement your pull requests are appreciated.

## License

All of this is under the [MIT License](LICENSE). Uses the wonderful [cxxopts](https://github.com/jarro2783/cxxopts) for command line argument parsing.

## Prequisites

* You **must** have ImageMagick / [Magick++](https://imagemagick.org/script/magick++.php) installed for compiling. Install it with:

  ```apt install libmagick++-dev```

* You must have [ImageMagick](https://imagemagick.org/index.php) installed for using the "convert" tool. Install it with:

  ```apt install imagemagick```

* For running the [vid2h](vid2h.cpp) tool you must have [devkitPro / devKitARM](https://devkitpro.org) [installed](https://devkitpro.org/wiki/Getting_Started) or the gbalzss tool in your PATH.

## Building

### From the command line

Navigate to the GBA-image-tools folder, then:

```sh
mkdir build && cd build
cmake ..
make
```

### From Visual Studio Code

* **Must**: Install the "C/C++ extension" by Microsoft.
* **Recommended**: If you want intellisense functionality install the "C++ intellisense" extension by austin.
* **Must**: Install the "CMake Tools" extension by vector-of-bool.
* You might need to restart / reload Visual Studio Code if you have installed extensions.
* Open the GBA-image-tools folder using "Open folder...".
* Choose a kit of your choice as your active CMake kit if asked.
* You should be able to build now using F7 and build + run using F5.

## TODO

* Improve docs / README
* Compress with LZSS directly

## Convert images / videos to GBA formats

### Convert an image to GBA RGB555 format with a restricted number of colors

```convert INFILE -colors NROFCOLORS -remap colormap555.png OUTFILE```

This will use the color map [colormap555.png](colormap555.png) to restrict output colors to the GBA color "palette" of possible RGB555 colors, which gives much better results. If you use regular dithering the image will be converted to RGB888 first, dithered and then later converted to RGB555 trying to match colors, which gives worse results.  
If you use wildcards for a list of images, you can create common palette for all images using "[+remap](https://www.imagemagick.org/script/command-line-options.php?#remap)" instead:

```convert INFILE -colors 256 +remap colormap555.png png8:OUTFILE```

Use a wildcard for INFILE, e.g. "in*.png". This will create a common palette from all images it finds for "in*.png" first and store it. Then the conversion to a palette image is done. The "png8:" prefix tells ImageMagick to store indexed 8-bit PNGs.  
To make sure your files are correctly ordered for further processing steps add "%0Xd" (printf-style) to the output to append a custom number to the file name:

```convert INFILE -colors 256 +remap colormap555.png png8:foo_%02d.png```

This will generate the file names "foo_00.png", "foo_01.png" and so on.

If you have images with larger flat areas of color and they come out all garbled due to the dithering, try the "[-posterize](https://www.imagemagick.org/script/command-line-options.php?#posterize)" option instead:

```convert INFILE -posterize 6 -remap colormap555.png OUTFILE```

### Making a color the transparent color / move it to index #0 in the palette

On GBA the color at palette index #0 is always transparent. When using ImageMagick for color mapping / conversion the correct color might not end up being the first in the color palette. You can use the option ```--movecolor0``` in [img2h](img2h.cpp) to move a color to index #0 in the palette:

```img2h --movecolor0 COLORVALUE INFILE OUTNAME```

COLORVALUE is a hex color value, e.g. "AA2345" or "123def".

Some conversions leave you with the correct color palette, but no transparent color at index #0. You can use the option ```--addcolor0``` in [img2h](img2h.cpp) to add a specific color at index #0 in the palette:

```img2h --addcolor0 COLORVALUE INFILE OUTNAME```

COLORVALUE is a hex color value, e.g. "AA2345" or "123def".

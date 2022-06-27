# GBA homebrew image and video conversion tools

This folder contains some tools to convert / compress images and videos to GBA formats for homebrew development:  

* [colormap555](src/colormap555.cpp) - Generate the file [colormap555.png](colormap555.png) that can be used to convert images directly to the GBAs RGB555 color-space with good quality.
* [gimppalette555](src/gimppalette555.cpp) - Generate the file [GBA.gpl](GBA.gpl) for using / editing / painting with GBA colors in Gimp.
* [hex2gba](src/hex2gba.cpp) - Convert a RGB888 color to GBA RGB555 / BGR555 high-color format.
* [img2h](src/img2h.cpp) - Convert / compress a (list of) image(s) that can be read with [ImageMagick](https://imagemagick.org/index.php) to a .h / .c file to compile them into your program. Can convert images to a tile- or sprite-compatible format ("1D mapping" order) and compress them with RLE or LZ77. Suitable to compress small image sequences too. Documentation is [here](img2h.md).
* [vid2h](src/vid2h.cpp) - Convert / compress a a video that can be read with [FFmpeg](https://www.ffmpeg.org/) to a .h / .c file to compile them into your program. Can convert images to a tile- or sprite-compatible format ("1D mapping" order) and compresses them using intra- and inter-frame techniques and RLE, LZ77 or DXT. Documentation is [here](vid2h.md).

If you find a bug or make an improvement your pull requests are appreciated.

## License

All of this is under the [MIT License](LICENSE). It uses:

* [cxxopts](https://github.com/jarro2783/cxxopts) for command line argument parsing.
* [glob](https://github.com/p-ranav/glob) for input file globbing / wildcards.
* [Eigen](https://gitlab.com/libeigen) for math and video compression.
* [Catch2](https://github.com/catchorg/Catch2) for unit tests.

## Prequisites

* You **must** have ImageMagick / [Magick++](https://imagemagick.org/script/magick++.php) installed for compiling. Install it with:

  ```apt install libmagick++-dev``` or ```dnf install libmagick++-devel```

* You must have [ImageMagick](https://imagemagick.org/index.php) installed for using the "convert" tool. Install it with:

  ```apt install imagemagick``` or ```dnf install imagemagick```

* You **must** have [OpenMP](https://www.openmp.org/) installed for compiling. Install it with:

  ```apt install libomp-dev``` or ```dnf install libomp-devel```

* You **must** have [FFmpeg](https://www.ffmpeg.org/) installed for compiling vid2h. Install it with:

  ```apt install libavcodec-dev libavformat-dev libavutil-dev libswscale-dev``` or ```dnf install libavcodec-devel libavformat-devel libavutil-devel libswscale-devel```

* For compressing data with LZ77 you need to have [devkitPro / devKitARM](https://devkitpro.org) [installed](https://devkitpro.org/wiki/Getting_Started) and the environment variable ```$DEVKITPRO``` set, or the [gbalzss](https://github.com/devkitPro/gba-tools) tool in your ```$PATH```.

## Building

### From the command line

Navigate to the GBA-image-tools folder, then:

```sh
mkdir build && cd build
cmake ..
make
```

To build a release package, call:

```sh
make package
```

### From Visual Studio Code

* **Must**: Install the "C/C++ extension" by Microsoft.
* **Recommended**: If you want intellisense functionality install the "C++ intellisense" extension by austin.
* **Must**: Install the "CMake Tools" extension by Microsoft.
* You might need to restart / reload Visual Studio Code if you have installed extensions.
* Open the GBA-image-tools folder using "Open folder...".
* Choose a kit of your choice as your active CMake kit if asked.
* You should be able to build now using F7 and build + run using F5.

# watplot: Interactive Waterfall Plots
&copy; Alex Yu 2019

This is a work in progress.

## Overview
A interactive waterfall plot for Breakthrough Listen data, supporting dragging, zooming, and adjusting colormaps on-the-fly for small to moderately large(1-2GB) files. Currently, only .fil (sigproc filterbank) files are supported. HDF5 support is planned.

## Build

*Dependencies:*

- OpenCV 3.3+
- Eigen 3 (included in repo)

*Build:* Use cmake on all platforms: `mkdir build && cd build && cmake ..`

Then use `make -j4` to build. On Windows, you may double-click the sln file generated to open Visual Studio to build.

You may need to specify the installation directory for OpenCV, OpenCV_dir.

## Usage

`watplot file.fil`


*Controls:*

- left click and drag to move

- mouse wheel to zoom

- right click and drag to adjust colormap (vertically: scale color range, horizontally: shift color range). Initially, the program will try to find a good colormap, but sometimes adjusting will reveal features in the data.

- `s` to save the plot to disk (will be waterfall-xxx.png)

- `q` to quit

Sample data (Voyager): [https://storage.googleapis.com/gbt_fil/voyager_f1032192_t300_v2.fil](https://storage.googleapis.com/gbt_fil/voyager_f1032192_t300_v2.fil)

Try zooming into Voyager's transmitting frequency, 8420.15 MHz, and you should see a powerful signal.

## License
Apache 2.0.

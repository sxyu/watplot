# watplot: Interactive Waterfall Plots
&copy; Alex Yu 2019

This is a work in progress.

## Overview
A interactive waterfall plot visualizer for Breakthrough Listen data, supporting dragging, zooming, and adjusting colormaps on-the-fly for small to moderately large (1-2GB) files. Currently, .fil (sigproc filterbank) files and .h5/.hdf5 (HDF5) files are supported.

## Build

*Dependencies:*

- OpenCV 3.3+
    - `sudo apt-get install libopencv-dev` on Ubuntu
- HDF5 from [https://www.hdfgroup.org/downloads/](https://www.hdfgroup.org/downloads/)
    - `sudo apt-get install libhdf5-dev` on Ubuntu
- Eigen 3 (included in repo)

*Build:* Use cmake on all platforms: `mkdir build && cd build && cmake ..`

Then use `make -j4` to build. On Windows, you may double-click the sln file generated to open Visual Studio to build. You might also need to specify the installation directory for OpenCV, by passing `-DOpenCV_dir='path'` to cmake.

## Usage

On the Breakthrough Listen computing cluster:
`watplot file [f_start[%] f_stop[%] [t_start[%] t_end[%]]]`

Where
- `file` may be a Filterbank (.fil) or HDF5 (.h5) data file in the Breakthrough Listen format
- `f_start, f_end` specify the frequency range, default everything
- `t_start, t_end` specify the time range, default everything
- Append % after any frequency or time value to use a percent of the data instead of specifying the explicit values.
    - For example, `watplot file.fil 0% 50%` loads the lower half of the frequencies

*GUI Controls:*

- left click and drag to move

- mouse wheel or use `+`, `=` keys to zoom

- `0` to reset zoom and position to initial

- right click and drag to adjust colormap (vertically: scale color range, horizontally: shift color range). Initially, the program will try to find a good colormap, but sometimes adjusting will reveal features in the data.

- `c`, `C` keys to cycle through colormaps

- `s` to save the plot to disk (will be waterfall-xxx.png)

- `q` to quit

Sample data (Voyager): [https://storage.googleapis.com/gbt_fil/voyager_f1032192_t300_v2.fil](https://storage.googleapis.com/gbt_fil/voyager_f1032192_t300_v2.fil)

Try zooming into Voyager's transmitting frequency, 8420.15 MHz, and you should see a powerful signal.

## License
Apache 2.0.

#include "stdafx.h"
#include "core.hpp"
#include "util.hpp"
#include "fsutil.hpp"

namespace {
    const char VERSION[] = "0.1.3 alpha";
    void CallBackFunc(int event, int x, int y, int flags, void* userdata)
    {
        static int mouse_down;
        static int mouse_down_flags;
        static int mouse_down_x, mouse_down_y;
        static int mouse_x, mouse_y;
        static cv::Rect2d rect;
        static cv::Size plot_size;
        static double scalex, scaley;
        static double color_scale, color_offset;
        static bool init_log_scale;
        auto * watrend = (watplot::Renderer *) (userdata);

        if (event == cv::EVENT_RBUTTONDOWN ||
            event == cv::EVENT_LBUTTONDOWN || event == cv::EVENT_MBUTTONDOWN) {
            mouse_down_x = x;
            mouse_down_y = y;
            mouse_down_flags = flags;

            if (event == cv::EVENT_LBUTTONDOWN)
            {
                rect = watrend->render_rect;
                plot_size = watrend->plot_size;
                scalex = rect.width / plot_size.height;
                scaley = rect.height / plot_size.width;
                mouse_down = 1;
            }
            else if (event == cv::EVENT_RBUTTONDOWN)
            {
                rect = watrend->render_rect;
                init_log_scale = watrend->log_scale;
                if (init_log_scale) {
                    color_scale = watrend->log_color_scale;
                    color_offset = watrend->log_color_offset;
                } else {
                    color_scale = watrend->color_scale;
                    color_offset = watrend->color_offset;
                }
                mouse_down = 2;
            }
            else //if (event == cv::EVENT_MBUTTONDOWN)
            {
                rect = watrend->render_rect;
                mouse_down = 3;
            }
        }
        else if (event == cv::EVENT_RBUTTONUP ||
            event == cv::EVENT_LBUTTONUP || event == cv::EVENT_MBUTTONUP)
        {
            mouse_down = 0;
        }
        else if (event == cv::EVENT_MOUSEWHEEL
#ifndef _WIN32
                || event == cv::EVENT_MOUSEHWHEEL
#endif
                )
        {
            // wheel scrolling, if supported (currently only on Windows sadly)
            rect = watrend->render_rect;
            double delt = cv::getMouseWheelDelta(flags);
#ifdef _WIN32
            double scale = 1.0 - delt / 1e4;
#else
            // this works for thinkpad pointing stick + middle button on Ubuntu
            double scale = 1.0 + delt / 1e2;
#endif
            cv::Point2d p = watrend->plot_to_time_freq(cv::Point2d(mouse_x, mouse_y));
            cv::Rect2d new_rect(rect.x - (p.x - rect.x) * (scale - 1.0), rect.y - (p.y - rect.y) * (scale - 1.0),
                rect.width * scale, rect.height * scale);
            watrend->render_rect = new_rect;
            watrend->render();
        }
        else if (event == cv::EVENT_MOUSEMOVE)
        {
            mouse_x = x;
            mouse_y = y;
            if (mouse_down == 1) {
                if (mouse_down_flags & cv::EVENT_FLAG_CTRLKEY) {
                    // ctrl + left: alt way to zoom

                    rect = watrend->render_rect;
                    double deltx = y - mouse_down_y;
                    double scalex = 1.0 - deltx / 4e3;
                    double delty = x - mouse_down_x;
                    double scaley = 1.0 - delty / 4e3;
                    if (mouse_down_flags & cv::EVENT_FLAG_SHIFTKEY) {
                        // shift: force same ratio
                        delty = deltx;
                        scaley = scalex;
                    }
                    cv::Point2d p = watrend->plot_to_time_freq(cv::Point2d(mouse_down_x, mouse_down_y));
                    cv::Rect2d new_rect(rect.x - (p.x - rect.x) * (scalex - 1.0),
                            rect.y - (p.y - rect.y) * (scaley - 1.0),
                            rect.width * scalex, rect.height * scaley);
                    watrend->render_rect = new_rect;
                    watrend->render();

                } else {
                    // pan
                    cv::Rect2d new_rect(rect.x + (y - mouse_down_y) * scalex, rect.y - (x - mouse_down_x) * scaley,
                        rect.width, rect.height);
                    watrend->render_rect = new_rect;
                    watrend->render();
                }
            }
            else if (mouse_down == 2) {
                // modify color
                if (init_log_scale) {
                    watrend->log_color_scale = color_scale * (1.0 + (y - mouse_down_y) * 4e-4);
                    watrend->log_color_offset = color_offset + (x - mouse_down_x) * 4e-4;
                } else {
                    watrend->color_scale = color_scale * (1.0 + (y - mouse_down_y) * 1e-3);
                    watrend->color_offset = color_offset + (x - mouse_down_x) * 1e6;
                }
                watrend->render();
            }
            /*
            else if (mouse_down == 3) {
               // drag middle button: not used
            }
            */

        }
    }

    double parse_dbl(char * str, double a, double b) {
        size_t len = strlen(str);
        if (len == 0) return 0.0;
        double result;
        if (str[len - 1] == '%') {
            str[len - 1] = 0;
            result = std::atof(str) / 100.0 * a + b;
            str[len - 1] = '%';
        }
        else {
            result = std::atof(str);
        }
        return result;
    }
}

int main(int argc, char** argv) {
    using namespace watplot;

    std::cout << std::fixed << std::setprecision(17);
    std::cerr << std::fixed << std::setprecision(17);
    std::cout << "watplot v" << VERSION << " - Interactive Waterfall Plotting Utility\n";
    std::cout << "(c) Alex Yu / Breakthrough Listen 2019\n\n";
    std::cout << "formats supported: .fil .h5./hdf5\n";

    bool stat = (argc >= 2 && strcmp(argv[1], "stat") == 0);

    if (argc != 2 + stat && argc != 4 + stat && argc != 6 + stat) {
        std::cerr << "\nusage: watplot [stat] <data_file_path> [f_start[%] f_stop[%] [t_start[%] t_stop[%]]]\n\n";
        std::cerr << "stat: if specified, displays header information without loading (ignores f, t range).\n";
        std::cerr << "f_start, f_stop: frequency range. Append '%' to use percent of max range of data,\n                 e.g., watplot file 0% 50%.\n";
        std::cerr << "t_start, t_stop: time range.\n";
        std::exit(0);
    }

    if (stat && argc != 3) {
        std::cerr << "WARNING: stat specified, range arguments ignored\n";
    }

    if (!stat) {
        std::cout << "Tips:\n"
        "- Left click and drag mouse OR use WASD to pan\n"
        "- To zoom, use:\n  - The scroll wheel OR\n  - -,= keys OR\n"
        "  - Control + Shift + Left click and drag vertically\n"
        "- Control + Left click and drag to zoom asymmetrically\n  Drag horizontally for frequency, vertically for time\n"
        "- Press 0 to reset zoom and position\n"
        "- Right click and drag mouse to adjust colormap (horizontal: shift, vertical: scale)\n"
        "- Press C, Shift + C to cycle through colormaps (15 total)\n"
        "- Press L to toggle log scale\n"
        "- Press Shift + S to save plot to ./waterfall-NUM.png\n"
        "- Press Q or ESC to exit\n"
        "\n"; }

    std::cout << "Loading data headers, please wait...\n";

    std::string path = argv[1 + stat];
    std::string ext = path.substr(path.find_last_of(".") + 1);

    const std::string WIND_NAME = "Interactive Waterfall Plot - " + std::string(path);
    cv::namedWindow(WIND_NAME, cv::WINDOW_NORMAL);
    Renderer::Ptr watrend;

    cv::Rect2d default_rect;
    if (ext == "fil") {
        Filterbank::Ptr fb = std::make_shared<Filterbank>(path);
        default_rect = fb->get_full_rect();
        watrend = std::make_shared<WaterfallRenderer<Filterbank>>(fb, WIND_NAME);
    }
    else if (ext == "h5" || ext == "hdf5" ) {
        HDF5::Ptr hdf5 = std::make_shared<HDF5>(path);
        default_rect = hdf5->get_full_rect();
        watrend = std::make_shared<WaterfallRenderer<HDF5>>(hdf5, WIND_NAME);
    }
    else {
        std::cerr << "Error: Unrecognized extension: \"" << ext << "\". Only .h5, .hdf5, .fil supported.\n";
        std::exit(5);
    }

    if (argc >= 4) {
        double f_start = parse_dbl(argv[2 + stat], default_rect.height, default_rect.y);
        double f_stop = parse_dbl(argv[3 + stat], default_rect.height, default_rect.y);
        if (f_start >= f_stop) {
            std::cerr << "Error: Frequency range is empty!\n";
            std::exit(6);
        }
        default_rect.height = f_stop - f_start;
        default_rect.y = f_start;
        if (argc >= 6) {
            double t_start = parse_dbl(argv[4 + stat], default_rect.width, default_rect.x);
            double t_stop = parse_dbl(argv[5 + stat], default_rect.width, default_rect.x);
            default_rect.width = t_stop - t_start;
            default_rect.x = t_start;
            if (t_start >= t_stop) {
                std::cerr << "Error: Time range is empty!\n";
                std::exit(6);
            }
        }
    }

    if (stat) { return 0; }

    watrend->render_rect = default_rect;
    watrend->render();
    cv::Mat init_rend = watrend->render(2);

    cv::resizeWindow(WIND_NAME, init_rend.cols, init_rend.rows);

    cv::setMouseCallback(WIND_NAME, CallBackFunc, watrend.get());
    int saveid = 0;
    while (true) {
        int k = cv::waitKey(1);
        // WASD: pan
        if (k == 'a') {
             watrend->render_rect.y -= watrend->render_rect.height / 80.0;
             watrend->render();
        } else if (k == 'd') {
             watrend->render_rect.y += watrend->render_rect.height / 80.0;
             watrend->render();
        } else if (k == 's') {
             watrend->render_rect.x -= watrend->render_rect.width / 80.0;
             watrend->render();
        } else if (k == 'w') {
             watrend->render_rect.x += watrend->render_rect.width / 80.0;
             watrend->render();
        } else if (k == 'S') {
            // shift + s: save
            std::string fname = "waterfall-" + util::padleft(++saveid, 3, '0') + ".png";
            cv::imwrite(fname, watrend->get_last_render());
            std::cout << "Saved to: " << fname << "\n";
        } else if (k == 'l') {
            // l: log scale
            watrend->log_scale ^= 1;
            watrend->render();
            std::cout << "Log scale: " <<
                (watrend->log_scale ? "ON" : "OFF") << "\n";
        } else if (k == 61 || k == 45) {
            // = (+) key, - key resp.: zoom
            double scale = k == 61 ? (1.0 - 3e-2) : (1.0 + 3e-2) ;
            cv::Rect2d rect = watrend->render_rect;
            cv::Rect2d new_rect(rect.x - (rect.width / 2) * (scale - 1.0),
                    rect.y - (rect.height / 2) * (scale - 1.0),
                    rect.width * scale, rect.height * scale);
            watrend->render_rect = new_rect;
            watrend->render();
        } else if (k == '0') {
            // 0: reset scale
            watrend->render_rect = default_rect;
            watrend->render();
        }
        else if (k == 'c' || k == 'C') {
            if (k == 'c') {
                ((watrend->colormap += 1) %=
                    static_cast<int>(consts::COLORMAPS.size()));
            }
            else {
                ((watrend->colormap += 14) %=
                    static_cast<int>(consts::COLORMAPS.size()));
            }
            watrend->render();
            std::cout << "Using colormap: " <<
                consts::COLORMAPS[watrend->colormap] << "\n";
        }
        else if (k == 'q' || k == 27) break;
    }

    return 0;
}

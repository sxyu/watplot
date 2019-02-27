#include "stdafx.h"
#include "core.hpp"
#include "util.hpp"
#include "fsutil.hpp"
#include "filterbank.hpp"
#include "waterfall.hpp"
#include "consts.hpp"

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    static int mouse_down;
    static int mouse_down_x, mouse_down_y;
    static cv::Rect2d rect;
    static cv::Size plot_size;
    static double scalex, scaley;
    static cv::Vec2d color;
    auto * watrend = (watplot::WaterfallRenderer<watplot::Filterbank> *) (userdata);

    if (event == cv::EVENT_RBUTTONDOWN ||
        event == cv::EVENT_LBUTTONDOWN || event == cv::EVENT_MBUTTONDOWN) {
        mouse_down_x = x;
        mouse_down_y = y;

        if (event == cv::EVENT_LBUTTONDOWN)
        {
            rect = watrend->get_render_rect();
            plot_size = watrend->get_plot_size();
            scalex = rect.width / plot_size.width;
            scaley = rect.height / plot_size.height;
            mouse_down = 1;
        }
        else if (event == cv::EVENT_RBUTTONDOWN)
        {
            rect = watrend->get_render_rect();
            color = watrend->get_color_scale_offset();
            mouse_down = 2;
        }
        else //if (event == cv::EVENT_MBUTTONDOWN)
        {
            rect = watrend->get_render_rect();
            mouse_down = 3;
        }
    }
    else if (event == cv::EVENT_RBUTTONUP ||
             event == cv::EVENT_LBUTTONUP || event == cv::EVENT_MBUTTONUP)
    {
        mouse_down = 0;
    }
    else if (event == cv::EVENT_MOUSEWHEEL)
    {
        // wheel scrolling, if supported 
        rect = watrend->get_render_rect();
        double delt = cv::getMouseWheelDelta(flags);
        double scale = 1.0 - delt / 1e4;
        cv::Point2d p = watrend->plot_to_time_freq(cv::Point2d(x, y));
        cv::Rect2d new_rect(rect.x - (p.x - rect.x) * (scale - 1.0), rect.y - (p.y - rect.y) * (scale - 1.0),
            rect.width * scale, rect.height * scale);
        watrend->set_render_rect(new_rect);
    }
    else if (event == cv::EVENT_MOUSEMOVE)
    {
        if (mouse_down == 1) {
            cv::Rect2d new_rect(rect.x + (y - mouse_down_y) * scalex, rect.y - (x - mouse_down_x) * scaley,
                                rect.width, rect.height);
            watrend->set_render_rect(new_rect);
        }
        else if (mouse_down == 2) {
            cv::Vec2d new_color(color[0] * (1.0 + (y - mouse_down_y) * 1e-3), color[1] - (x - mouse_down_x) * 1e6);
            watrend->set_color_scale_offset(new_color);
        }
        /*
        else if (mouse_down == 3) {
            // drag middle button: not used
        }
        */

    }
}

int main(int argc, char** argv) {
    using namespace watplot;
    //create_dir(OUT_DIR);
    //create_dir(DATA_DIR);

    std::cout << std::fixed << std::setprecision(17);
    std::cerr << std::fixed << std::setprecision(17);
    std::cout << "watplot v0.1 alpha - Interactive Waterfall Plotting Utility\n";
    std::cout << "(c) Alex Yu / Breakthrough Listen 2019\n";
    std::cout << "This is an early unstable version, for proof-of-concept purposes only\n";
    std::cout << "Only the filterbank (.fil) format is supported at the moment\n\n";
    std::cout << "Tips:\n- Right click and drag mouse to adjust colormap (horizontal: shift, vertical: scale)\n";
    std::cout << "- Use the scroll wheel OR -,= keys to zoom\n";
    std::cout << "- Press 0 to reset zoom\n";
    std::cout << "- Press s to save plot to ./waterfall-NUM.png\n";
    std::cout << "- Press q or ESC to exit\n";
    std::cout << "\n";

    if (argc != 2)  {
        std::cerr << "Usage: watplot filterbank_file\n";
        std::exit(0);
    }
    std::cout << "Loading filterbank, please wait...\n";

    std::string path;// = "D:/bl/test_fil.fil";
    path = argv[1];
    Filterbank fb(path);
    std::cout << fb;

    const std::string WIND_NAME = "Interactive Waterfall Plot - " + std::string(path);
    cv::namedWindow(WIND_NAME);
    WaterfallRenderer<Filterbank> watrend(fb, WIND_NAME);

    watrend.render(2);

    cv::setMouseCallback(WIND_NAME, CallBackFunc, &watrend);
    int saveid = 0;
    while (true) {
        int k = cv::waitKey(0);
        if (k == 'q' || k == 27) break;
        else if (k == 's') {
            std::string fname = "waterfall-" + util::padleft(++saveid, 3, '0') + ".png";
            cv::imwrite(fname, watrend.get_last_render());
            std::cout << "Saved to: " << fname << "\n";
        } 
        else if (k == 61 || k == 45) {
            // = (+) key, - key resp.
            double scale = k == 61 ? (1.0 - 1.5e-2) : (1.0 + 1.5e-2) ;
            cv::Rect2d rect = watrend.get_render_rect();
            cv::Rect2d new_rect(rect.x - (rect.width / 2) * (scale - 1.0),
                    rect.y - (rect.height / 2) * (scale - 1.0),
                    rect.width * scale, rect.height * scale);
            watrend.set_render_rect(new_rect);
        } else if (k == '0') {
            // reset scale
            watrend.set_render_rect(fb.get_full_rect());
        }
    }
    return 0;
}

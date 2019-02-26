#include "stdafx.h"
#include "core.hpp"
#include "util.hpp"
#include "fsutil.hpp"
#include "filterbank.hpp"
#include "waterfall.hpp"
#include "consts.hpp"

void CallBackFunc(int event, int x, int y, int flags, void* userdata)
{
    static bool mouse_down_left = false, mouse_down_right = false;
    static int mouse_down_x, mouse_down_y;
    static cv::Rect2d rect;
    static cv::Size plot_size;
    static double scalex, scaley;
    static cv::Vec2d color;
    auto * watrend = (watplot::WaterfallRenderer<watplot::Filterbank> *) (userdata);

    if (event == cv::EVENT_LBUTTONDOWN)
    {
        // one at a time
        mouse_down_right = false;
        mouse_down_x = x;
        mouse_down_y = y;
        rect = watrend->get_render_rect();
        plot_size = watrend->get_plot_size();
        scalex = rect.width / plot_size.width;
        scaley = rect.height / plot_size.height;
        mouse_down_left = true;
    }
    else if (event == cv::EVENT_LBUTTONUP)
    {
        mouse_down_left = false;
    }
    else if (event == cv::EVENT_RBUTTONDOWN)
    {
        mouse_down_left = false;
        mouse_down_x = x;
        mouse_down_y = y;
        color = watrend->get_color_scale_offset();
        mouse_down_right = true;
    }
    else if (event == cv::EVENT_RBUTTONUP)
    {
        mouse_down_right = false;
    }
    else if (event == cv::EVENT_MBUTTONDOWN)
    {
    }
    else if (event == cv::EVENT_MOUSEWHEEL)
    {
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
        if (mouse_down_left) {
            cv::Rect2d new_rect(rect.x + (y - mouse_down_y) * scalex, rect.y - (x - mouse_down_x) * scaley,
                                rect.width, rect.height);
            watrend->set_render_rect(new_rect);
        }
        else if (mouse_down_right) {
            cv::Vec2d new_color(color[0] * (1.0 + (y - mouse_down_y) * 1e-3), color[1] - (x - mouse_down_x) * 1e6);
            watrend->set_color_scale_offset(new_color);
        }
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
    }
    return 0;
}

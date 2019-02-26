#include "stdafx.h"
#include "core.hpp"
#include "util.hpp"
#include "fsutil.hpp"
#include "filterbank.hpp"

// internal linkage
namespace {
}

int main(int argc, char** argv) {
    using namespace watmaps;
    //create_dir(OUT_DIR);
    //create_dir(DATA_DIR);

    std::cout << std::fixed << std::setprecision(17);
    std::cerr << std::fixed << std::setprecision(17);

    Filterbank fb("/bl/test_fil.fil");
    cv::namedWindow("Waterfall");
    cv::Mat m = cv::Mat::zeros(fb.nints, fb.nchans, CV_32F);
    for (int i = 0; i < fb.nints; ++i) {
        for (int j = 0; j < fb.nchans; ++j) {
            if (fb.foff < 0) {
                m.at<float>(i, j) = log(fb.data(fb.nchans - j - 1, i));
            }
            else {
                m.at<float>(i, j) = log(fb.data(j, i));
            }
        }
    }

    int wid = 900, hi = wid * 9 / 16;

    cv::resize(m, m, cv::Size(wid, hi), 0, 0, cv::InterpolationFlags::INTER_LINEAR);
    cv::Mat m8u, wat;
    cv::normalize(m, m, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    m.convertTo(m8u, CV_8UC1);
    util::applyColorMapViridis(m8u, wat);
    cv::imshow("Waterfall", wat);

    cv::waitKey(0);
    return 0;
}

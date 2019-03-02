#pragma once
#include "util.hpp"
#include "renderer.hpp"

namespace watplot {
    /** Renderer for interactive waterfall plot */
    template <class BLFileType>
    class WaterfallRenderer : public Renderer {
    public:
        typedef std::shared_ptr<WaterfallRenderer> Ptr;

        // terminology: render=what is displayed to user; view=chunk of data cached in memory, larger than render
        /** Create plot from file, initial rectangle, plot size, etc.
          * Does not take ownership of file, so please do not destroy it before the waterfall.
          * @param file pointer to data file
          * @param wind_name OpenCV window name. If empty, does not show plot (although image is still returned by render())
          * @param init_render_rect initial rectangle to render (by default, renders entire file)
          * @param plot_size size of output plot
          * @param color if true, colors output plot 
          * @param colormap colormap to use. 0...14: 0...12 OpenCV colormaps
                                                     13 viridis, 14 grayscale (no map)
          * @param axes if true, draws axes and colorbar on output plot
          */
        WaterfallRenderer(const std::shared_ptr<BLFileType> file, const std::string & wind_name, 
            const cv::Rect & init_render_rect = cv::Rect(0, 0, 0, 0),
            const cv::Size & plot_size = cv::Size(600, 400), bool color = true, int colormap = 13, bool axes = true)
                : file(file), Renderer(wind_name, init_render_rect, plot_size, color, colormap, axes) {
            if (init_render_rect.width == 0) {
                render_rect = file->get_full_rect();
            }
            else {
                render_rect = init_render_rect;
            }

            int64_t mem_limit = consts::MEMORY / sizeof(float);
            // find appropriate amount of memory to allocate
            view_scale_x = static_cast<int>(sqrt(mem_limit / plot_size.area()));
            view_scale_y = view_scale_x;
            // support very skinny data
            if (plot_size.height * view_scale_x > file->nints) {
                view_scale_x = static_cast<double>(file->nints) / plot_size.height;
                view_scale_y = floor(mem_limit / file->nints) / plot_size.width;
            }
            else if (plot_size.width * view_scale_y > file->header.nchans) {
                view_scale_y = file->header.nchans / plot_size.width;
                view_scale_x = floor(mem_limit / file->header.nchans) / plot_size.height;
            }

            static const float COLOR_FACT = 1.75f, MAX_FACT = 2.5e-4f;
            float mean_scaled = ((1.f - MAX_FACT) * float(file->mean_val) + MAX_FACT * float(file->max_val));
            color_scale = 255.f / (mean_scaled * COLOR_FACT - float(file->min_val) / COLOR_FACT);
            color_offset = -float(file->min_val) / COLOR_FACT;
        }

    protected:
        /** Render to an image of size plot_size
          * @param recompute_view 2=force recompute; 0=force use old view; 1=smart
          * @param rendered image. CV_8UC3
          */
        virtual cv::Mat _render(int recompute_view = 1) override {
            if (recompute_view == 2) {
                // placeholder implementation, if recompute_view=1 should recompute when needed
                view_rect = file->view(render_rect, view,
                    static_cast<int>(plot_size.height * view_scale_x), static_cast<int>(plot_size.width * view_scale_y));
                update_dxy();
                std::cerr << "Waterfall-render: Updated view\n";
            }

            cv::Mat wat_raw(plot_size, CV_32F), wat_gray, wat_color;
            std::vector<std::thread> thd_mgr;
            static const unsigned int N_THREADS = std::thread::hardware_concurrency();

            auto worker = [&](unsigned int i) {
                cv::Point2i pt;
                for (pt.y = int(i); pt.y < wat_raw.rows; pt.y += N_THREADS) {
                    float * ptr = wat_raw.ptr<float>(pt.y);
                    for (pt.x = 0; pt.x < wat_raw.cols; ++pt.x) {
                        ptr[pt.x] = (compute_pixel(pt) + color_offset) * color_scale;
                    }
                }
            };
            for (unsigned int i = 0; i < N_THREADS; ++i) {
                thd_mgr.emplace_back(worker, i);
            }
            for (unsigned int i = 0; i < N_THREADS; ++i) {
                thd_mgr[i].join();
            }

            wat_raw.convertTo(wat_gray, CV_8U);
            if (color) {
                util::applyColorMap(wat_gray, wat_color, false, colormap);
            }
            else {
                cv::cvtColor(wat_gray, wat_color, CV_GRAY2BGR);
            }

            if (axes) {
                // axes
                double dx = render_rect.width / plot_size.height;
                double dy = render_rect.height / plot_size.width;
                for (int i = 30; i < wat_color.cols - 10; i += wat_color.cols / 6) {
                    cv::putText(wat_color, util::round(i * dy + render_rect.y, 2),
                        cv::Point(i, wat_color.rows - 15),
                        0, 0.4, cv::Scalar(255, 255, 255));
                }
                for (int i = 50; i < wat_color.rows - 10; i += wat_color.rows / 10) {
                    cv::putText(wat_color, util::round(i * dx + render_rect.x, 2), cv::Point(10, wat_color.rows - i),
                        0, 0.4, cv::Scalar(255, 255, 255));
                }
                cv::putText(wat_color, "MHz", cv::Point(wat_color.cols - 45, wat_color.rows - 35), 0,
                    0.35, cv::Scalar(50, 50, 255));
                cv::putText(wat_color, "s", cv::Point(74, 30), 0,
                    0.4, cv::Scalar(50, 50, 255));

                // spectrum
                const int spectrum_height = 100, spectrum_pad_top_bot = 15, spectrum_border_top = 3;
                cv::Mat spectrum_gray(spectrum_height + spectrum_pad_top_bot * 2, wat_color.cols, CV_8U), spectrum_color;
                spectrum_gray = 0;
                for (int j = 0; j < plot_size.height; j += plot_size.height / 60) {
                    for (int i = 0; i < plot_size.width; ++i) {
                        int pix = wat_gray.at<uint8_t>(j, i);
                        int h = static_cast<int>(pix) * spectrum_height / 256;
                        if (h <= 0 || h >= spectrum_height) continue;
                        cv::Point pt(i, spectrum_height - h - 1 + spectrum_pad_top_bot);
                        if (pix > 115) {
                            spectrum_gray.at<uint8_t>(pt.y-1, pt.x) = pix;
                            spectrum_gray.at<uint8_t>(pt.y+1, pt.x) = pix;
                            if (pt.x) spectrum_gray.at<uint8_t>(pt.y, pt.x-1) = pix;
                            spectrum_gray.at<uint8_t>(pt.y, pt.x+1) = pix;
                        }
                        spectrum_gray.at<uint8_t>(pt) = pix;
                    }
                }
                if (color) {
                    util::applyColorMap(spectrum_gray, spectrum_color, false, colormap);
                }
                else {
                    cv::cvtColor(spectrum_gray, spectrum_color, CV_GRAY2BGR);
                }
                cv::putText(spectrum_color, "Spectrum", cv::Point(10, spectrum_color.rows - 15), 0, 0.35,
                    cv::Scalar(50, 50, 255));
                cv::vconcat(cv::Mat::zeros(spectrum_border_top, spectrum_color.cols, CV_8UC3), spectrum_color, spectrum_color);
                cv::vconcat(wat_color, spectrum_color, wat_color);

                // colorbar
                const int cb_wid = 67, cb_border_left = 3;
                cv::Mat cb_gray(wat_color.rows, cb_wid, CV_8U), cb_color;
                int cb_step = cb_gray.rows / 255;
                for (int i = 0; i < cb_gray.rows; ++i) {
                    cb_gray.row(i) = 255 - i / cb_step;
                }
                if (color) {
                    util::applyColorMap(cb_gray, cb_color, false, colormap);
                }
                else {
                    cv::cvtColor(cb_gray, cb_color, CV_GRAY2BGR);
                }
                for (int i = 30; i < 250; i += 23) {
                    double power_db = 10 * log10(i / color_scale - color_offset);
                    if (isnan(power_db)) continue;
                    cv::putText(cb_color, util::round(power_db, 2),
                        cv::Point(10, cb_color.rows * (255 - i) / 255 + 5), 0, 0.4, cv::Scalar(255, 255, 255));
                }

                cv::putText(cb_color, "dB", cv::Point(cb_color.cols - 38, cb_color.rows - 15), 0,
                    0.4, cv::Scalar(50, 50, 255));

                // leave a gap to left of colorbar
                cv::hconcat(cv::Mat::zeros(cb_color.rows, cb_border_left, CV_8UC3), cb_color, cb_color);
                cv::hconcat(wat_color, cb_color, wat_color);
            }

            last_render = wat_color;
            if (!wind_name.empty()) {
                cv::imshow(wind_name, wat_color);
            }
            return wat_color;
        }

    private:
        /** Pointer to file to plot */
        const std::shared_ptr<BLFileType> file;
    };
}

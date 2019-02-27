#pragma once
#include "util.hpp"

namespace watplot {
    /** Backend for interactive waterfall plot */
    template <class BLFileType>
    class WaterfallRenderer {
    public:
        // terminology: render=what is displayed to user; view=chunk of data cached in memory, larger than render
        /** Create plot from file, initial rectangle, plot size, etc.
          * Does not take ownership of file, so please do not destroy it before the waterfall.
          * @param file data file
          * @param wind_name OpenCV window name. If empty, does not show plot (although image is still returned by render())
          * @param init_render_rect initial rectangle to render (by default, renders entire file)
          * @param plot_size size of output plot
          * @param color if true, colors output plot (using viridis colormap)
          * @param axes if true, draws axes and colorbar on output plot
          */
        WaterfallRenderer(const BLFileType & file, const std::string & wind_name, 
            const cv::Rect & init_render_rect = cv::Rect(0, 0, 0, 0),
            const cv::Size & plot_size = cv::Size(600, 400), bool color = true, bool axes = true)
                : file(file), wind_name(wind_name), plot_size(plot_size), color(color), axes(axes) {
            if (init_render_rect.width == 0) {
                render_rect = file.get_full_rect();
            }
            else {
                render_rect = init_render_rect;
            }
            int64_t mem_limit = consts::MEMORY / sizeof(double);
            // find appropriate amount of memory to allocate
            view_scale_x = static_cast<int>(sqrt(mem_limit / plot_size.area()));
            view_scale_y = view_scale_x;
            // support very skinny data
            if (plot_size.height * view_scale_x > file.nints) {
                view_scale_x = static_cast<double>(file.nints) / plot_size.height;
                view_scale_y = floor(mem_limit / file.nints) / plot_size.width;
            }
            else if (plot_size.width * view_scale_y > file.header.nchans) {
                view_scale_y = file.header.nchans / plot_size.width;
                view_scale_x = floor(mem_limit / file.header.nchans) / plot_size.height;
            }

            double color_fact = 1.1;
            color_scale = 255. / (file.mean_val * color_fact - file.min_val / color_fact) * 0.7;
            color_offset = -file.min_val / color_fact;
        }

        volatile bool rendering = false;

        /** Render to an image of size plot_size
          * @param recompute_view 2=force recompute; 0=force use old view; 1=smart
          * @param rendered image. CV_8UC3
          */
        cv::Mat render(int recompute_view = 1) {
            if (rendering) return last_render;
            rendering = true;
            if (recompute_view == 2) {
                // placeholder implementation, if recompute_view=1 should recompute when needed
                view_rect = file.view(render_rect, view,
                    static_cast<int>(plot_size.height * view_scale_x), static_cast<int>(plot_size.width * view_scale_y));
                std::cerr << "Waterfall-render: Updated view\n";
            }

            cv::Mat wat_raw(plot_size, CV_32F), wat_gray, wat_color;
            cv::Point2i pt;
            for (pt.y = 0; pt.y < wat_raw.rows; ++pt.y) {
                float * ptr = wat_raw.ptr<float>(pt.y);
                for (pt.x = 0; pt.x < wat_raw.cols; ++pt.x) {
                    ptr[pt.x] = float((compute_pixel(pt) + color_offset) * color_scale);
                }
            }

            wat_raw.convertTo(wat_gray, CV_8U);
            if (color) {
                util::applyColorMapViridis(wat_gray, wat_color);
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

                // colorbar
                const int cb_wid = 67;
                cv::Mat cb_gray(wat_color.rows, cb_wid, CV_8U), cb_color;
                int cb_step = (cb_gray.rows / 255);
                for (int i = 0; i < cb_gray.rows; ++i) {
                    cb_gray.row(i) = 255 - i / cb_step;
                }
                if (color) {
                    util::applyColorMapViridis(cb_gray, cb_color);
                }
                else {
                    cv::cvtColor(cb_gray, cb_color, CV_GRAY2BGR);
                }
                for (int i = 16; i < 255; i += 32) {
                    cv::putText(cb_color, util::round((i / color_scale - color_offset) / 1e9, 2),
                        cv::Point(10, cb_color.rows * (255 - i) / 255 + 5), 0, 0.4, cv::Scalar(255, 255, 255));
                }
                // leave a gap to left of colorbar
                cv::hconcat(cv::Mat::zeros(cb_color.rows, 3, CV_8UC3), cb_color, cb_color);
                cv::hconcat(wat_color, cb_color, wat_color);
            }

            last_render = wat_color;
            if (!wind_name.empty()) {
                cv::imshow(wind_name, wat_color);
            }
            rendering = false;
            return wat_color;
        }

        /** get the plot size */
        const cv::Size & get_plot_size() const {
            return plot_size;
        }

        /** modify the plot size */
        cv::Mat set_plot_size(const cv::Size & new_size) {
            plot_size = new_size;
            return render();
        }

        /** get the color scale and offset */
        cv::Vec2d get_color_scale_offset() const {
            return cv::Vec2d(color_scale, color_offset);
        }

        /** set the color scale and offset */
        cv::Mat set_color_scale_offset(const cv::Vec2d & new_values) {
            color_scale = new_values[0];
            color_offset = new_values[1];
            return render();
        }

        /** get the rendered rectangle */
        const cv::Rect2d & get_render_rect() const {
            return render_rect;
        }

        /** modify the rendered rectangle */
        cv::Mat set_render_rect(const cv::Rect2d & new_rect) {
            render_rect = new_rect;
            return render();
        }

        /** get the last render */
        cv::Mat get_last_render() const {
            return last_render;
        }


        /** helper for projecting plot point to (time, frequency) space */
        inline cv::Point2d plot_to_time_freq(cv::Point2d point) const {
            double dx = render_rect.width / plot_size.height;
            double dy = render_rect.height / plot_size.width;
            point.y = plot_size.height - point.y - 1;
            std::swap(point.x, point.y);
            point.x = point.x * dx + render_rect.x;
            point.y = point.y * dy + render_rect.y;
            return point;
        }

    private:
        /** helper for projecting screen point to view space */
        inline cv::Point2d plot_to_view(cv::Point2d point) const {
            double rdx = render_rect.width / plot_size.height;
            double rdy = render_rect.height / plot_size.width;
            double vdx = view_rect.width / (view.cols() - 2);
            double vdy = view_rect.height / (view.rows() - 2);

            point.y = plot_size.height - point.y - 1;
            std::swap(point.x, point.y);
            point.x += (render_rect.x - view_rect.x) / rdx;
            point.y += (render_rect.y - view_rect.y) / rdy;
            point.x *= rdx / vdx;
            point.y *= rdy / vdy;

            return point;
        }


        inline double compute_pixel(const cv::Point2i & point) const {
            cv::Point2d tl = plot_to_view(cv::Point2i(point.x, point.y + 1));
            cv::Point2d br = plot_to_view(cv::Point2i(point.x + 1, point.y));

            tl.x = max(tl.x, 0.);
            tl.y = max(tl.y, 0.);
            br.x = min(br.x, view.cols() - 1.);
            br.y = min(br.y, view.rows() - 1.);

            if (br.x <= 0. || br.y <= 0. || tl.x > view.cols() || tl.y >= view.rows()) return 0.0;
            double area = (br.x - tl.x) * (br.y - tl.y);
            if (area <= 0.0) return 0.0;

            cv::Point2i tl_i(int(ceil(tl.x)), int(ceil(tl.y)));
            cv::Point2i br_i(int(floor(br.x)), int(floor(br.y)));
            cv::Point2i tl_o(int(floor(tl.x)), int(floor(tl.y)));
            cv::Point2i br_o(int(ceil(br.x)), int(ceil(br.y)));

            double area_i = double(br_i.x - tl_i.x) * (br_i.y - tl_i.y);
            double area_o = max(double(br_o.x - tl_o.x) * (br_o.y - tl_o.y), area);

            double ans_i = 0.0;
            ans_i += view(br_i.y, br_i.x) - view(tl_i.y, br_i.x) - view(br_i.y, tl_i.x) + view(tl_i.y, tl_i.x);
            ans_i /= area_i;

            double ans_o = 0.0;
            ans_o += view(br_o.y, br_o.x) - view(tl_o.y, br_o.x) - view(br_o.y, tl_o.x) + view(tl_o.y, tl_o.x);
            ans_o /= area_o;
            if (area_i <= 0) return ans_o;

            // interpolate between inner, outer areas
            double fo = (area - area_i) / area_o;
            return fo * ans_o + (1. - fo) * ans_i;
        }

        const BLFileType & file;

        /** Color scale */
        double color_scale;

        /** Color offset */
        double color_offset;

        /** Whether to color map the output plot */
        bool color;

        /** Whether to draw axes on output plot */
        bool axes;

        /** The output plot size */
        cv::Size plot_size;

        /** The current view */
        Eigen::MatrixXd view;

        /** The window name */
        const std::string wind_name;

        /** The boundaries of the current view */
        cv::Rect2d view_rect;

        /** The boundaries of the current render */
        cv::Rect2d render_rect;

        /** Last render */
        cv::Mat last_render;

        /** Amount by which view is larger than render; determined according to system memory size */
        double view_scale_x, view_scale_y;
    };
}

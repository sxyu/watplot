#pragma once
#include "util.hpp"

namespace watplot {
    /** Abstract base class for renderers */
    class Renderer {
    public:
        typedef std::shared_ptr<Renderer> Ptr;

        // terminology: render=what is displayed to user; view=chunk of data cached in memory, larger than render
        /** Render to an image of size plot_size (must be implemented in child class)
          * @param recompute_view 2=force recompute; 0=force use old view; 1=smart
          * @param rendered image. CV_8UC3
          */
        cv::Mat render(int recompute_view = 1);

        /** Get the last render */
        cv::Mat get_last_render() const;

        /** Helper for projecting plot point to (time, frequency) space */
        cv::Point2d plot_to_time_freq(cv::Point2d point) const;


        /** render parameters */

        /** The output plot size */
        cv::Size plot_size;

        /** The boundaries of the current render */
        cv::Rect2d render_rect;
        

        /** Whether to color map the output plot */
        bool color;

        /** Color scale for linear scale */
        float color_scale;

        /** Color offset for linear scale */
        float color_offset;

        /** Color scale for log scale */
        float log_color_scale;

        /** Color offset for log scale */
        float log_color_offset;

        /** Colormap ID 
          * 0...14: 0...12 OpenCV colormaps 13 viridis,
          * 14 grayscale (no map) */
        int colormap;

        /** Whether to draw axes on output plot */
        bool axes;

        /** Whether log scale is used for coloring output */
        bool log_scale = false;

    protected:

        /**
         * Generic renderer constructor
         * @param wind_name OpenCV window name. If empty, does not show plot (although image is still returned by render())
         * @param init_render_rect initial rectangle to render (by default, renders entire file)
         * @param plot_size size of output plot
         * @param color if true, colors output plot
         * @param colormap colormap to use. 0...14: 0...12 OpenCV colormaps
                                                    13 viridis, 14 grayscale (no map)
         * @param axes if true, draws axes and colorbar on output plot
         */
        Renderer(const std::string & wind_name, 
            const cv::Rect & init_render_rect = cv::Rect(0, 0, 0, 0),
            const cv::Size & plot_size = cv::Size(600, 400), bool color = true, int colormap = 13, bool axes = true)
            : wind_name(wind_name), plot_size(plot_size), color(color), colormap(colormap), axes(axes) {
            render_rect = init_render_rect;
        }

        /** render implmentation */
        virtual cv::Mat _render(int recompute_view = 1) = 0;

        /** update ratios*/
        void update_dxy();

        /** Helper for projecting plot point to view space */
        inline cv::Point2d plot_to_view(cv::Point2d point) const;

        /** Compute the power at a particular plot pixel */
        float compute_pixel(const cv::Point2i & point) const;

        /** The current view */
        Eigen::MatrixXd view;

        /** The window name */
        const std::string wind_name;

        /** The boundaries of the current view */
        cv::Rect2d view_rect;

        /** Last render */
        cv::Mat last_render;

        /** Amount by which view is larger than render; determined according to system memory size */
        double view_scale_x, view_scale_y;

        /** cached ratios */
        double dx, dy, px, py;
    };
}

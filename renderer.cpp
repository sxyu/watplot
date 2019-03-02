#include "stdafx.h"
#include "renderer.hpp"

namespace watplot {
    cv::Mat Renderer::render(int recompute_view)
    {
        update_dxy();
        cv::Mat res = _render(recompute_view);
        return res;
    }

    const cv::Size & Renderer::get_plot_size() const {
        return plot_size;
    }

    cv::Mat Renderer::set_plot_size(const cv::Size & new_size) {
        plot_size = new_size;
        return render();
    }

    cv::Vec2f Renderer::get_color_scale_offset() const {
        return cv::Vec2f(color_scale, color_offset);
    }

    cv::Mat Renderer::set_color_scale_offset(const cv::Vec2f & new_values) {
        color_scale = new_values[0];
        color_offset = new_values[1];
        return render();
    }

    int Renderer::get_colormap() const
    {
        return colormap;
    }

    cv::Mat Renderer::set_colormap(int colormap)
    {
        this->colormap = colormap;
        return render();
    }

    const cv::Rect2d & Renderer::get_render_rect() const {
        return render_rect;
    }

    cv::Mat Renderer::set_render_rect(const cv::Rect2d & new_rect) {
        render_rect = new_rect;
        return render();
    }

    cv::Mat Renderer::get_last_render() const {
        return last_render;
    }

    cv::Point2d Renderer::plot_to_time_freq(cv::Point2d point) const {
        double dx = render_rect.width / plot_size.height;
        double dy = render_rect.height / plot_size.width;
        point.y = plot_size.height - point.y - 1;
        std::swap(point.x, point.y);
        point.x = point.x * dx + render_rect.x;
        point.y = point.y * dy + render_rect.y;
        return point;
    }

    void Renderer::update_dxy()
    {
        double rdx = render_rect.width / plot_size.height;
        double rdy = render_rect.height / plot_size.width;
        double vdx = view_rect.width / (view.cols() - 2);
        double vdy = view_rect.height / (view.rows() - 2);
        dx = rdx / vdx;
        dy = rdy / vdy;
        px = (render_rect.x - view_rect.x) / rdx;
        py = (render_rect.y - view_rect.y) / rdy;
    }

    cv::Point2d Renderer::plot_to_view(cv::Point2d point) const {

        double tmp = point.x;
        point.x = (plot_size.height - point.y - 1 + px) * dx;
        point.y = (tmp + py) * dy;
        return point;
    }

    float Renderer::compute_pixel(const cv::Point2i & point) const {
        cv::Point2d tl = plot_to_view(cv::Point2i(point.x, point.y + 1));
        cv::Point2d br = plot_to_view(cv::Point2i(point.x + 1, point.y));

        tl.x = max(tl.x, 0.f);
        tl.y = max(tl.y, 0.f);
        br.x = min(br.x, view.cols() - 1.f);
        br.y = min(br.y, view.rows() - 1.f);

        if (br.x <= 0.f || br.y <= 0.f || tl.x >= view.cols() - 1.f || tl.y >= view.rows() - 1.f) return 0.0f;
        double area = double(br.x - tl.x) * (br.y - tl.y);
        if (area <= 0.0f) return 0.0f;

        cv::Point2i tl_i(int(ceil(tl.x)), int(ceil(tl.y)));
        cv::Point2i br_i(int(br.x), int(br.y));
        cv::Point2i tl_o(int(tl.x), int(tl.y));
        cv::Point2i br_o(int(ceil(br.x)), int(ceil(br.y)));

        double area_i = (br_i.x - tl_i.x) * (br_i.y - tl_i.y);
        double area_o = max((br_o.x - tl_o.x) * (br_o.y - tl_o.y), area);

        double ans_i = view(br_i.y, br_i.x) - view(tl_i.y, br_i.x) - view(br_i.y, tl_i.x) + view(tl_i.y, tl_i.x);
        ans_i /= area_i;

        double ans_o = view(br_o.y, br_o.x) - view(tl_o.y, br_o.x) - view(br_o.y, tl_o.x) + view(tl_o.y, tl_o.x);
        ans_o /= area_o;
        if (area_i <= 0) return ans_o;

        // interpolate between inner, outer areas
        float fo = (area - area_i) / area_o;
        return fo * ans_o + (1. - fo) * ans_i;
    }
}


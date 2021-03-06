#pragma once
#include<string>
#include "blfile.hpp"
namespace watplot {
    /* Implementation of filterbank file loader */
    class Filterbank : public BLFile<Filterbank> {
    friend class BLFile<Filterbank>;
    public:
        typedef std::shared_ptr<Filterbank> Ptr;

        /* Load filterbank file from the given path */
        explicit Filterbank(const std::string & path) : BLFile<Filterbank>(path) { }
        int64_t header_end;
    protected:
        /* load implementation */
        void _load(const std::string & path);

        /* view implementation */
        void _view(const cv::Rect2d & rect, Eigen::MatrixXd & out, int64_t t_lo, int64_t t_hi, int64_t t_step,
                                                                   int64_t f_lo, int64_t f_hi, int64_t f_step) const;

        static const std::string FILE_FORMAT_NAME;
    };
}

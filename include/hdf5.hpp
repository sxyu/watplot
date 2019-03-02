#pragma once
#include<string>
#include<H5Cpp.h>
#include "blfile.hpp"
namespace watplot {
    /* Implementation of HDF5 file loader */
    class HDF5 : public BLFile<HDF5> {
    friend class BLFile<HDF5>;
    public:
        typedef std::shared_ptr<HDF5> Ptr;

        /* Load HDF5 file from the given path */
        explicit HDF5(const std::string & path) : BLFile<HDF5>(path) { }

    protected:
        /* load implementation */
        void _load(const std::string & path);

        /* view implementation */
        void _view(const cv::Rect2d & rect, Eigen::MatrixXd & out, int64_t t_lo, int64_t t_hi, int64_t t_step,
                                                                   int64_t f_lo, int64_t f_hi, int64_t f_step) const;

        static const std::string FILE_FORMAT_NAME;
        static const std::string DATASET_SUBSET_NAME;
    };
}

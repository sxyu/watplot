#pragma once
#include<string>
#include<vector>
#include<fstream>
#include "blfile.hpp"
namespace watplot {
    /* Implementation of filterbank file loader
     * Note: only 32-bit float data currently supported. */
    class Filterbank : public BLFile<Filterbank> {
    friend class BLFile<Filterbank>;
    public:
        /* Create filterbank file from a file */
        explicit Filterbank(const std::string & path) { load(path); }
        int64_t header_end;
    protected:
        /* load implementation */
        void _load(const std::string & path);

        /* view implementation */
        cv::Rect2d _view(const cv::Rect2d & rect, Eigen::MatrixXd & out, int max_wid, int max_hi) const;

        /** helper for reading the entire header
         *  @return position, in bytes, at end of header */
        int64_t _read_header(std::ifstream & ifs);

        /* helper for reading one keyword from the header */
        bool _read_next_header_keyword(std::ifstream & ifs, std::string & kwd);

        static const std::string FILE_FORMAT_NAME;
    };
}

#pragma once
#include<string>
#include<vector>
#include<fstream>
#include "blfile.hpp"
namespace watmaps {
    /* Implementation of filterbank file loader
     * Note: only 32-bit float data currently supported. */
    class Filterbank : public BLFile<Filterbank> {
    friend class BLFile<Filterbank>;
    public:
        /* Create filterbank file from a file */
        explicit Filterbank(const std::string & path) { load(path); }
        long long header_end;

        Eigen::MatrixXf data;
    protected:
        /* load implementation */
        void _load(const std::string & path);
        /* helper to read the header */
        long long _read_header(std::ifstream & ifs);
        /* helper to read one keyword from the header */
        bool _read_next_header_keyword(std::ifstream & ifs, std::string & kwd);
    };
}

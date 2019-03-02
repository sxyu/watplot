#include "stdafx.h"
#include "filterbank.hpp"
#include "util.hpp"

namespace {
    // helpers
    /* helper for reading one keyword from the header */
    bool _read_next_header_keyword(std::ifstream & ifs, std::string & kwd, watplot::Filterbank::Header & header)
    {
        uint32_t kwdlen;
        ifs.read((char*)& kwdlen, sizeof(kwdlen));

        kwd.resize(kwdlen);
        ifs.read(&kwd[0], kwdlen);
        if (kwd == "HEADER_START" || kwd == "HEADER_END") return false;

        char dtype = watplot::consts::HEADER_KEYWORD_TYPES.at(kwd);

        switch (dtype) {
        case 'l':
        {
            int data;
            ifs.read((char*)&data, sizeof(data));
            if (kwd == "telescope_id") header.telescope_id = data;
            else if (kwd == "machine_id") header.machine_id = data;
            else if (kwd == "data_type") header.data_type = data;
            else if (kwd == "barycentric") header.barycentric = data != 0;
            else if (kwd == "pulsarcentric") header.pulsarcentric = data != 0;
            else if (kwd == "nbits") header.nbits = data;
            else if (kwd == "nsamples") header.nsamples = data;
            else if (kwd == "nchans") header.nchans = data;
            else if (kwd == "nifs") header.nifs = data;
            else if (kwd == "nbeams") header.nbeams = data;
            else header.ibeam = data; // ibeam
        }
        break;
        case 'd':
        {
            double data;
            ifs.read((char*)&data, sizeof(data));
            if (kwd == "az_start") header.az_start = data;
            else if (kwd == "za_start") header.za_start = data;
            else if (kwd == "tstart") header.tstart = data;
            else if (kwd == "tsamp") header.tsamp = data;
            else if (kwd == "fch1") header.fch1 = data;
            else if (kwd == "foff") header.foff = data;
            else if (kwd == "refdm") header.refdm = data;
            else header.period = data; // period
        }
        break;
        case 's':
        {
            uint32_t len;
            ifs.read((char*)& len, sizeof(len));
            std::string data;
            data.resize(len);
            ifs.read(&data[0], len);

            if (kwd == "rawdatafile") header.rawdatafile = data;
            else header.source_name = data; // source_name
        }
        break;
        case 'a':
        {
            double raw, data;
            ifs.read((char*)& raw, sizeof(raw));
            data = watplot::util::double_to_angle(raw);
            if (kwd == "src_raj") header.src_raj = data;
            else header.src_dej = data; // src_dej
        }
        break;
        default:
            std::cerr << "Unsupported header keyword: " << kwd << "\n";
            std::exit(2);
        }
        return true;
    }

    /** helper for reading the entire header
     *  @return position, in bytes, at end of header */
    int64_t _read_header(std::ifstream & ifs, watplot::Filterbank::Header & header)
    {
        std::string keyword;

        // check this is a blimpy file
        if (_read_next_header_keyword(ifs, keyword, header) || keyword != "HEADER_START") {
            std::cerr << "Not a BLIMPY filterbank file!\n";
            std::exit(1);
        }
        while (_read_next_header_keyword(ifs, keyword, header)) {
            // do nothing 
        }

        return ifs.tellg();
    }
}


namespace watplot {
    const std::string Filterbank::FILE_FORMAT_NAME = "Sigproc Filterbank";
    void Filterbank::_load(const std::string & path) {
        std::ifstream ifs(path, std::ios::in | std::ios::binary);
        header_end = _read_header(ifs, header);
        data_size_bytes = file_size_bytes - header_end;

        // read some data to estimate mean, etc. for initializing plot
        int64_t nbytes = (header.nbits / 8);
        int64_t bufsize = int64_t(min(max(consts::MEMORY / 10, 10), int(2e8) / nbytes));
        char * buf = new char[bufsize + 1];
        int64_t total = 0;
        //while (true) {
        ifs.read(buf, bufsize);
        int64_t nread = ifs.gcount() / nbytes;
        total += nread;

        if (nbytes == 4) {
            // float
            Eigen::Map<Eigen::VectorXf> map((float*)buf, nread);
            max_val = max(double(map.maxCoeff()), max_val);
            min_val = min(double(map.minCoeff()), min_val);
            mean_val += map.sum();
        }
        else if (nbytes == 8) {
            // double
            Eigen::Map<Eigen::VectorXd> map((double*)buf, nread);
            max_val = max(map.maxCoeff(), max_val);
            min_val = min(map.minCoeff(), min_val);
            mean_val += map.sum();
        }
        else if (nbytes == 2) {
            // 16 bit int
            Eigen::Map<Eigen::Matrix<uint16_t, Eigen::Dynamic, 1>> map((uint16_t*) buf, nread);
            max_val = max(double(map.maxCoeff()), max_val);
            min_val = min(double(map.minCoeff()), min_val);
            mean_val += map.cast<double>().sum();
        }
        else if (nbytes == 1) {
            // 8 bit int
            Eigen::Map<Eigen::Matrix<uint8_t, Eigen::Dynamic, 1>> map((uint8_t*) buf, nread);
            max_val = max(double(map.maxCoeff()) * 256, max_val);
            min_val = min(double(map.minCoeff()) * 256, min_val);
            mean_val += map.cast<double>().sum() * 256;
        }
        else {
            std::cerr << "Error: Unsupported data width: " << header.nbits << " (only 8, 16, 32 bit data supported)\n";
            std::exit(4);
        }

        mean_val /= total;
    }

    void Filterbank::_view(const cv::Rect2d & rect, Eigen::MatrixXd & out, int64_t t_lo, int64_t t_hi, int64_t t_step, 
                                                                           int64_t f_lo, int64_t f_hi, int64_t f_step) const
    {
        // REMEMBER: y axis is frequency, x is time
        std::ifstream ifs(file_path, std::ios::binary | std::ios::in); 

        int64_t out_hi = (f_hi - f_lo) / f_step;
        int64_t out_wid = (t_hi - t_lo) / t_step;
        int64_t nbytes = header.nbits / 8;

        if (t_lo <= 0 && t_hi >= nints && f_lo <= 0 && f_hi >= header.nchans
            && f_step == 1 && t_step == 1 && (nbytes == 4 || nbytes == 8)) {
            // want everything in the file; fast-forward and load the entire file
            // (only support 32/64-bit)
            ifs.seekg(header_end);
            if (nbytes == 4) {
                Eigen::MatrixXf buf(out_hi, out_wid);
                ifs.read((char*)buf.data(), out_hi * out_wid * nbytes);
                for (int64_t i = out.cols() - 2; i > 0; --i) {
                    for (int64_t j = out.rows() - 2; j > 0; --j) {
                        out(j, i) = buf(j - 1, i - 1);
                    }
                }
            }
            else if (nbytes == 8) {
                ifs.read((char*)out.data(), out_hi * out_wid * nbytes);
                for (int64_t i = out.cols() - 2; i > 0; --i) {
                    for (int64_t j = out.rows() - 2; j > 0; --j) {
                        out(j, i) = out.data()[i * header.nchans + j];
                    }
                }
                out.row(0).setZero();
                out.col(0).setZero();
                out.row(out.rows() - 1).setZero();
                out.col(out.cols() - 1).setZero();
            }
        }
        else {

            // load the data into bins

            int64_t maxf = min(f_hi, header.nchans), maxt = min(t_hi, nints);

            {
                // buffer to load several columns at once
                int64_t bufsize = min(max((consts::MEMORY / 12), (maxf - f_lo) * nbytes), data_size_bytes + 1);
                std::string bufs;
                bufs.resize(bufsize + 1);
                char * buf = &bufs[0];
                int64_t last_read_pos = 0;

                // for every timestamp
                for (int64_t t = t_lo; t < maxt; ++t) {
                    double * out_data = out.data() + ((t - t_lo) / t_step + 1) * (out_hi + 2) + 1;

                    int64_t pos = header_end + (t * header.nchans + f_lo) * nbytes;
                    int64_t offset = pos - last_read_pos;
                    // try to use existing buffer if possible
                    if (!last_read_pos || offset + (maxf - f_lo) * nbytes >= bufsize) {
                        ifs.seekg(pos);
                        ifs.read(buf, bufsize);
                        last_read_pos = pos;
                        offset = 0;
                        std::cerr << "Filterbank-view: Data file " << util::round(double(t - t_lo) / maxt * 100, 2) << "% loaded\n";
                    }

                    int64_t n_f_bins = (maxf - f_lo) / f_step;
                    int64_t f_xtra_bin_size = (maxf - f_lo) % f_step;
                    switch (nbytes) {
                    case 4:
                    {
                        Eigen::Map<Eigen::VectorXd> out_mp(out_data, n_f_bins);
                        Eigen::Map<Eigen::MatrixXf> in_mp((float*)(buf + offset), f_step, n_f_bins);
                        out_mp += in_mp.colwise().sum().cast<double>();

                        if (f_xtra_bin_size) {
                            double * out_mp_xtra = out_data + n_f_bins;
                            Eigen::Map<Eigen::VectorXf> in_mp_xtra((float*)(buf + offset) + f_step * n_f_bins, f_xtra_bin_size);
                            *out_mp_xtra += in_mp_xtra.sum();
                        }
                    }
                    break;
                    case 8:
                    {
                        Eigen::Map<Eigen::VectorXd> out_mp(out_data, n_f_bins);
                        Eigen::Map<Eigen::MatrixXd> in_mp((double*)(buf + offset), f_step, n_f_bins);
                        out_mp += in_mp.colwise().sum();

                        if (f_xtra_bin_size) {
                            double * out_mp_xtra = out_data + n_f_bins;
                            Eigen::Map<Eigen::VectorXd> in_mp_xtra((double*)(buf + offset) + f_step * n_f_bins, f_xtra_bin_size);
                            *out_mp_xtra += in_mp_xtra.sum();
                        }
                    }
                    break;
                    case 2:
                    {
                        Eigen::Map<Eigen::VectorXd> out_mp(out_data, n_f_bins);
                        Eigen::Map<Eigen::Matrix<uint16_t, Eigen::Dynamic, Eigen::Dynamic>>
                            in_mp((uint16_t*)(buf + offset), f_step, n_f_bins);
                        out_mp += in_mp.colwise().sum().cast<double>();

                        if (f_xtra_bin_size) {
                            double * out_mp_xtra = out_data + n_f_bins;
                            Eigen::Map<Eigen::Matrix<uint16_t, Eigen::Dynamic, 1>> in_mp_xtra((uint16_t*)(buf + offset) + f_step * n_f_bins, f_xtra_bin_size);
                            *out_mp_xtra += static_cast<double>(in_mp_xtra.sum());
                        }

                    }
                    break;
                    case 1:
                        Eigen::Map<Eigen::VectorXd> out_mp(out_data, n_f_bins);
                        Eigen::Map<Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic>>
                            in_mp((uint8_t*)(buf + offset), f_step, n_f_bins);
                        out_mp += in_mp.colwise().sum().cast<double>() * 256;

                        if (f_xtra_bin_size) {
                            double * out_mp_xtra = out_data + n_f_bins;
                            Eigen::Map<Eigen::Matrix<uint16_t, Eigen::Dynamic, 1>> in_mp_xtra((uint16_t*)(buf + offset) + f_step * n_f_bins, f_xtra_bin_size);
                            *out_mp_xtra += static_cast<double>(in_mp_xtra.sum()) * 256;
                        }
                    }
                }
            }
        }
        std::cerr << "Filterbank-view: 100% loaded, processing data in memory...\n";
    } 
}

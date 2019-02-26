#include "stdafx.h"
#include "filterbank.hpp"
#include "util.hpp"

namespace watplot {
    const std::string Filterbank::FILE_FORMAT_NAME = "Sigproc Filterbank";
    void Filterbank::_load(const std::string & path) {
        std::ifstream ifs(path, std::ios::in | std::ios::binary);
        if (!ifs) {
            std::cerr << "File not found: " << path << "\n";
            std::exit(1);
        }
        header_end = _read_header(ifs);
        data_size_bytes = file_size_bytes - header_end;

        int64_t nbytes = (header.nbits / 8);
        int64_t bufsize = max(consts::MEMORY / 10 / nbytes, 1);
        Eigen::VectorXf buf(bufsize);
        int64_t total = 0;
        while (true) {
            ifs.read((char*)buf.data(), bufsize * nbytes);
            int64_t nread = ifs.gcount() / nbytes;
            max_val = max(buf.head(nread).maxCoeff(), max_val);
            min_val = min(buf.head(nread).minCoeff(), min_val);
            total += nread;
            mean_val += buf.head(nread).cast<double>().mean() / data_size_bytes * nread * nbytes;
            if (!ifs) break;
        }
    }

    cv::Rect2d Filterbank::_view(const cv::Rect2d & rect, Eigen::MatrixXd & out, int max_wid, int max_hi) const
    {
        // REMEMBER: y axis is frequency, x is time
        std::ifstream ifs(file_path, std::ios::in | std::ios::binary);

        // convert to array indices
        int64_t f_lo = static_cast<int64_t>(std::upper_bound(freqs.begin(), freqs.end(), rect.y) - freqs.begin()) - 1;
        f_lo = max(0LL, f_lo);
        int64_t f_hi = static_cast<int64_t>(std::upper_bound(freqs.begin(), freqs.end(), rect.y + rect.height)
                                        - freqs.begin());
        int64_t t_lo = static_cast<int64_t>(std::upper_bound(timestamps.begin(), timestamps.end(), rect.x)
                                            - timestamps.begin()) - 1;
        t_lo = max(0LL, t_lo);
        int64_t t_hi = static_cast<int64_t>(std::upper_bound(timestamps.begin(), timestamps.end(), rect.x + rect.width)
                                            - timestamps.begin());
        int64_t f_step = max((f_hi - f_lo - 1) / max_hi + 1, 1LL);
        int64_t t_step = max((t_hi - t_lo - 1) / max_wid + 1, 1LL);
        std::cerr << "Reloading data with f_step=" << f_step << " t_step=" << t_step << "\n";

        // swap if step is reversed in input data file
        if (header.foff < 0) {
            f_lo = header.nchans - f_lo;
            f_hi = header.nchans - f_hi;
            std::swap(f_lo, f_hi);
            f_lo = max(0LL, f_lo);
        }
        if (header.tsamp < 0) {
            t_lo = nints - t_lo;
            t_hi = nints - t_hi;
            std::swap(t_lo, t_hi);
            t_lo = max(0LL, t_lo);
        }


        // round to multiple of step size
        f_hi += (f_step - (f_hi - f_lo) % f_step) % f_step;
        t_hi += (t_step - (t_hi - t_lo) % t_step) % t_step;

        // load the data into bins
        int64_t out_hi = (f_hi - f_lo) / f_step;
        int64_t out_wid = (t_hi - t_lo) / t_step;
        out.resize(out_hi + 2, out_wid + 2);
        out.setZero();

        int64_t out_f_cnt;
        int64_t maxf = min(f_hi, header.nchans), maxt = min(t_hi, nints);

        {
            int64_t nbytes = (header.nbits / 8);
            // buffer to load several columns at once
            int64_t bufsize = max((consts::MEMORY / 12 / nbytes), maxf - f_lo);
            Eigen::VectorXf buf(bufsize + 1);
            int64_t last_read_pos = 0;

            // for every timestamp
            for (int64_t t = t_lo; t < maxt; ++t) {
                double * out_data = out.data() + ((t - t_lo) / t_step + 1) * (out_hi + 2) + 1;

                out_f_cnt = 0;
                int64_t pos = header_end + (t * header.nchans + f_lo) * nbytes;
                int64_t offset = (pos - last_read_pos) / nbytes;
                // try to use existing buffer if possible
                if (!last_read_pos || offset + maxf - f_lo >= bufsize) {
                    ifs.seekg(pos);
                    ifs.read((char*)buf.data(), bufsize * nbytes);
                    last_read_pos = pos;
                    offset = 0;
                }

                // for every frequency
                for (int64_t f = 0; f < maxf - f_lo; ++f) {
                    if (out_f_cnt == f_step) {
                        out_f_cnt = 0;
                        ++out_data;
                    }
                    *out_data += buf[offset + f];
                    ++out_f_cnt;
                }
            }
        }

        // reverse cols/rows if frequency/time axis is reversed
        if (header.foff < 0) {
            out.colwise().reverseInPlace();
        }
        if (header.tsamp < 0) {
            out.rowwise().reverseInPlace();
        }

        double area = double(f_step * t_step);

        // now compute prefix sum matrix from binned matrix
        // sum columns
        double * out_data = out.data() + out_hi + 2;
        for (int64_t t = 0; t <= out_wid; ++t) {
            *out_data = 0.0f;
            ++out_data;
            for (int64_t f = 0; f <= out_hi; ++f) {
                *out_data += *(out_data - 1);
                *out_data /= area;
                ++out_data;
            }
        }

        // sum column sums
        out_data = out.data() + (out_hi + 2) * 2;
        for (int64_t t = 1; t <= out_wid; ++t) {
            ++out_data;
            for (int64_t f = 0; f <= out_hi; ++f) {
                *out_data += *(out_data - out_hi - 2);
                ++out_data;
            }
        }

        // compute output rectangle
        return cv::Rect2d(
            min(header.tstart + t_lo * header.tsamp, header.tstart + t_hi * header.tsamp),
            min(header.fch1 + f_lo * header.foff, header.fch1 + f_hi * header.foff),
            (t_hi - t_lo) * fabs(header.tsamp), (f_hi - f_lo) * fabs(header.foff));
    }

    int64_t Filterbank::_read_header(std::ifstream & ifs)
    {
        std::string keyword;

        // check this is a blimpy file
        if (_read_next_header_keyword(ifs, keyword) || keyword != "HEADER_START") {
            std::cerr << "Not a BLIMPY filterbank file!\n";
            std::exit(1);
        }
        while (_read_next_header_keyword(ifs, keyword)) {
            // do nothing 
        }

        return ifs.tellg();
    }

    bool Filterbank::_read_next_header_keyword(std::ifstream & ifs, std::string & kwd)
    {
        uint32_t kwdlen;
        ifs.read((char*)& kwdlen, sizeof(kwdlen));

        kwd.resize(kwdlen);
        ifs.read(&kwd[0], kwdlen);

        char dtype = consts::HEADER_KEYWORD_TYPES.at(kwd);

        switch (dtype) {
        case '!':
            return false;
            break;
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
            data = util::double_to_angle(raw);
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
}

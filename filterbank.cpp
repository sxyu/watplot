#include "stdafx.h"
#include "filterbank.hpp"
#include "util.hpp"

namespace watmaps {
    void Filterbank::_load(const std::string & path) {
        std::ifstream ifs(path, std::ios::in | std::ios::binary);
        if (!ifs) {
            std::cerr << "File not found: " << path << "\n";
            std::exit(1);
        }
        header_end = _read_header(ifs);
        data_size_bytes = file_size_bytes - header_end;
        nints = data_size_bytes / (nbits / 8) / nchans;
        if (nbits != 32) {
            std::cerr << nbits << " bit data not supported. Currently only 32 bit is supported.\n";
            std::exit(3);
        }

        data = Eigen::MatrixXf(nchans, nints);

        // temporary lazy method
        ifs.read((char * )data.data(), data_size_bytes);
    }

    long long Filterbank::_read_header(std::ifstream & ifs)
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

        freqs.reserve(nchans);
        for (int i = 0; i < nchans; ++i) {
            freqs.push_back(fch1 + foff * i);
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
            if (kwd == "telescope_id") telescope_id = data;
            else if (kwd == "machine_id") machine_id = data;
            else if (kwd == "data_type") data_type = data;
            else if (kwd == "barycentric") barycentric = data != 0;
            else if (kwd == "pulsarcentric") pulsarcentric = data != 0;
            else if (kwd == "nbits") nbits = data;
            else if (kwd == "nsamples") nsamples = data;
            else if (kwd == "nchans") nchans = data;
            else if (kwd == "nifs") nifs = data;
            else if (kwd == "nbeams") nbeams = data;
            else ibeam = data; // ibeam
        }
            break;
        case 'd':
        {
            double data;
            ifs.read((char*)&data, sizeof(data));
            if (kwd == "az_start") az_start = data;
            else if (kwd == "za_start") za_start = data;
            else if (kwd == "tstart") tstart = data;
            else if (kwd == "tsamp") tsamp = data;
            else if (kwd == "fch1") fch1 = data;
            else if (kwd == "foff") foff = data;
            else if (kwd == "refdm") refdm = data;
            else period = data; // period
        }
            break;
        case 's':
        {
            uint32_t len;
            ifs.read((char*)& len, sizeof(len));
            std::string data;
            data.resize(len);
            ifs.read(&data[0], len);

            if (kwd == "rawdatafile") rawdatafile = data;
            else source_name = data; // source_name
        }
        break;
        case 'a':
        {
            double raw, data;
            ifs.read((char*)& raw, sizeof(raw));
            data = util::double_to_angle(raw);
            if (kwd == "src_raj") src_raj = data;
            else src_dej = data; // src_dej
        }
        break;
        default:
            std::cerr << "Unsupported header keyword: " << kwd << "\n";
            std::exit(2);
        }
        return true;
    }
}

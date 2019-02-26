#pragma once
#include<string>
#include<vector>
#include<map>
namespace watmaps {
    struct consts {
    private:
        static std::map<std::string, char> _header_keyword_types() {
                std::map<std::string, char> mp;
                mp["telescope_id"] = 'l';
                mp["machine_id"] = 'l';
                mp["data_type"] = 'l';
                mp["barycentric"] = 'l',
                mp["pulsarcentric"] = 'l',
                mp["nbits"] = 'l';
                mp["nsamples"] = 'l';
                mp["nchans"] = 'l';
                mp["nifs"] = 'l';
                mp["nbeams"] = 'l';
                mp["ibeam"] = 'l';
                mp["rawdatafile"] = 's';
                mp["source_name"] = 's';
                mp["az_start"] = 'd';
                mp["za_start"] = 'd';
                mp["tstart"] = 'd';
                mp["tsamp"] = 'd';
                mp["fch1"] = 'd';
                mp["foff"] = 'd';
                mp["refdm"] = 'd';
                mp["period"] = 'd';
                mp["src_raj"] = 'a';
                mp["src_dej"] = 'a';
                mp["HEADER_START"] = '!';
                mp["HEADER_END"] = '!';
                return mp;
        }

        static std::vector<std::string> _telescopes() {
                std::vector<std::string> vec(66);
                vec[0] = "Fake data";
                vec[1] = "Arecibo";
                vec[2] = "Ooty";
                vec[3] = "Nancay";
                vec[4] = "Parkes";
                vec[5] = "Jodrell";
                vec[6] = "GBT";
                vec[8] = "Effelsberg";
                vec[10] = "SRT";
                vec[64] = "MeerKAT";
                vec[65] = "KAT7";
                return vec;
        }

    public:
        /* Map of allowed keywords and their types */
        static const std::map<std::string, char> HEADER_KEYWORD_TYPES;
        /* List of telescopes (index i = name of telescope with ID i, e.g., 6 is GBT) */
        static const std::vector<std::string> TELESCOPES;
    };
}

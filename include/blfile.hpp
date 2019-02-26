#pragma once
#include<string>
#include<vector>

namespace watmaps {
    /* CRTP base class for all BL file formats */
    template <class ImplType>
    class BLFile {
    public:
        /* load from a path */
        void load(const std::string & path) {
            file_path = path;
            std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
            file_size_bytes = in.tellg();
            in.close();
            static_cast<ImplType *>(this)->_load(path);
        }

        /* Get the telescope name */
        const std::string & get_telescope_name() const {
            return consts::TELESCOPES[telescope_id];
        }

        // 0 = fake data; 1 = Arecibo; 2 = Ooty... others to be added
        int telescope_id = -1; 
        // 0=FAKE; 1=PSPM; 2=WAPP; 3=OOTY... others to be added
        int machine_id = -1;
        // 1=blimpy; 2=time series... others to be added
        int data_type = -1; 
        // the name of the original data file
        std::string rawdatafile; 
        // the name of the source being observed by the telescope
        std::string source_name; 
        // true if data are barycentric or 0 otherwise
        bool barycentric = 0; 
        // true if data are pulsarcentric or 0 otherwise
        bool pulsarcentric = 0; 
        // telescope azimuth at start of scan (degrees)
        double az_start = NAN;
        // telescope zenith angle at start of scan (degrees)
        double za_start = NAN; 
        // right ascension (J2000) of source (hours, converted from hhmmss.s)
        double src_raj = NAN; 
        // declination (J2000) of source (degrees, converted from ddmmss.s)
        double src_dej = NAN; 
        // time stamp (MJD) of first sample
        double tstart = NAN; 
        // time interval between samples (s)
        double tsamp = NAN; 
        // number of bits per time sample
        int nbits; 
        // number of time samples in the data file (rarely used any more)
        int nsamples = -1; 
        // number of time integrations in the data file
        long long nints = -1; 
        // centre frequency (MHz) of first blimpy channel
        double fch1 = NAN; 
        // blimpy channel bandwidth (MHz)
        double foff = NAN; 
        // number of blimpy channels
        int nchans = -1;
        // number of seperate IF channels
        int nifs = -1;
        // reference dispersion measure (pc/cm**3)
        double refdm = NAN;
        // folding period (s)
        double period = NAN;
        //total number of beams (?)
        int nbeams = -1;
        // number of the beam in this file (?)
        int ibeam = -1;

        // size of file in bytes
        long long file_size_bytes;
        // size of data in bytes
        long long data_size_bytes;

        // list of frequencies
        std::vector<double> freqs;

        // path to data file
        std::string file_path;
    };

    template<class ImplType>
    std::ostream & operator<< (std::ostream & o, const BLFile<ImplType> & file) {
        o << "---------------------------------\n";
        o << "Breakthrough Listen data file\n";
        o << "Path:\t" << file.file_path << "\n";
        o << "Data from:\t" << file.get_telescope_name() << "\n";
        o << "Source:\t\t" << file.source_name << "\n";
        o << "Raw file:\t" << file.rawdatafile << "\n";
        o << "\n";
        o << "File size:\t\t" << file.file_size_bytes << " bytes (data: " << file.data_size_bytes << ")\n";
        o << "Time axis:\t\t" << file.nints << " integrations\n";
        o << "Frequency axis:\t\t" << file.nchans << " channels,\n Range:\t\t\t[" <<
            file.freqs.front() << ", " << file.freqs.back() << "]\n";
        o << "Bits per sample:\t" << file.nbits << "\n";
        o << "---------------------------------\n";
        return o;
    }
}

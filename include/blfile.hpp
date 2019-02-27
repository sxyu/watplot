#pragma once
#include<string>
#include<vector>

namespace watplot {
    /** CRTP base class for all Breakthrough Listen data file formats
     *  Note: do not actually instantiate this class
     *  Child classes must implement: _load, _view, _file_format */
    template <class ImplType>
    class BLFile {
    public:
        /** Read data file from a path (reads header, 'skims' data without loading it) */
        void load(const std::string & path) {
            file_path = path;
            std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
            file_size_bytes = in.tellg();
            in.close();

            static_cast<ImplType *>(this)->_load(path);

            // precompute axes, rectangle
            nints = data_size_bytes / (header.nbits / 8) / header.nchans;
            data_rect.y = min(header.fch1, header.fch1 + header.foff * header.nchans);
            data_rect.x = min(header.tstart, header.tstart + header.tsamp * nints);
            data_rect.height = fabs(header.foff) * header.nchans;
            data_rect.width = fabs(header.tsamp) * nints;

            freqs.reserve(header.nchans);
            for (int i = 0; i < header.nchans; ++i) {
                freqs.push_back(header.fch1 + header.foff * i);
            }

            timestamps.reserve(nints);
            for (int i = 0; i < nints; ++i) {
                timestamps.push_back(header.tstart + header.tsamp * i);
            }

            if (header.foff < 0) std::reverse(freqs.begin(), freqs.end());
            if (header.tsamp < 0) std::reverse(timestamps.begin(), timestamps.end());
        }

        /** Load rectangle of data into memory as a 2D prefix-sum matrix
         * @param rect rectangle of interest
         *  Important: axis order is transpose of waterfall diagram: x is time, y is frequency
         *  This is due to Eigen's default column major storage order 
         *  Also note: lower bounds should be inclusive, upper should be exclusive
         * @param[in] max_wid max pixels returned width-wise
         * @param[in] max_hi max pixels returned height-wise
         * @param[out] out prefix-sum matrix. Sum in rectangle (x, y, w, h) may be computed as
         *                 (M[x+w,y+h] - M[x,y+h] - M[x+w,y] + M[x,y])/(wh)
         *                 padded with one row, one column on each side.
         * @return actual rectangle returned. May be rounded.
         */
        cv::Rect2d view(const cv::Rect2d & rect, Eigen::MatrixXd & out, int max_wid, int max_hi) const {
            return static_cast<const ImplType *>(this)->_view(rect, out, max_wid, max_hi);
        }

        /* Get the file format name (e.g. sigproc filterbank) */
        const std::string & get_file_format() const {
            return ImplType::FILE_FORMAT_NAME;
        }

        /* Get the name of the telescope used to collect this data */
        const std::string & get_telescope_name() const {
            return consts::TELESCOPES[header.telescope_id];
        }

        // Sigproc filterbank header fields; not filled fields are: -1 for ints, empty strings, NAN floats
        struct Header {
            // telescope: 0 = fake data; 1 = Arecibo; 2 = Ooty... others to be added
            int telescope_id = -1;
            // machine: 0=FAKE; 1=PSPM; 2=WAPP; 3=OOTY... others to be added
            int machine_id = -1;
            // data type: 1=blimpy; 2=time series... others to be added
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
            int nbits = -1;
            // number of time samples in the data file (rarely used any more)
            int nsamples = -1;
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
        };

        /** the data file header */
        Header header;

        /** get the rectangle (y frequency, x time) containing all samples */
        inline const cv::Rect2d & get_full_rect() const {
            return data_rect;
        }

        /** list of frequencies (y-axis on view) */
        std::vector<double> freqs;

        /** list of timestamps (x-axis on view) */
        std::vector<double> timestamps;

        // size of file in bytes
        int64_t file_size_bytes;

        // size of data in bytes (file_size_bytes - size of header)
        int64_t data_size_bytes;

        /* number of time integrations in the data file (overrides header nsamples) */
        int64_t nints;

        /* path to data file */
        std::string file_path;

        // statistics
        /* minimum sample value */
        double min_val = DBL_MAX;

        /* maximum sample value */
        double max_val = -DBL_MAX;

        /* mean sample value */
        double mean_val = 0.0;
    protected:
        /* rectangle containing all data */
        cv::Rect2d data_rect;
    };

    // printing the file instance
    template<class ImplType>
    std::ostream & operator<< (std::ostream & o, const BLFile<ImplType> & file) {
        o << "---------------------------------\n";
        o << "Breakthrough Listen data file\n";
        o << "File:\t\t" << file.file_path << "\n";
        o << "Format:\t\t" << file.get_file_format() << "\n";
        o << "Telescope:\t" << file.get_telescope_name() << "\n";
        o << "Source:\t\t" << file.header.source_name << "\n";
        o << "Raw file:\t" << file.header.rawdatafile << "\n";
        o << "\nData Format\n";
        o << "Time axis:\t\t" << file.nints << " timestamps\n Range:\t\t\t[" <<
            file.timestamps.front() << ", " << file.timestamps.back() << "]\n";
        o << "Frequency axis:\t\t" << file.header.nchans << " channels\n Range:\t\t\t[" <<
            file.freqs.front() << ", " << file.freqs.back() << "]\n";
        o << "Bits per sample:\t" << file.header.nbits << "\n\nStatistics\n";
        o << "File size:\t\t" << file.file_size_bytes << " bytes (data: " << file.data_size_bytes << ")\n";
        o << "Approx min, mean, max:\t" << file.min_val << ", " << file.mean_val << ", " << file.max_val << "\n";
        o << "---------------------------------\n";
        return o;
    }
}

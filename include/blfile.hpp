#pragma once
#include<string>
#include<vector>

namespace watplot {
    /** Base class for all Breakthrough Listen data file formats
     *  Note: do not actually instantiate this class
     *  Child classes must implement: _load, _view, _file_format */
    template <class ImplType>
    class BLFile {
    public:
        /** Threshold (GB) above which file is considered large */
        static const int LARGE_FILE_THRESH = 5;

        /** Read data file from a path (reads header, 'skims' data without loading it) */
        void load(const std::string & path) {
            file_path = path;
            std::ifstream in(path, std::ifstream::ate | std::ifstream::binary);
            if (!in) {
                std::cerr << "Fatal error: File not found (" << path << ")\n";
                std::exit(1);
            }
            file_size_bytes = in.tellg();

            if (file_size_bytes > LARGE_FILE_THRESH * 1e9) {
                std::cerr << "WARNING: file exceeds " << LARGE_FILE_THRESH << "GB; may take a while to load.\n";
            }

            in.close();

            static_cast<ImplType *>(this)->_load(path);

            // precompute axes, rectangle
            if (~header.nchans) {
                if (~nints) nints = data_size_bytes / (header.nbits / 8) / header.nchans;
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

            std::cerr << *this;
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
            std::cerr << "BLFile-view: Reloading data file, with t_step=" << t_step << " f_step=" << f_step << " t=[" <<
                t_lo << ", " << t_hi << "] f=[" << f_lo << ", " << f_hi << "]\nBLFile-view: Allocating memory...\n";

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
            }

            // round to multiple of step size
            f_hi += (f_step - (f_hi - f_lo) % f_step) % f_step;
            t_hi += (t_step - (t_hi - t_lo) % t_step) % t_step;


            int64_t out_hi = (f_hi - f_lo) / f_step;
            int64_t out_wid = (t_hi - t_lo) / t_step;

            // allocate memory
            out.resize(out_hi + 2, out_wid + 2);

            // call viewer implementation 
            static_cast<const ImplType *>(this)->_view(rect, out, t_lo, t_hi, t_step, f_lo, f_hi, f_step);

            // reverse cols/rows if frequency/time axis is reversed
            if (header.foff < 0) {
                out.colwise().reverseInPlace();
            }
            if (header.tsamp < 0) {
                out.rowwise().reverseInPlace();
            }

            // now compute prefix sum matrix from binned matrix
            // sum columns
            double * out_data = out.data() + out_hi + 2;
            for (int64_t t = 0; t <= out_wid; ++t) {
                std::partial_sum(out_data + t * (out_hi + 2),
                    out_data + (t + 1) * (out_hi + 2),
                    out_data + t * (out_hi + 2));
            }

            double area = double(f_step * t_step);
            out /= area;

            // sum column sums
            out_data = out.data() + (out_hi + 2) * 2;
            for (int64_t t = 1; t <= out_wid; ++t) {
                ++out_data;
                for (int64_t f = 0; f <= out_hi; ++f) {
                    *out_data += *(out_data - out_hi - 2);
                    ++out_data;
                }
            }

            return cv::Rect2d(
                min(header.tstart + t_lo * header.tsamp, header.tstart + t_hi * header.tsamp),
                min(header.fch1 + f_lo * header.foff, header.fch1 + f_hi * header.foff),
                (t_hi - t_lo) * fabs(header.tsamp), (f_hi - f_lo) * fabs(header.foff));
        }

        /* Get the file format name (e.g. sigproc filterbank) */
        const std::string & get_file_format() const {
            return ImplType::FILE_FORMAT_NAME;
        }

        /* Get the name of the telescope used to collect this data */
        const std::string & get_telescope_name() const {
            return consts::TELESCOPES[header.telescope_id];
        }

        /** Sigproc filterbank header fields; not filled fields are:
          * 0 for telescope/machine id, -1 for other ints, empty strings, NAN floats */
        struct Header {
            // telescope: 0 = fake data; 1 = Arecibo; 2 = Ooty... others to be added
            int telescope_id = 0;
            // machine: 0=FAKE; 1=PSPM; 2=WAPP; 3=OOTY... others to be added
            int machine_id = 0;
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

        /** basic constructor, checks if a file exists and if so loads from it */
        BLFile(const std::string & path) { load(path); }

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
        if (!file.timestamps.empty()) {
            o << "Time axis:\t\t" << file.nints << " timestamps\n Range:\t\t\t[" <<
                file.timestamps.front() << ", " << file.timestamps.back() << "]\n";
        }
        if (!file.freqs.empty()) {
            o << "Frequency axis:\t\t" << file.header.nchans << " channels\n Range:\t\t\t[" <<
                file.freqs.front() << ", " << file.freqs.back() << "]\n";
        }
        o << "Bits per sample:\t" << file.header.nbits << "\n\nStatistics\n";
        o << "File size:\t\t" << file.file_size_bytes << " bytes (data: " << file.data_size_bytes << ")\n";
        o << "Approx min, mean, max:\t" << file.min_val << ", " << file.mean_val << ", " << file.max_val << "\n";
        o << "---------------------------------\n";
        return o;
    }
}

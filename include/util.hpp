#pragma once
#include<string>
#include<vector>
#include<utility>
#include<random>
namespace watmaps {
    // helpers
    namespace util {
        std::string random_string(std::string::size_type length);

        template<class T>
        /** mersenne twister PRNG */
        inline T randint_mt(T lo, T hi) {
            static std::random_device rd{};
            // some compilers (e.g. mingw g++) have bad random device implementation, must use time(NULL)
            static std::mt19937 rg(rd.entropy() > 1e-9 ? rd() : static_cast<unsigned>(time(NULL)));
            std::uniform_int_distribution<T> pick(lo, hi);
            return pick(rg);
        }

        template<class T>
        /** xorshift-based PRNG */
        inline T randint(T lo, T hi) {
            if (hi <= lo) return lo;
            static std::random_device rd{};
            // some compilers (e.g. mingw g++) have bad random device implementation, must use PRNG
            static unsigned long x = (rd.entropy() > 1e-9) ? rd() : randint_mt(0, INT_MAX),
                                 y = (rd.entropy() > 1e-9) ? rd() : randint_mt(0, INT_MAX),
                                 z = (rd.entropy() > 1e-9) ? rd() : randint_mt(0, INT_MAX);
            unsigned long t;
            x ^= x << 16;
            x ^= x >> 5;
            x ^= x << 1;
            t = x;
            x = y;
            y = z;
            z = t ^ x ^ y;
            return z % (hi - lo + 1) + lo;
        }

        template<class T>
        inline std::pair<T, T> randpair(T lo, T hi) {
            std::pair<T, T> e(randint(lo, hi), randint(lo, hi - 1));
            if (e.second >= e.first) ++e.second;
            else std::swap(e.first, e.second);
            return e;
        }

        template<class T>
        void choose(std::vector<T> & source, int k, std::vector<T> & out, bool sort = false) {
            for (int j = 0; j < k; ++j) {
                int r = randint(j, static_cast<int>(source.size()) - 1);
                out.push_back(source[r]);
                std::swap(source[j], source[r]);
            }
            if (sort) std::sort(out.begin(), out.end());
        }

        template<class T>
        void shuffle(std::vector<T> & v) {
            //std::mt19937 rg{ std::random_device{}() };
            int last = static_cast<int>(v.size()) - 1;
            for (int j = 0; j < static_cast<int>(v.size()); ++j) {
                //std::uniform_int_distribution<int> pick(j, last);
                std::swap(v[j], v[randint(j, last)]);
            }
        }

        std::string padleft(int x, int digs, char chr = ' ');

        // from OpenARK utils
        /**
        * Splits a string into components based on a delimiter
        * @param string_in string to split
        * @param delimiters c_str of delimiters to split at
        * @param ignore_empty if true, ignores empty strings
        * @param trim if true, trims whitespaces from each string after splitting
        * @return vector of string components
        */
        std::vector<std::string> split(const std::string & string_in,
            char const * delimiters = " ", bool ignore_empty = false, bool trim = false);

        /**
        * Splits a string into components based on a delimiter
        * @param string_in string to split
        * @param delimiters c_str of delimiters to split at
        * @param ignore_empty if true, ignores empty strings
        * @param trim if true, trims whitespaces from each string after splitting
        * @return vector of string components
        */
        std::vector<std::string> split(const char * string_in, char const * delimiters = " ",
            bool ignore_empty = false, bool trim = false);

        /** Trims whitespaces (space, newline, etc.) in-place from the left end of the string */
        void ltrim(std::string & s, const std::string & chrs = "\t \n\r");

        /** Trims whitespaces (space, newline, etc.) in-place from the right end of the string */
        void rtrim(std::string & s, const std::string & chrs = "\t \n\r");

        /** Trims whitespaces (space, newline, etc.) in-place from both ends of the string */
        void trim(std::string & s, const std::string & chrs = "\t \n\r");

        /** Convert a string to upper case in-place */
        void upper(std::string & s);

        /** Convert a string to lower case in-place */
        void lower(std::string & s);

        typedef std::pair<double, double> point_t;
        typedef std::pair<point_t, point_t> points_t;

        /* find squared norm */
        double sqr_norm(const point_t& a);

        /* find squared distance */
        double sqr_dist(const point_t& a, const point_t& b);

        /* find distance between closest pair in set of 2D points 'points' O(nlogn) */
        double closest_pair_dist(const std::vector<point_t> & points);

        /* parallel for. divides interval [l, r) to subintervals and passes them to threads */
        void par_for(std::function<void(int, int)> f, int l, int r, int num_threads = -1);

        /* parallel foreach. each worker thread consumes from [l, r) whenever available */
        void par_foreach(std::function<void(int)> f, int l, int r, std::mutex * mtx = NULL, int num_threads = -1);

        /* find GCD */
        template<class T>
        inline T gcd(T a, T b)
        {
            while (a&&b) a > b ? a %= b : b %= a;
            return a + b;
        }

        /* finds a partition of 'x' with lowest possible largest partition size.
         * such that each segment has at least 1 and at most s points; O(|x| log(x[-1] - x[0]))
         * assumes: |x| >= k
         * returns: min max partition size
         * outputs: optionally, index array for an optimal solution into 'out' (out[i] = k => item i in partition k)
         */
        double min_max_k_partition(std::vector<std::pair<double, int> > & x, int k, int s,
            Eigen::ArrayXi * out = NULL);


        /** Reads a little-endian double in ddmmss.s (or hhmmss.s) format and then
         *  converts to Float degrees (or hours).  This is primarily used to read
         *  src_raj and src_dej header values. (from Blimpy) */
        double double_to_angle(double angle);


        /** Applies viridis color map from matplotlib
          * @param reverse if true, color map is reversed
          **/
        void applyColorMapViridis(const cv::Mat& gray, cv::Mat& color, bool reverse=false);
   }
}

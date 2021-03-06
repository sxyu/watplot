#include "stdafx.h"
#include "hdf5.hpp"
#include "util.hpp"

namespace {
    void _read_header(H5::DataSet & ds, watplot::HDF5::Header & header) {
        using namespace H5;
        using namespace watplot;

        // parse all header attributes
        for (int i = 1; i < ds.getNumAttrs(); ++i) {
            Attribute attr = ds.openAttribute(i);
            const std::string & kwd = attr.getName();
            char dtype = consts::HEADER_KEYWORD_TYPES.at(kwd);
            IntType inttype = attr.getIntType();
            FloatType flttype = attr.getFloatType();
            StrType strtype = attr.getStrType();

            H5std_string order_string;
            H5T_order_t order = inttype.getOrder(order_string);

            switch (dtype) {
            case 'l':
            {
                int data;
                attr.read(inttype, &data);
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
                attr.read(flttype, &data);
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
                std::string data;
                attr.read(flttype, data);

                if (kwd == "rawdatafile") header.rawdatafile = data;
                else header.source_name = data; // source_name
            }
            break;
            case 'a':
            {
                double raw, data;
                attr.read(flttype, (char*)& raw);
                data = util::double_to_angle(raw);
                if (kwd == "src_raj") header.src_raj = data;
                else header.src_dej = data; // src_dej
            }
            break;
            default:
                std::cerr << "Unsupported header keyword: " << kwd << "\n";
                std::exit(2);
            }

        }
    }

}

namespace watplot {
    const std::string HDF5::FILE_FORMAT_NAME = "HDF5";
    const std::string HDF5::DATASET_SUBSET_NAME = "data";

    void HDF5::_load(const std::string & path) {
        H5::H5File file = H5::H5File(path, H5F_ACC_RDONLY);
        H5::DataSet dataset = file.openDataSet(DATASET_SUBSET_NAME);
        data_size_bytes = dataset.getStorageSize();
        _read_header(dataset, header);

        H5::DataSpace dataspace = dataset.getSpace();
        hsize_t dims_out[3];
        dataspace.getSimpleExtentDims(dims_out, NULL);
        nints = dims_out[0];

        if (header.nbits != 8 && header.nbits != 16 && header.nbits != 32 && header.nbits != 64) {
            std::cerr << "Error: Unsupported data width: " << header.nbits << " (only 8, 16, 32 bit data supported)\n";
            std::exit(4);
        }
    }

    void HDF5::_view(const cv::Rect2d & rect, Eigen::MatrixXd & out, int64_t t_lo, int64_t t_hi, int64_t t_step,
                                                                     int64_t f_lo, int64_t f_hi, int64_t f_step) const {
        // load the data int64_to bins
        H5::H5File file = H5::H5File(file_path, H5F_ACC_RDONLY);
        H5::DataSet dataset = file.openDataSet(DATASET_SUBSET_NAME);

        int64_t out_hi = (f_hi - f_lo) / f_step;
        //int64_t out_wid = (t_hi - t_lo) / t_step;

        int64_t maxf = min(f_hi, header.nchans), maxt = min(t_hi, nints);

        int64_t nbytes = (header.nbits / 8);
        H5::DataSpace dataspace = dataset.getSpace();

        hsize_t offset[3] = { hsize_t(t_lo), 0, hsize_t(f_lo) };
        hsize_t stride[3] = { hsize_t(t_step), 1, hsize_t(f_step) };
        hsize_t count[3] = { hsize_t(maxt - t_lo) / stride[0], 1, hsize_t(maxf - f_lo) / stride[2] };
        dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride);

        hsize_t mem_count[2] = { count[0], count[2] };
        H5::DataSpace memspace(2, mem_count);

        std::cerr << "HDF5-view: Reading data...\n";
        if (nbytes == 4) {
            Eigen::MatrixXf buf(count[2], count[0]);
            dataset.read(buf.data(), H5::PredType::NATIVE_FLOAT, memspace, dataspace);
            std::cerr << "HDF5-view: Copying from buffer...\n";
            for (int64_t i = out.cols() - 2; i > 0; --i) {
                for (int64_t j = out.rows() - 2; j > 0; --j) {
                    out(j, i) = static_cast<double>(buf(j - 1, i - 1));
                }
            }
        }
        else if (nbytes == 8) {
            Eigen::MatrixXd buf(count[2], count[0]);
            dataset.read(buf.data(), H5::PredType::NATIVE_DOUBLE, memspace, dataspace);
            std::cerr << "HDF5-view: Copying from buffer...\n";
            for (int64_t i = out.cols() - 2; i > 0; --i) {
                for (int64_t j = out.rows() - 2; j > 0; --j) {
                    out(j, i) = buf(j - 1, i - 1);
                }
            }
        }
        else if (nbytes == 2) {
            Eigen::Matrix<uint16_t, Eigen::Dynamic, 1> buf(count[2], count[0]);
            dataset.read(buf.data(), H5::PredType::NATIVE_UINT16, memspace, dataspace);
            std::cerr << "HDF5-view: Copying from buffer...\n";
            for (int64_t i = out.cols() - 2; i > 0; --i) {
                for (int64_t j = out.rows() - 2; j > 0; --j) {
                    out(j, i) = static_cast<double>(buf(j - 1, i - 1));
                }
            }
        }
        else if (nbytes == 1) {
            Eigen::Matrix<uint16_t, Eigen::Dynamic, 1>  buf(count[2], count[0]);
            dataset.read(buf.data(), H5::PredType::NATIVE_UINT8, memspace, dataspace);
            std::cerr << "HDF5-view: Copying from buffer...\n";
            for (int64_t i = out.cols() - 2; i > 0; --i) {
                for (int64_t j = out.rows() - 2; j > 0; --j) {
                    out(j, i) = static_cast<double>(buf(j - 1, i - 1));
                }
            }
        }

        out *= static_cast<double>(t_step * f_step);
        out.row(0).setZero();
        out.col(0).setZero();
        out.row(out.rows() - 1).setZero();
        out.col(out.cols() - 1).setZero();

        std::cerr << "HDF5-view: 100% loaded, processing data in memory...\n";
    }
}

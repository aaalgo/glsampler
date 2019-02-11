#include <string>
#include <vector>
#include <glog/logging.h>
#include <boost/python.hpp>
#include <boost/python/numpy.hpp>
#include "glsampler.h"

namespace py = boost::python;
namespace np = boost::python::numpy;

namespace {

    template <typename T=float>
    void check_dense (np::ndarray array, int nd = 0) {
        CHECK(array.get_dtype() == np::dtype::get_builtin<T>());
        if (nd > 0) CHECK(array.get_nd() == nd);
        else nd = array.get_nd();
        int stride = sizeof(T);
        for (int i = 0, off=nd-1; i < nd; ++i, --off) {
            CHECK(array.strides(off) == stride);
            stride *= array.shape(off);
        }
    }

    class Sampler {
        glsampler::Sampler *sampler;
    public:
        Sampler (bool hide): sampler(glsampler::Sampler::get(true)) {
        }

        void load (np::ndarray array) {
            check_dense<uint8_t>(array, 3);
            int constexpr sz = glsampler::Sampler::VOLUME_SIZE;
            CHECK(array.shape(0) == sz);
            CHECK(array.shape(1) == sz);
            CHECK(array.shape(2) == sz);
            sampler->load(reinterpret_cast<uint8_t *>(array.get_data()));
        }

        np::ndarray sample (float x, float y, float z, float phi, float theta, float kappa, float scale) {
            int constexpr sz = glsampler::Sampler::CUBE_SIZE;

            np::ndarray array(np::zeros(py::make_tuple(sz,sz,sz), np::dtype::get_builtin<uint8_t>()));
            sampler->sample(x, y, z, phi, theta, kappa, scale, reinterpret_cast<uint8_t *>(array.get_data()));
            return array;
        }
    };

    void cleanup () {
        glsampler::Sampler::cleanup();
    }
}

BOOST_PYTHON_MODULE(glsampler)
{
    np::initialize();
    py::class_<Sampler, boost::noncopyable>("Sampler", py::init<bool>())
        .def("load", &Sampler::load)
        .def("sample", &Sampler::sample)
        ;
    py::def("cleanup", &::cleanup);
}


#ifndef GTENSOR_FFT_CUDA_H
#define GTENSOR_FFT_CUDA_H

#include <numeric>
#include <stdexcept>
#include <vector>

// Note: this file is included by fft/hip.h after redef/type aliasing
// all the necessary types and functions.
#ifdef GTENSOR_DEVICE_CUDA
#include <cufft.h>
#endif

// ======================================================================
// error handling helper

#define gtFFTCheck(what)                                                       \
  {                                                                            \
    gtFFTCheckImpl(what, __FILE__, __LINE__);                                  \
  }

inline void gtFFTCheckImpl(cufftResult_t code, const char* file, int line)
{
  if (code != CUFFT_SUCCESS) {
    fprintf(stderr, "gtFFTCheck: status %d at %s:%d\n", code, file, line);
    abort();
  }
}

namespace gt
{

namespace fft
{

namespace detail
{

template <gt::fft::Domain D, typename R>
struct fft_config;

template <>
struct fft_config<gt::fft::Domain::COMPLEX, double>
{
  constexpr static cufftType type_forward = CUFFT_Z2Z;
  constexpr static cufftType type_inverse = CUFFT_Z2Z;
  constexpr static auto exec_fn_forward = &cufftExecZ2Z;
  constexpr static auto exec_fn_inverse = &cufftExecZ2Z;
  using Tin = gt::complex<double>;
  using Tout = gt::complex<double>;
  using Bin = cufftDoubleComplex;
  using Bout = cufftDoubleComplex;
};

template <>
struct fft_config<gt::fft::Domain::COMPLEX, float>
{
  constexpr static cufftType type_forward = CUFFT_C2C;
  constexpr static cufftType type_inverse = CUFFT_C2C;
  constexpr static auto exec_fn_forward = &cufftExecC2C;
  constexpr static auto exec_fn_inverse = &cufftExecC2C;
  using Tin = gt::complex<float>;
  using Tout = gt::complex<float>;
  using Bin = cufftComplex;
  using Bout = cufftComplex;
};

template <>
struct fft_config<gt::fft::Domain::REAL, double>
{
  constexpr static cufftType type_forward = CUFFT_D2Z;
  constexpr static cufftType type_inverse = CUFFT_Z2D;
  constexpr static auto exec_fn_forward = &cufftExecD2Z;
  constexpr static auto exec_fn_inverse = &cufftExecZ2D;
  using Tin = double;
  using Tout = gt::complex<double>;
  using Bin = cufftDoubleReal;
  using Bout = cufftDoubleComplex;
};

template <>
struct fft_config<gt::fft::Domain::REAL, float>
{
  constexpr static cufftType type_forward = CUFFT_R2C;
  constexpr static cufftType type_inverse = CUFFT_C2R;
  constexpr static auto exec_fn_forward = &cufftExecR2C;
  constexpr static auto exec_fn_inverse = &cufftExecC2R;
  using Tin = float;
  using Tout = gt::complex<float>;
  using Bin = cufftReal;
  using Bout = cufftComplex;
};

} // namespace detail

template <gt::fft::Domain D, typename R>
class FFTPlanManyCUDA;

template <typename R>
class FFTPlanManyCUDA<gt::fft::Domain::REAL, R>
{
  constexpr static gt::fft::Domain D = gt::fft::Domain::REAL;

public:
  FFTPlanManyCUDA(std::vector<int> real_lengths, int batch_size = 1)
    : is_valid_(true)
  {
    int rank = real_lengths.size();
    int idist = std::accumulate(real_lengths.begin(), real_lengths.end(), 1,
                                std::multiplies<int>());
    int odist =
      idist / real_lengths[rank - 1] * (real_lengths[rank - 1] / 2 + 1);
    init(real_lengths, 1, idist, 1, odist, batch_size);
  }

  FFTPlanManyCUDA(std::vector<int> real_lengths, int istride, int idist,
                  int ostride, int odist, int batch_size = 1)
    : is_valid_(true)
  {
    init(real_lengths, istride, idist, ostride, odist, batch_size);
  }

  // move only
  // delete copy ctor/assign
  FFTPlanManyCUDA(const FFTPlanManyCUDA& other) = delete;
  FFTPlanManyCUDA& operator=(const FFTPlanManyCUDA& other) = delete;

  // custom move to avoid double destroy in moved-from object
  FFTPlanManyCUDA(FFTPlanManyCUDA&& other) : is_valid_(true)
  {
    plan_forward_ = other.plan_forward_;
    plan_inverse_ = other.plan_inverse_;
    other.is_valid_ = false;
  }

  FFTPlanManyCUDA& operator=(FFTPlanManyCUDA&& other)
  {
    plan_forward_ = other.plan_forward_;
    plan_inverse_ = other.plan_inverse_;
    other.is_valid_ = false;
    return *this;
  }

  virtual ~FFTPlanManyCUDA()
  {
    if (is_valid_) {
      cufftDestroy(plan_forward_);
      cufftDestroy(plan_inverse_);
    }
  }

  void operator()(typename detail::fft_config<D, R>::Tin* indata,
                  typename detail::fft_config<D, R>::Tout* outdata) const
  {
    if (!is_valid_) {
      throw std::runtime_error("can't use a moved-from plan");
    }
    using Bin = typename detail::fft_config<D, R>::Bin;
    using Bout = typename detail::fft_config<D, R>::Bout;
    auto bin = reinterpret_cast<Bin*>(indata);
    auto bout = reinterpret_cast<Bout*>(outdata);
    auto fn = detail::fft_config<D, R>::exec_fn_forward;
    gtFFTCheck(fn(plan_forward_, bin, bout));
  }

  void inverse(typename detail::fft_config<D, R>::Tout* indata,
               typename detail::fft_config<D, R>::Tin* outdata) const
  {
    if (!is_valid_) {
      throw std::runtime_error("can't use a moved-from plan");
    }
    using Bin = typename detail::fft_config<D, R>::Bin;
    using Bout = typename detail::fft_config<D, R>::Bout;
    auto bin = reinterpret_cast<Bout*>(indata);
    auto bout = reinterpret_cast<Bin*>(outdata);
    auto fn = detail::fft_config<D, R>::exec_fn_inverse;
    gtFFTCheck(fn(plan_inverse_, bin, bout));
  }

private:
  void init(std::vector<int> real_lengths, int istride, int idist, int ostride,
            int odist, int batch_size)
  {
    int rank = real_lengths.size();
    int* nreal = real_lengths.data();

    std::vector<int> complex_lengths = real_lengths;
    complex_lengths[rank - 1] = real_lengths[rank - 1] / 2 + 1;
    int* ncomplex = complex_lengths.data();

    auto type_forward = detail::fft_config<D, R>::type_forward;
    auto type_inverse = detail::fft_config<D, R>::type_inverse;
    gtFFTCheck(cufftPlanMany(&plan_forward_, rank, nreal, nreal, istride, idist,
                             ncomplex, ostride, odist, type_forward,
                             batch_size));
    gtFFTCheck(cufftPlanMany(&plan_inverse_, rank, nreal, ncomplex, ostride,
                             odist, nreal, istride, idist, type_inverse,
                             batch_size));
  }

  cufftHandle plan_forward_;
  cufftHandle plan_inverse_;
  bool is_valid_;
};

template <typename R>
class FFTPlanManyCUDA<gt::fft::Domain::COMPLEX, R>
{
  constexpr static gt::fft::Domain D = gt::fft::Domain::COMPLEX;

public:
  FFTPlanManyCUDA(std::vector<int> lengths, int batch_size = 1)
    : is_valid_(true)
  {
    int dist = std::accumulate(lengths.begin(), lengths.end(), 1,
                               std::multiplies<int>());
    init(lengths, 1, dist, 1, dist, batch_size);
  }

  FFTPlanManyCUDA(std::vector<int> lengths, int istride, int idist, int ostride,
                  int odist, int batch_size = 1)
    : is_valid_(true)
  {
    init(lengths, istride, idist, ostride, odist, batch_size);
  }

  // move only
  // delete copy ctor/assign
  FFTPlanManyCUDA(const FFTPlanManyCUDA& other) = delete;
  FFTPlanManyCUDA& operator=(const FFTPlanManyCUDA& other) = delete;

  // custom move to avoid double destroy in moved-from object
  FFTPlanManyCUDA(FFTPlanManyCUDA&& other) : is_valid_(true)
  {
    plan_ = other.plan_;
    other.is_valid_ = false;
  }

  FFTPlanManyCUDA& operator=(FFTPlanManyCUDA&& other)
  {
    plan_ = other.plan_;
    other.is_valid_ = false;
    return *this;
  }

  virtual ~FFTPlanManyCUDA()
  {
    if (is_valid_) {
      cufftDestroy(plan_);
    }
  }

  void operator()(typename detail::fft_config<D, R>::Tin* indata,
                  typename detail::fft_config<D, R>::Tout* outdata) const
  {
    if (!is_valid_) {
      throw std::runtime_error("can't use a moved-from plan");
    }
    using Bin = typename detail::fft_config<D, R>::Bin;
    using Bout = typename detail::fft_config<D, R>::Bout;
    auto bin = reinterpret_cast<Bin*>(indata);
    auto bout = reinterpret_cast<Bout*>(outdata);
    auto fn = detail::fft_config<D, R>::exec_fn_forward;
    gtFFTCheck(fn(plan_, bin, bout, CUFFT_FORWARD));
  }

  void inverse(typename detail::fft_config<D, R>::Tout* indata,
               typename detail::fft_config<D, R>::Tin* outdata) const
  {
    if (!is_valid_) {
      throw std::runtime_error("can't use a moved-from plan");
    }
    using Bin = typename detail::fft_config<D, R>::Bin;
    using Bout = typename detail::fft_config<D, R>::Bout;
    auto bin = reinterpret_cast<Bout*>(indata);
    auto bout = reinterpret_cast<Bin*>(outdata);
    auto fn = detail::fft_config<D, R>::exec_fn_inverse;
    gtFFTCheck(fn(plan_, bin, bout, CUFFT_INVERSE));
  }

private:
  void init(std::vector<int> lengths, int istride, int idist, int ostride,
            int odist, int batch_size)
  {
    int* n = lengths.data();
    auto type_forward = detail::fft_config<D, R>::type_forward;
    gtFFTCheck(cufftPlanMany(&plan_, lengths.size(), n, n, istride, idist, n,
                             ostride, odist, type_forward, batch_size));
  }

  cufftHandle plan_;
  bool is_valid_;
};

template <gt::fft::Domain D, typename R>
using FFTPlanManyBackend = FFTPlanManyCUDA<D, R>;

} // namespace fft

} // namespace gt

#endif // GTENSOR_FFT_CUDA_H

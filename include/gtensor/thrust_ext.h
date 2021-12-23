
// ======================================================================
// thrust_ext.h
//
// adds support for writing simple expresion like a + b, where a and b
// are thrust::device_reference<thrust::complex<T>>
//
// Copyright (C) 2019 Kai Germaschewski

#ifndef GTENSOR_THRUST_EXT_H
#define GTENSOR_THRUST_EXT_H

#include "device_runtime.h"

#include "gtl.h"
#include "macros.h"

#include <thrust/device_allocator.h>
#include <thrust/device_ptr.h>
#include <thrust/system_error.h>

namespace thrust // need to put it here for ADL to work
{
namespace ext
{

// ======================================================================
// is_device_reference

template <typename T>
struct is_device_reference : std::false_type
{};

template <typename T>
struct is_device_reference<thrust::device_reference<T>> : std::true_type
{};

// ======================================================================
// has_device_reference

template <typename... Args>
using has_device_reference = gt::disjunction<is_device_reference<Args>...>;

// ======================================================================
// remove_device_reference_t

namespace detail
{
template <typename T>
struct remove_device_reference
{
  using type = T;
};

template <typename T>
struct remove_device_reference<thrust::device_reference<T>>
{
  using type = T;
};

} // namespace detail

template <typename T>
using remove_device_reference_t =
  typename detail::remove_device_reference<T>::type;

// ======================================================================
// managed allocator

#ifdef GTENSOR_DEVICE_CUDA

#include <thrust/system/cuda/error.h>

template <class T>
class managed_allocator : public ::thrust::device_allocator<T>
{
public:
  using value_type = T;

  typedef thrust::device_ptr<T> pointer;
  inline pointer allocate(size_t n)
  {
    value_type* result = nullptr;

    cudaError_t error =
      cudaMallocManaged(&result, n * sizeof(T), cudaMemAttachGlobal);

    if (error != cudaSuccess) {
      throw thrust::system_error(
        error, thrust::cuda_category(),
        "managed_allocator::allocate(): cudaMallocManaged");
    }

    return thrust::device_pointer_cast(result);
  }

  inline void deallocate(pointer ptr, size_t)
  {
    cudaError_t error = cudaFree(thrust::raw_pointer_cast(ptr));

    if (error != cudaSuccess) {
      throw thrust::system_error(error, thrust::cuda_category(),
                                 "managed_allocator::deallocate(): cudaFree");
    }
  }
};

#elif defined(GTENSOR_DEVICE_HIP)

template <class T>
class managed_allocator : public thrust::device_allocator<T>
{
public:
  using value_type = T;

  typedef thrust::device_ptr<T> pointer;
  inline pointer allocate(size_t n)
  {
    value_type* result = nullptr;

    hipError_t error =
      hipMallocManaged(&result, n * sizeof(T), hipMemAttachGlobal);

    if (error != hipSuccess) {
      throw thrust::system_error(
        error, thrust::hip_category(),
        "managed_allocator::allocate(): hipMallocManaged");
    }

    return thrust::device_pointer_cast(result);
  }

  inline void deallocate(pointer ptr, size_t)
  {
    hipError_t error = hipFree(thrust::raw_pointer_cast(ptr));

    if (error != hipSuccess) {
      throw thrust::system_error(error, thrust::hip_category(),
                                 "managed_allocator::deallocate(): hipFree");
    }
  }
};

#endif

} // namespace ext

// ======================================================================
// add operators that handle device references to thrust::complex

template <
  typename T, typename U,
  typename Enable = std::enable_if_t<ext::has_device_reference<T, U>::value>>
GT_INLINE auto operator+(T a, U b)
{
  return ext::remove_device_reference_t<T>(a) +
         ext::remove_device_reference_t<U>(b);
}

template <
  typename T, typename U,
  typename Enable = std::enable_if_t<ext::has_device_reference<T, U>::value>>
GT_INLINE auto operator-(T a, U b)
{
  return ext::remove_device_reference_t<T>(a) -
         ext::remove_device_reference_t<U>(b);
}

template <
  typename T, typename U,
  typename Enable = std::enable_if_t<ext::has_device_reference<T, U>::value>>
GT_INLINE auto operator*(T a, U b)
{
  return ext::remove_device_reference_t<T>(a) *
         ext::remove_device_reference_t<U>(b);
}

template <
  typename T, typename U,
  typename Enable = std::enable_if_t<ext::has_device_reference<T, U>::value>>
GT_INLINE auto operator/(T a, U b)
{
  return ext::remove_device_reference_t<T>(a) /
         ext::remove_device_reference_t<U>(b);
}

} // namespace thrust

#endif

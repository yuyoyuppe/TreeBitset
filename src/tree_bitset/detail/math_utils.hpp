#pragma once

#include <type_traits>

namespace math {

template <typename T>
constexpr uint64_t ceil(const T val)
{
  const uint64_t ival = static_cast<uint64_t>(val);
  if(val == static_cast<T>(ival))
    return ival;
  return ival + 1;
}

template <typename T>
constexpr std::enable_if_t<std::is_integral_v<T>, T> int_log2(T val)
{
  T result = 0;
  while(val >>= 1)
    ++result;
  return result;
}

template <typename T>
constexpr T int_log_ceil(T base, T val)
{
  return static_cast<T>(ceil(static_cast<double>(int_log2(val)) / int_log2(base)));
}
}

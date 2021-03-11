/*******************************************************************************
* Copyright 2020-2021 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#pragma once

#ifdef ONEDAL_DATA_PARALLEL
#include <CL/sycl.hpp>
#endif

#include "oneapi/dal/detail/common.hpp"

#if defined(__INTEL_COMPILER)
#define PRAGMA_IVDEP         _Pragma("ivdep")
#define PRAGMA_VECTOR_ALWAYS _Pragma("vector always")
#else
#define PRAGMA_IVDEP
#define PRAGMA_VECTOR_ALWAYS
#endif

namespace oneapi::dal::backend {

/// Finds the largest multiple of `multiple` not larger than `x`
/// Return `x`, if `x` is already multiple of `multiple`
/// Example: down_multiple(10, 4) == 8
/// Example: down_multiple(10, 5) == 10
template <typename Integer>
inline constexpr Integer down_multiple(Integer x, Integer multiple) {
    static_assert(std::is_integral_v<Integer>);
    ONEDAL_ASSERT(x > 0);
    ONEDAL_ASSERT(multiple > 0);
    return (x / multiple) * multiple;
}

/// Finds the smallest multiple of `multiple` not smaller than `x`.
/// Return `x`, if `x` is already multiple of `multiple`
/// Example: up_multiple(10, 4) == 12
/// Example: up_multiple(10, 5) == 10
template <typename Integer>
inline constexpr Integer up_multiple(Integer x, Integer multiple) {
    static_assert(std::is_integral_v<Integer>);
    ONEDAL_ASSERT(x > 0);
    ONEDAL_ASSERT(multiple > 0);
    const Integer y = down_multiple<Integer>(x, multiple);
    const Integer z = multiple * Integer((x % multiple) != 0);
    ONEDAL_ASSERT_SUM_OVERFLOW(Integer, y, z);
    return y + z;
}

/// Finds the largest power of 2 number not larger than `x`.
/// Return `x`, if `x` is already power of 2
/// Example: down_pow2(10) == 8
/// Example: down_pow2(16) == 16
template <typename Integer>
inline constexpr Integer down_pow2(Integer x) {
    static_assert(std::is_integral_v<Integer>);
    ONEDAL_ASSERT(x > 0);
    Integer power = 1;
    while (power < x / 2) {
        power *= 2;
    }
    return power;
}

/// Finds the smallest power of 2 number not smaller than `x`.
/// Return `x`, if `x` is already power of 2
/// Example: up_pow2(10) == 16
/// Example: up_pow2(16) == 16
template <typename Integer>
inline constexpr Integer up_pow2(Integer x) {
    static_assert(std::is_integral_v<Integer>);
    ONEDAL_ASSERT(x > 0);
    Integer power = 1;
    while (power < x) {
        power *= 2;
    }
    return power;
}

#ifdef ONEDAL_DATA_PARALLEL

using event_vector = std::vector<sycl::event>;

/// Creates `nd_range`, where global size is multiple of local size
inline sycl::nd_range<1> make_multiple_nd_range_1d(std::int64_t global_size,
                                                   std::int64_t local_size) {
    const std::int64_t g = dal::detail::integral_cast<std::size_t>(global_size);
    const std::int64_t l = dal::detail::integral_cast<std::size_t>(local_size);
    return { up_multiple(g, l), l };
}

#endif

} // namespace oneapi::dal::backend

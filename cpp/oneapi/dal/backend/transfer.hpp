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

#include "oneapi/dal/array.hpp"
#include "oneapi/dal/backend/memory.hpp"

namespace oneapi::dal::backend {

#ifdef ONEDAL_DATA_PARALLEL
template <typename T>
inline std::tuple<array<T>, sycl::event> to_device(sycl::queue& q, const array<T>& ary) {
    if (!ary.get_count()) {
        return { ary, sycl::event{} };
    }

    if (ary.get_queue().has_value()) {
        // TODO: Consider replacing by exception
        ONEDAL_ASSERT(ary.get_queue().value() == q,
                      "We do not support data tranfer between different queues yet");

        const auto ary_alloc = sycl::get_pointer_type(ary.get_data(), q.get_context());
        if (ary_alloc == sycl::usm::alloc::device) {
            return { ary, sycl::event{} };
        }
    }

    const std::int64_t element_count = ary.get_count();
    const auto ary_device = array<T>::empty(q, element_count, sycl::usm::alloc::device);
    const auto event = q.memcpy(ary_device.get_mutable_data(),
                                ary.get_data(),
                                sizeof(T) * dal::detail::integral_cast<std::size_t>(element_count));
    return { ary_device, event };
}

template <typename T>
inline std::tuple<array<T>, sycl::event> to_host(const array<T>& ary) {
    if (!ary.get_count()) {
        return { ary, sycl::event{} };
    }

    if (!ary.get_queue().has_value()) {
        // Data is already on host
        return { ary, sycl::event{} };
    }

    const std::int64_t element_count = ary.get_count();

    ONEDAL_ASSERT(ary.get_queue().has_value());
    auto q = ary.get_queue().value();

    // TODO: Change allocation kind to normal host memory once
    //       bug in `memcpy` with the host memory is fixed
    const auto ary_host = array<T>::empty(q, element_count, sycl::usm::alloc::host);
    const auto event = q.memcpy(ary_host.get_mutable_data(),
                                ary.get_data(),
                                sizeof(T) * dal::detail::integral_cast<std::size_t>(element_count));
    return { ary_host, event };
}

template <typename T>
inline array<T> to_device_sync(sycl::queue& q, const array<T>& ary) {
    auto [ary_device, event] = to_device(q, ary);
    event.wait_and_throw();
    return ary_device;
}

template <typename T>
inline array<T> to_host_sync(const array<T>& ary) {
    auto [ary_host, event] = to_host(ary);
    event.wait_and_throw();
    return ary_host;
}

#endif

} // namespace oneapi::dal::backend

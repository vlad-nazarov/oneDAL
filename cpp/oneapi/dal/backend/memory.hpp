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
#include "oneapi/dal/backend/common.hpp"

namespace oneapi::dal::backend {

#ifdef ONEDAL_DATA_PARALLEL
inline bool is_device_usm_pointer(const sycl::queue& queue, const void* pointer) {
    const auto pointer_type = sycl::get_pointer_type(pointer, queue.get_context());
    return pointer_type == sycl::usm::alloc::device;
}

inline bool is_shared_usm_pointer(const sycl::queue& queue, const void* pointer) {
    const auto pointer_type = sycl::get_pointer_type(pointer, queue.get_context());
    return pointer_type == sycl::usm::alloc::shared;
}

inline bool is_host_usm_pointer(const sycl::queue& queue, const void* pointer) {
    const auto pointer_type = sycl::get_pointer_type(pointer, queue.get_context());
    return pointer_type == sycl::usm::alloc::host;
}

inline bool is_device_friendly_usm_pointer(const sycl::queue& queue, const void* pointer) {
    const auto pointer_type = sycl::get_pointer_type(pointer, queue.get_context());
    return (pointer_type == sycl::usm::alloc::device) || //
           (pointer_type == sycl::usm::alloc::shared);
}

inline bool is_known_usm_pointer_type(const sycl::queue& queue, const void* pointer) {
    auto pointer_type = sycl::get_pointer_type(pointer, queue.get_context());
    return pointer_type != sycl::usm::alloc::unknown;
}

inline bool is_same_context(const sycl::queue& q1, const sycl::queue& q2) {
    return q1.get_context() == q2.get_context();
}

inline bool is_same_context(const sycl::queue& q1, const sycl::queue& q2, const sycl::queue& q3) {
    return is_same_context(q1, q2) && is_same_context(q1, q3);
}

inline bool is_same_context(const sycl::queue& q1,
                            const sycl::queue& q2,
                            const sycl::queue& q3,
                            const sycl::queue& q4) {
    return is_same_context(q1, q2, q3) && is_same_context(q1, q4);
}

inline bool is_same_device(const sycl::queue& q1, const sycl::queue& q2) {
    return q1.get_device() == q2.get_device();
}

inline bool is_same_device(const sycl::queue& q1, const sycl::queue& q2, const sycl::queue& q3) {
    return is_same_device(q1, q2) && is_same_device(q1, q3);
}

inline bool is_same_device(const sycl::queue& q1,
                           const sycl::queue& q2,
                           const sycl::queue& q3,
                           const sycl::queue& q4) {
    return is_same_device(q1, q2, q3) && is_same_device(q1, q4);
}

inline void check_if_same_context(const sycl::queue& q1, const sycl::queue& q2) {
    if (!is_same_context(q1, q2)) {
        throw invalid_argument{ dal::detail::error_messages::queues_in_different_contexts() };
    }
}

inline void check_if_same_context(const sycl::queue& q1,
                                  const sycl::queue& q2,
                                  const sycl::queue& q3) {
    check_if_same_context(q1, q2);
    check_if_same_context(q1, q3);
}

inline void check_if_same_context(const sycl::queue& q1,
                                  const sycl::queue& q2,
                                  const sycl::queue& q3,
                                  const sycl::queue& q4) {
    check_if_same_context(q1, q2, q3);
    check_if_same_context(q1, q4);
}

inline void* malloc(const sycl::queue& queue, std::size_t size, const sycl::usm::alloc& alloc) {
    auto ptr = sycl::malloc(size, queue, alloc);
    if (!ptr) {
        if (alloc == sycl::usm::alloc::shared || alloc == sycl::usm::alloc::host) {
            throw dal::host_bad_alloc{};
        }
        else if (alloc == sycl::usm::alloc::device) {
            throw dal::device_bad_alloc{};
        }
        else {
            throw dal::invalid_argument{ detail::error_messages::unknown_usm_pointer_type() };
        }
    }
    return ptr;
}

inline void* malloc_device(const sycl::queue& queue, std::size_t size) {
    return malloc(queue, size, sycl::usm::alloc::device);
}

inline void* malloc_shared(const sycl::queue& queue, std::size_t size) {
    return malloc(queue, size, sycl::usm::alloc::shared);
}

inline void* malloc_host(const sycl::queue& queue, std::size_t size) {
    return malloc(queue, size, sycl::usm::alloc::host);
}

inline void free(const sycl::queue& queue, void* pointer) {
    ONEDAL_ASSERT(pointer == nullptr || is_known_usm_pointer_type(queue, pointer));
    sycl::free(pointer, queue);
}

template <typename T>
inline T* malloc(const sycl::queue& queue, std::int64_t count, const sycl::usm::alloc& alloc) {
    ONEDAL_ASSERT_MUL_OVERFLOW(std::size_t, sizeof(T), count);
    const std::size_t bytes_count = sizeof(T) * count;
    return static_cast<T*>(malloc(queue, bytes_count, alloc));
}

template <typename T>
inline T* malloc_device(const sycl::queue& queue, std::int64_t count) {
    return malloc<T>(queue, count, sycl::usm::alloc::device);
}

template <typename T>
inline T* malloc_shared(const sycl::queue& queue, std::int64_t count) {
    return malloc<T>(queue, count, sycl::usm::alloc::shared);
}

template <typename T>
inline T* malloc_host(const sycl::queue& queue, std::int64_t count) {
    return malloc<T>(queue, count, sycl::usm::alloc::host);
}

template <typename T>
class usm_deleter {
public:
    explicit usm_deleter(const sycl::queue& queue) : queue_(queue) {}

    void operator()(T* ptr) const {
        free(queue_, ptr);
    }

    sycl::queue& get_queue() {
        return queue_;
    }

    const sycl::queue& get_queue() const {
        return queue_;
    }

private:
    sycl::queue queue_;
};

template <typename T>
using unique_usm_ptr = std::unique_ptr<T, usm_deleter<T>>;

inline unique_usm_ptr<void> make_unique_usm(const sycl::queue& q,
                                            std::size_t size,
                                            sycl::usm::alloc alloc) {
    return unique_usm_ptr<void>{ malloc(q, size, alloc), usm_deleter<void>{ q } };
}

inline unique_usm_ptr<void> make_unique_usm_device(const sycl::queue& q, std::size_t size) {
    return unique_usm_ptr<void>{ malloc_device(q, size), usm_deleter<void>{ q } };
}

inline unique_usm_ptr<void> make_unique_usm_shared(const sycl::queue& q, std::size_t size) {
    return unique_usm_ptr<void>{ malloc_shared(q, size), usm_deleter<void>{ q } };
}

inline unique_usm_ptr<void> make_unique_usm_host(const sycl::queue& q, std::size_t size) {
    return unique_usm_ptr<void>{ malloc_host(q, size), usm_deleter<void>{ q } };
}

template <typename T>
inline unique_usm_ptr<T> make_unique_usm(const sycl::queue& q,
                                         std::int64_t count,
                                         sycl::usm::alloc alloc) {
    return unique_usm_ptr<T>{ malloc<T>(q, count, alloc), usm_deleter<T>{ q } };
}

template <typename T>
inline unique_usm_ptr<T> make_unique_usm_device(const sycl::queue& q, std::int64_t count) {
    return unique_usm_ptr<T>{ malloc_device<T>(q, count), usm_deleter<T>{ q } };
}

template <typename T>
inline unique_usm_ptr<T> make_unique_usm_shared(const sycl::queue& q, std::int64_t count) {
    return unique_usm_ptr<T>{ malloc_shared<T>(q, count), usm_deleter<T>{ q } };
}

template <typename T>
inline unique_usm_ptr<T> make_unique_usm_host(const sycl::queue& q, std::int64_t count) {
    return unique_usm_ptr<T>{ malloc_host<T>(q, count), usm_deleter<T>{ q } };
}

template <typename T>
inline bool is_device_usm(const array<T>& ary) {
    if (ary.get_queue().has_value()) {
        auto q = ary.get_queue().value();
        return is_device_usm_pointer(q, ary.get_data());
    }
    else {
        return false;
    }
}

template <typename T>
inline bool is_same_context(const sycl::queue& q, const array<T>& ary) {
    if (ary.get_queue().has_value()) {
        auto ary_q = ary.get_queue().value();
        return is_same_context(q, ary_q);
    }
    else {
        return false;
    }
}

#endif

} // namespace oneapi::dal::backend

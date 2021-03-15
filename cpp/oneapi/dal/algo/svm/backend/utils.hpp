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

#include <daal/include/algorithms/svm/svm_train_types.h>

#include "oneapi/dal/table/row_accessor.hpp"
#include "oneapi/dal/detail/error_messages.hpp"
#include "oneapi/dal/backend/transfer.hpp"

namespace oneapi::dal::svm::backend {

namespace daal_svm = daal::algorithms::svm;

template <daal_svm::training::Method Value>
using daal_method_constant = std::integral_constant<daal_svm::training::Method, Value>;

template <typename Method>
struct to_daal_method;

template <>
struct to_daal_method<method::smo> : daal_method_constant<daal_svm::training::boser> {};

template <>
struct to_daal_method<method::thunder> : daal_method_constant<daal_svm::training::thunder> {};

template <typename Float>
struct binary_label_t {
    Float first;
    Float second;
};

template <typename Float>
inline array<Float> convert_labels(const array<Float>& arr_label,
                                   const binary_label_t<Float>& in_binary_labels,
                                   binary_label_t<Float>& out_binary_labels) {
    const std::int64_t count = arr_label.get_count();
    auto new_label_arr = array<Float>::empty(count);
    auto new_label_data = new_label_arr.get_mutable_data();

    Float value_first_class_label = arr_label[0];
    Float value_second_class_label = arr_label[1];

    new_label_data[0] = in_binary_labels.first;
    std::int64_t i = 1;
    for (; i < count; ++i) {
        if (arr_label[i] == value_first_class_label) {
            new_label_data[i] = in_binary_labels.first;
        }
        else {
            value_second_class_label = arr_label[i];
            new_label_data[i] = in_binary_labels.second;
            break;
        }
    }
    if (value_first_class_label == value_second_class_label) {
        throw invalid_argument(
            dal::detail::error_messages::input_labels_contain_only_one_unique_value_expect_two());
    }

    for (; i < count; ++i) {
        if (arr_label[i] == value_first_class_label) {
            new_label_data[i] = in_binary_labels.first;
        }
        else if (arr_label[i] == value_second_class_label) {
            new_label_data[i] = in_binary_labels.second;
        }
        else {
            throw invalid_argument(dal::detail::error_messages::
                                       input_labels_contain_wrong_unique_values_count_expect_two());
        }
    }

    out_binary_labels.first = value_first_class_label;
    out_binary_labels.second = value_second_class_label;
    return new_label_arr;
}

#ifdef ONEDAL_DATA_PARALLEL
template <typename Float>
inline array<Float> convert_labels(sycl::queue& queue,
                                   const array<Float>& arr_label,
                                   const binary_label_t<Float>& in_binary_labels,
                                   binary_label_t<Float>& out_binary_labels) {
    // TODO: Implement conversion on device
    using error_msg = dal::detail::error_messages;
    const std::int64_t count = arr_label.get_count();

    // TODO: Replace allocation type once bug with `memcpy` and host memory is fixed
    auto new_label_arr = array<Float>::empty(queue, count, sycl::usm::alloc::host);
    auto new_label_data = new_label_arr.get_mutable_data();

    const auto arr_label_host = dal::backend::to_host_sync(arr_label);
    Float value_first_class_label = arr_label_host[0];
    Float value_second_class_label = arr_label_host[1];

    new_label_data[0] = in_binary_labels.first;
    std::int64_t i = 1;
    for (; i < count; ++i) {
        if (arr_label_host[i] == value_first_class_label) {
            new_label_data[i] = in_binary_labels.first;
        }
        else {
            value_second_class_label = arr_label_host[i];
            new_label_data[i] = in_binary_labels.second;
            break;
        }
    }
    if (value_first_class_label == value_second_class_label) {
        throw invalid_argument{
            error_msg::input_labels_contain_only_one_unique_value_expect_two()
        };
    }

    for (; i < count; ++i) {
        if (arr_label_host[i] == value_first_class_label) {
            new_label_data[i] = in_binary_labels.first;
        }
        else if (arr_label_host[i] == value_second_class_label) {
            new_label_data[i] = in_binary_labels.second;
        }
        else {
            throw invalid_argument{
                error_msg::input_labels_contain_wrong_unique_values_count_expect_two()
            };
        }
    }

    out_binary_labels.first = value_first_class_label;
    out_binary_labels.second = value_second_class_label;

    return dal::backend::to_device_sync(queue, new_label_arr);
}
#endif

} // namespace oneapi::dal::svm::backend

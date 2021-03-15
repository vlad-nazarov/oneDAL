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

#include "oneapi/dal/algo/linear_kernel/backend/gpu/compute_kernel.hpp"
#include "oneapi/dal/backend/interop/common.hpp"
#include "oneapi/dal/backend/interop/common_dpc.hpp"
#include "oneapi/dal/backend/interop/table_conversion.hpp"

#include "oneapi/dal/table/row_accessor.hpp"

#include <daal/src/algorithms/kernel_function/oneapi/kernel_function_linear_kernel_oneapi.h>

namespace oneapi::dal::linear_kernel::backend {

using dal::backend::context_gpu;
using input_t = compute_input<task::compute>;
using result_t = compute_result<task::compute>;
using descriptor_t = detail::descriptor_base<task::compute>;

namespace daal_linear_kernel = daal::algorithms::kernel_function::linear;
namespace interop = dal::backend::interop;

template <typename Float>
using daal_linear_kernel_t =
    daal_linear_kernel::internal::KernelImplLinearOneAPI<daal_linear_kernel::defaultDense, Float>;

template <typename Float>
static result_t call_daal_kernel(const context_gpu& ctx,
                                 const descriptor_t& desc,
                                 const table& x,
                                 const table& y) {
    auto& queue = ctx.get_queue();
    interop::execution_context_guard guard(queue);

    const int64_t row_count_x = x.get_row_count();
    const int64_t row_count_y = y.get_row_count();

    dal::detail::check_mul_overflow(row_count_x, row_count_y);
    auto arr_values =
        array<Float>::empty(queue, row_count_x * row_count_y, sycl::usm::alloc::device);

    const auto daal_x = interop::convert_to_daal_table(queue, x);
    const auto daal_y = interop::convert_to_daal_table(queue, y);
    const auto daal_values =
        interop::convert_to_daal_table(queue, arr_values, row_count_x, row_count_y);

    daal_linear_kernel::Parameter daal_parameter(desc.get_scale(), desc.get_shift());
    daal_linear_kernel_t<Float>().compute(daal_x.get(),
                                          daal_y.get(),
                                          daal_values.get(),
                                          &daal_parameter);

    return result_t{}.set_values(
        dal::detail::homogen_table_builder{}.reset(arr_values, row_count_x, row_count_y).build());
}

template <typename Float>
static result_t compute(const context_gpu& ctx, const descriptor_t& desc, const input_t& input) {
    return call_daal_kernel<Float>(ctx, desc, input.get_x(), input.get_y());
}

template <typename Float>
struct compute_kernel_gpu<Float, method::dense, task::compute> {
    result_t operator()(const context_gpu& ctx,
                        const descriptor_t& desc,
                        const input_t& input) const {
        return compute<Float>(ctx, desc, input);
    }
};

template struct compute_kernel_gpu<float, method::dense, task::compute>;
template struct compute_kernel_gpu<double, method::dense, task::compute>;

} // namespace oneapi::dal::linear_kernel::backend

/* file: kernel_function_polynomial_base.h */
/*******************************************************************************
* Copyright 2021 Intel Corporation
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

#ifndef __KERNEL_FUNCTION_POLYNOMIAL_BASE_H__
#define __KERNEL_FUNCTION_POLYNOMIAL_BASE_H__

#include "src/algorithms/kernel_function/polynomial/kernel_function_types_polynomial.h"
#include "src/algorithms/kernel.h"

namespace daal
{
namespace algorithms
{
namespace kernel_function
{
namespace polynomial
{
namespace internal
{
using namespace daal::internal;

template <Method method, typename algorithmFPType, CpuType cpu>
struct KernelImplPolynomial
{};

} // namespace internal
} // namespace polynomial
} // namespace kernel_function
} // namespace algorithms
} // namespace daal

#endif

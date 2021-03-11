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

#include "oneapi/dal/test/engine/common.hpp"
#include "oneapi/dal/table/column_accessor.hpp"
#include "oneapi/dal/table/homogen.hpp"
#include "oneapi/dal/table/row_accessor.hpp"

namespace oneapi::dal {

TEST("can read table data via row accessor") {
    using oneapi::dal::detail::empty_delete;

    double data[] = { 1.0, 2.0, 3.0, -1.0, -2.0, -3.0 };

    homogen_table t{ data, 2, 3, empty_delete<const double>() };
    const auto rows_block = row_accessor<const double>(t).pull({ 0, -1 });

    REQUIRE(t.get_row_count() * t.get_column_count() == rows_block.get_count());
    REQUIRE(data == rows_block.get_data());

    for (std::int64_t i = 0; i < rows_block.get_count(); i++) {
        REQUIRE(rows_block[i] == data[i]);
    }
}

TEST("can read table data via row accessor with conversion") {
    using oneapi::dal::detail::empty_delete;

    float data[] = { 1.0f, 2.0f, 3.0f, -1.0f, -2.0f, -3.0f };

    homogen_table t{ data, 2, 3, empty_delete<const float>() };
    auto rows_block = row_accessor<const double>(t).pull({ 0, -1 });

    REQUIRE(t.get_row_count() * t.get_column_count() == rows_block.get_count());
    REQUIRE((void*)data != (void*)rows_block.get_data());

    for (std::int64_t i = 0; i < rows_block.get_count(); i++) {
        REQUIRE(rows_block[i] == Approx(static_cast<double>(data[i])));
    }
}

TEST("can_read table data via row accessor and array outside") {
    using oneapi::dal::detail::empty_delete;

    float data[] = { 1.0f, 2.0f, 3.0f, -1.0f, -2.0f, -3.0f };

    homogen_table t{ data, 2, 3, empty_delete<const float>() };
    auto arr = array<float>::empty(10);

    auto rows_ptr = row_accessor<const float>(t).pull(arr, { 0, -1 });

    REQUIRE(t.get_row_count() * t.get_column_count() == arr.get_count());

    REQUIRE(data == rows_ptr);
    REQUIRE(data == arr.get_data());

    auto data_ptr = arr.get_data();
    for (std::int64_t i = 0; i < arr.get_count(); i++) {
        REQUIRE(rows_ptr[i] == data[i]);
        REQUIRE(data_ptr[i] == data[i]);
    }
}

TEST("can read rows from column major table") {
    float data[] = { 1.0f, 2.0f, 3.0f, -1.0f, -2.0f, -3.0f };

    auto t = homogen_table::wrap(data, 3, 2, data_layout::column_major);

    auto rows_data = row_accessor<const float>(t).pull({ 1, -1 });

    REQUIRE(rows_data.get_count() == 2 * t.get_column_count());

    REQUIRE(rows_data[0] == Approx(2.0f));
    REQUIRE(rows_data[1] == Approx(-2.0f));
    REQUIRE(rows_data[2] == Approx(3.0f));
    REQUIRE(rows_data[3] == Approx(-3.0f));
}

TEST("can read rows from column major table with conversion") {
    float data[] = { 1.0f, 2.0f, 3.0f, -1.0f, -2.0f, -3.0f };

    auto t = homogen_table::wrap(data, 3, 2, data_layout::column_major);

    auto rows_data = row_accessor<const std::int32_t>(t).pull({ 1, 2 });

    REQUIRE(rows_data.get_count() == 1 * t.get_column_count());
    REQUIRE(rows_data[0] == 2);
    REQUIRE(rows_data[1] == -2);
}

TEST("pull throws exception if invalid range") {
    detail::homogen_table_builder b;
    b.reset(array<float>::zeros(3 * 2), 3, 2);
    row_accessor<float> acc{ b };

    REQUIRE_THROWS_AS(acc.pull({ 1, 4 }), dal::range_error);
}

TEST("push throws exception if invalid range") {
    detail::homogen_table_builder b;
    b.reset(array<float>::zeros(3 * 2), 3, 2);
    row_accessor<float> acc{ b };
    auto rows_data = acc.pull({ 1, 2 });

    REQUIRE_THROWS_AS(acc.push(rows_data, { 0, 2 }), dal::range_error);
    REQUIRE_THROWS_AS(acc.push(rows_data, { 3, 4 }), dal::range_error);
}

#ifdef ONEDAL_DATA_PARALLEL
TEST("pull with queue throws exception if invalid range") {
    DECLARE_TEST_POLICY(policy);
    auto& q = policy.get_queue();

    detail::homogen_table_builder b;
    b.reset(array<float>::zeros(q, 3 * 2), 3, 2);
    row_accessor<float> acc{ b };

    REQUIRE_THROWS_AS(acc.pull(q, { 1, 4 }), dal::range_error);
}

TEST("push with queue throws exception if invalid range") {
    DECLARE_TEST_POLICY(policy);
    auto& q = policy.get_queue();

    detail::homogen_table_builder b;
    b.reset(array<float>::zeros(q, 3 * 2), 3, 2);
    row_accessor<float> acc{ b };

    auto rows_data = acc.pull(q, { 1, 2 });
    REQUIRE_THROWS_AS(acc.push(q, rows_data, { 0, 2 }), dal::range_error);
    REQUIRE_THROWS_AS(acc.push(q, rows_data, { 3, 4 }), dal::range_error);
}

TEST("can pull rows as device usm from host-allocated homogen table", "[device-usm]") {
    DECLARE_TEST_POLICY(policy);
    auto& q = policy.get_queue();

    const float data_ptr[] = { 1.0f,  2.0f, //
                               3.0f,  -1.0f, //
                               -2.0f, -3.0f };
    const std::int64_t row_count = 3;
    const std::int64_t column_count = 2;
    const auto data = homogen_table::wrap(data_ptr, row_count, column_count);

    const auto data_arr_device = //
        row_accessor<const float>{ data } //
            .pull(q, { 1, 3 }, sycl::usm::alloc::device);

    auto data_arr_host = array<float>::empty(data_arr_device.get_count());
    q.memcpy(data_arr_host.get_mutable_data(),
             data_arr_device.get_data(),
             sizeof(float) * data_arr_device.get_count())
        .wait_and_throw();
    const float* data_arr_host_ptr = data_arr_host.get_data();

    REQUIRE(data_arr_host_ptr[0] == 3.0f);
    REQUIRE(data_arr_host_ptr[1] == -1.0f);
    REQUIRE(data_arr_host_ptr[2] == -2.0f);
    REQUIRE(data_arr_host_ptr[3] == -3.0f);
}

TEST("can pull rows from column-major shared usm homogen table", "[shared-usm]") {
    DECLARE_TEST_POLICY(policy);
    auto& q = policy.get_queue();

    constexpr std::int64_t row_count = 4;
    constexpr std::int64_t column_count = 3;
    constexpr std::int64_t data_size = row_count * column_count;

    auto data = sycl::malloc_shared<float>(data_size, q);

    q.submit([&](sycl::handler& cgh) {
         cgh.parallel_for(sycl::range<1>(data_size), [=](sycl::id<1> idx) {
             data[idx[0]] = idx[0];
         });
     }).wait();

    auto t = homogen_table::wrap(q, data, row_count, column_count, {}, data_layout::column_major);
    row_accessor<const float> acc{ t };
    auto block = acc.pull(q, { 1, 3 });

    REQUIRE(block.get_count() == 2 * column_count);

    REQUIRE(block[0] == Approx(1));
    REQUIRE(block[1] == Approx(5));
    REQUIRE(block[2] == Approx(9));

    REQUIRE(block[3] == Approx(2));
    REQUIRE(block[4] == Approx(6));
    REQUIRE(block[5] == Approx(10));

    sycl::free(data, q);
}
#endif

} // namespace oneapi::dal

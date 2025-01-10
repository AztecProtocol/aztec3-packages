#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdint>

#include "barretenberg/polynomials/univariate.hpp"
#include "barretenberg/vm2/generated/flavor_settings.hpp"
#include "barretenberg/vm2/generated/full_row.hpp"
#include "barretenberg/vm2/tracegen/ecc_trace.hpp"
#include "barretenberg/vm2/tracegen/test_trace_container.hpp"

namespace bb::avm2::tracegen {
namespace {

using testing::ElementsAre;
using testing::Field;

using R = TestTraceContainer::Row;
using FF = R::FF;

using GrumpkinPoint = grumpkin::g1::affine_element;

TEST(AvmTraceGenEccTest, TraceGenerationAdd)
{
    TestTraceContainer trace;
    EccTraceBuilder builder;

    bb::grumpkin::fq p_x = grumpkin::fq("0x04c95d1b26d63d46918a156cae92db1bcbc4072a27ec81dc82ea959abdbcf16a");
    bb::grumpkin::fq p_y = grumpkin::fq("0x035b6dd9e63c1370462c74775765d07fc21fd1093cc988149d3aa763bb3dbb60");
    GrumpkinPoint p = GrumpkinPoint{ p_x, p_y };
    bb::grumpkin::fq q_x = grumpkin::fq("0x009242167ec31949c00cbe441cd36757607406e87844fa2c8c4364a4403e66d7");
    bb::grumpkin::fq q_y = grumpkin::fq("0x0fe3016d64cfa8045609f375284b6b739b5fa282e4cbb75cc7f1687ecc7420e3");
    GrumpkinPoint q = GrumpkinPoint{ q_x, q_y };
    bb::grumpkin::fq r_x = grumpkin::fq("0x2b01df0ef6d941a826bea23bece8243cbcdc159d5e97fbaa2171f028e05ba9b6");
    bb::grumpkin::fq r_y = grumpkin::fq("0x0cc4c71e882bc62b7b3d1964a8540cb5211339dfcddd2e095fd444bf1aed4f09");
    GrumpkinPoint r = GrumpkinPoint{ r_x, r_y };
    builder.process({ { .p = p, .q = q, .result = r } }, trace);

    EXPECT_THAT(trace.as_rows(),
                ElementsAre(
                    // Only one row.
                    AllOf(Field(&R::ecc_add_op, 1),
                          Field(&R::ecc_double_op, 0),
                          Field(&R::ecc_inv_2_p_y, FF::zero()),
                          Field(&R::ecc_inv_x_diff, (q.x - p.x).invert()),
                          Field(&R::ecc_inv_y_diff, (q.y - p.y).invert()),
                          Field(&R::ecc_lambda, (q.y - p.y) / (q.x - p.x)),
                          Field(&R::ecc_p_is_inf, 0),
                          Field(&R::ecc_p_x, p.x),
                          Field(&R::ecc_p_y, p.y),
                          Field(&R::ecc_q_is_inf, 0),
                          Field(&R::ecc_q_x, q.x),
                          Field(&R::ecc_q_y, q.y),
                          Field(&R::ecc_r_is_inf, 0),
                          Field(&R::ecc_r_x, r.x),
                          Field(&R::ecc_r_y, r.y),
                          Field(&R::ecc_result_infinity, 0),
                          Field(&R::ecc_sel, 1),
                          Field(&R::ecc_x_match, 0),
                          Field(&R::ecc_y_match, 0))));
}

TEST(AvmTraceGenEccTest, TraceGenerationDouble)
{
    TestTraceContainer trace;
    EccTraceBuilder builder;

    bb::grumpkin::fq p_x = grumpkin::fq("0x04c95d1b26d63d46918a156cae92db1bcbc4072a27ec81dc82ea959abdbcf16a");
    bb::grumpkin::fq p_y = grumpkin::fq("0x035b6dd9e63c1370462c74775765d07fc21fd1093cc988149d3aa763bb3dbb60");
    GrumpkinPoint p = GrumpkinPoint{ p_x, p_y };
    GrumpkinPoint q = p;
    bb::grumpkin::fq r_x = grumpkin::fq("0x2b01df0ef6d941a826bea23bece8243cbcdc159d5e97fbaa2171f028e05ba9b6");
    bb::grumpkin::fq r_y = grumpkin::fq("0x0cc4c71e882bc62b7b3d1964a8540cb5211339dfcddd2e095fd444bf1aed4f09");
    GrumpkinPoint r = GrumpkinPoint{ r_x, r_y };
    builder.process({ { .p = p, .q = q, .result = r } }, trace);

    EXPECT_THAT(trace.as_rows(),
                ElementsAre(
                    // Only one row.
                    AllOf(Field(&R::ecc_add_op, 0),
                          Field(&R::ecc_double_op, 1),
                          Field(&R::ecc_inv_2_p_y, (p.y * 2).invert()),
                          Field(&R::ecc_inv_x_diff, FF::zero()),
                          Field(&R::ecc_inv_y_diff, FF::zero()),
                          Field(&R::ecc_lambda, (p.x * p.x * 3) / (p.y * 2)),
                          Field(&R::ecc_p_is_inf, 0),
                          Field(&R::ecc_p_x, p.x),
                          Field(&R::ecc_p_y, p.y),
                          Field(&R::ecc_q_is_inf, 0),
                          Field(&R::ecc_q_x, p.x),
                          Field(&R::ecc_q_y, p.y),
                          Field(&R::ecc_r_is_inf, 0),
                          Field(&R::ecc_r_x, r.x),
                          Field(&R::ecc_r_y, r.y),
                          Field(&R::ecc_result_infinity, 0),
                          Field(&R::ecc_sel, 1),
                          Field(&R::ecc_x_match, 1),
                          Field(&R::ecc_y_match, 1))));
}

TEST(AvmTraceGenEccTest, TraceGenerationInf)
{
    TestTraceContainer trace;
    EccTraceBuilder builder;

    bb::grumpkin::fq p_x = grumpkin::fq("0x04c95d1b26d63d46918a156cae92db1bcbc4072a27ec81dc82ea959abdbcf16a");
    bb::grumpkin::fq p_y = grumpkin::fq("0x035b6dd9e63c1370462c74775765d07fc21fd1093cc988149d3aa763bb3dbb60");
    GrumpkinPoint p = GrumpkinPoint{ p_x, p_y };

    GrumpkinPoint q = GrumpkinPoint{ p.x, -p.y };
    GrumpkinPoint r = p + q;
    builder.process({ { .p = p, .q = q, .result = r } }, trace);

    EXPECT_THAT(trace.as_rows(),
                ElementsAre(
                    // Only one row.
                    AllOf(Field(&R::ecc_add_op, 0),
                          Field(&R::ecc_double_op, 0),
                          Field(&R::ecc_inv_2_p_y, 0),
                          Field(&R::ecc_inv_x_diff, FF::zero()),
                          Field(&R::ecc_inv_y_diff, (q.y - p.y).invert()),
                          Field(&R::ecc_lambda, 0),
                          Field(&R::ecc_p_is_inf, 0),
                          Field(&R::ecc_p_x, p.x),
                          Field(&R::ecc_p_y, p.y),
                          Field(&R::ecc_q_is_inf, 0),
                          Field(&R::ecc_q_x, q.x),
                          Field(&R::ecc_q_y, q.y),
                          Field(&R::ecc_r_is_inf, 1),
                          Field(&R::ecc_r_x, r.x),
                          Field(&R::ecc_r_y, r.y),
                          Field(&R::ecc_result_infinity, 1),
                          Field(&R::ecc_sel, 1),
                          Field(&R::ecc_x_match, 1),
                          Field(&R::ecc_y_match, 0))));
}

} // namespace
} // namespace bb::avm2::tracegen

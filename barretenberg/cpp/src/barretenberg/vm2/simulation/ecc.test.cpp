#include "barretenberg/vm2/simulation/ecc.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "barretenberg/ecc/groups/affine_element.hpp"
#include "barretenberg/vm2/common/memory_types.hpp"
#include "barretenberg/vm2/simulation/events/ecc_event.hpp"
#include "barretenberg/vm2/simulation/events/event_emitter.hpp"
#include "barretenberg/vm2/simulation/events/memory_event.hpp"
#include "barretenberg/vm2/simulation/memory.hpp"
#include "barretenberg/vm2/simulation/testing/mock_context.hpp"

namespace bb::avm2::simulation {
namespace {

using ::testing::ReturnRef;
using ::testing::StrictMock;

TEST(AvmSimulationEccTest, Add)
{
    NoopEventEmitter<MemoryEvent> emitter;
    Memory mem(/*space_id=*/0, emitter);
    StrictMock<MockContext> context;
    EXPECT_CALL(context, get_memory()).WillRepeatedly(ReturnRef(mem));

    EventEmitter<EccAddEvent> ecc_event_emitter;
    Ecc ecc(ecc_event_emitter);

    bb::grumpkin::fq p_x = grumpkin::fq("0x04c95d1b26d63d46918a156cae92db1bcbc4072a27ec81dc82ea959abdbcf16a");
    bb::grumpkin::fq p_y = grumpkin::fq("0x035b6dd9e63c1370462c74775765d07fc21fd1093cc988149d3aa763bb3dbb60");
    GrumpkinPoint p(p_x, p_y);

    bb::grumpkin::fq q_x = grumpkin::fq("0x009242167ec31949c00cbe441cd36757607406e87844fa2c8c4364a4403e66d7");
    bb::grumpkin::fq q_y = grumpkin::fq("0x0fe3016d64cfa8045609f375284b6b739b5fa282e4cbb75cc7f1687ecc7420e3");
    GrumpkinPoint q(q_x, q_y);

    GrumpkinPoint result = ecc.add(p, q);

    bb::grumpkin::fq r_x = grumpkin::fq("0x2b01df0ef6d941a826bea23bece8243cbcdc159d5e97fbaa2171f028e05ba9b6");
    bb::grumpkin::fq r_y = grumpkin::fq("0x0cc4c71e882bc62b7b3d1964a8540cb5211339dfcddd2e095fd444bf1aed4f09");

    EXPECT_EQ(result.x, r_x);
    EXPECT_EQ(result.y, r_y);
    EXPECT_EQ(result.is_point_at_infinity(), 0);
}

} // namespace
} // namespace bb::avm2::simulation

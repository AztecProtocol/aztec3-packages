#pragma once

#include <cstdint>
#include <memory>

#include "barretenberg/vm2/simulation/ecc.hpp"
#include <gmock/gmock.h>

namespace bb::avm2::simulation {

class MockEcc : public EccInterface {
  public:
    MockEcc();
    ~MockEcc() override;

    MOCK_METHOD(GrumpkinPoint, add, (const GrumpkinPoint& p, const GrumpkinPoint& q), (override));
};

} // namespace bb::avm2::simulation

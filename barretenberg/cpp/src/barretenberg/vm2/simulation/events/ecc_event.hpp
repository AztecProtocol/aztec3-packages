#pragma once

#include <cstdint>

#include "barretenberg/ecc/curves/grumpkin/grumpkin.hpp"
#include "barretenberg/ecc/groups/affine_element.hpp"
#include "barretenberg/vm2/common/memory_types.hpp"
#include "barretenberg/vm2/common/opcodes.hpp"

namespace bb::avm2::simulation {

using GrumpkinPoint = grumpkin::g1::affine_element;

struct EccAddEvent {
    GrumpkinPoint p;
    GrumpkinPoint q;
    GrumpkinPoint result;
};

} // namespace bb::avm2::simulation

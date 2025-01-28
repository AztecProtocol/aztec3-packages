#include "barretenberg/vm2/simulation/ecc.hpp"

#include <cstdint>

namespace bb::avm2::simulation {

GrumpkinPoint Ecc::add(const GrumpkinPoint& p, const GrumpkinPoint& q)
{
    events.emit({ .p = p, .q = q, .result = p + q });
    return p + q;
}

} // namespace bb::avm2::simulation

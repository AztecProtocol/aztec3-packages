#pragma once

#include <cstdint>
#include <memory>

#include "barretenberg/vm2/simulation/events/ecc_event.hpp"
#include "barretenberg/vm2/simulation/events/event_emitter.hpp"

namespace bb::avm2::simulation {

class EccInterface {
  public:
    virtual ~EccInterface() = default;
    virtual GrumpkinPoint add(const GrumpkinPoint& p, const GrumpkinPoint& q) = 0;
};

class Ecc : public EccInterface {
  public:
    Ecc(EventEmitterInterface<EccAddEvent>& event_emitter)
        : events(event_emitter)
    {}

    GrumpkinPoint add(const GrumpkinPoint& p, const GrumpkinPoint& q) override;

  private:
    EventEmitterInterface<EccAddEvent>& events;
};

} // namespace bb::avm2::simulation

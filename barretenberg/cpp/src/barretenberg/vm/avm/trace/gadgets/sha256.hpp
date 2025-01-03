#pragma once

#include "barretenberg/vm/avm/trace/common.hpp"

#include <array>
#include <cstdint>
#include <vector>

namespace bb::avm_trace {

class AvmSha256TraceBuilder {
  public:
    struct Sha256TraceEntry {
        uint32_t clk = 0;
        std::array<uint32_t, 8> state{};
        std::array<uint32_t, 16> input{};
        std::array<uint32_t, 8> output{};
        uint32_t output_offset = 0;
        uint32_t state_offset = 0;
        uint32_t input_offset = 0;
    };

    AvmSha256TraceBuilder() = default;
    void reset();
    // Finalize the trace
    void finalize(std::vector<AvmFullRow<FF>>& main_trace);

    std::array<uint32_t, 8> sha256_compression(const std::array<uint32_t, 8>& h_init,
                                               const std::array<uint32_t, 16>& input,
                                               const uint32_t output_offset,
                                               const uint32_t state_offset,
                                               const uint32_t input_offset,
                                               uint32_t clk);

    size_t size() const { return sha256_trace.size(); }

  private:
    std::vector<Sha256TraceEntry> sha256_trace;
};

} // namespace bb::avm_trace

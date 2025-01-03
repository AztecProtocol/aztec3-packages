
#pragma once

#include <cstddef>
#include <cstdint>

#include "barretenberg/ecc/curves/bn254/fr.hpp"
#include "barretenberg/vm/avm/trace/common.hpp"
#include "barretenberg/vm/avm/trace/opcode.hpp"

namespace bb::avm_trace {

class FixedSha256ConstantsTable {
  public:
    struct Sha256ConstantsRow {
        uint8_t round_index;
        uint32_t round_constant;
    };

    static const FixedSha256ConstantsTable& get();

    size_t size() const;
    const Sha256ConstantsRow& at(size_t i) const;

  private:
    FixedSha256ConstantsTable() = default;
};

template <typename DestRow>
void merge_into(DestRow& dest, const FixedSha256ConstantsTable::Sha256ConstantsRow& src, uint32_t multiplicity)
{
    dest.sha256_params_lookup_table_sel = FF(1);
    dest.sha256_params_lookup_table_round_index = src.round_index;
    dest.sha256_params_lookup_table_round_constant = src.round_constant;

    dest.lookup_round_constant_counts = FF(multiplicity);
}

} // namespace bb::avm_trace

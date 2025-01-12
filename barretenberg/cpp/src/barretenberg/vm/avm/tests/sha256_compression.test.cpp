
#include "barretenberg/common/thread.hpp"
#include "barretenberg/vm/avm/generated/circuit_builder.hpp"
#include "barretenberg/vm/avm/generated/flavor.hpp"
#include "barretenberg/vm/avm/generated/full_row.hpp"
#include "barretenberg/vm/avm/trace/fixed_powers.hpp"

#include "barretenberg/vm/avm/trace/gadgets/poseidon2.hpp"
#include "barretenberg/vm/avm/trace/gadgets/sha256.hpp"
#include <cstdint>
#include <gtest/gtest.h>

namespace tests_avm {
using namespace bb;
using namespace bb::avm;

TEST(AvmShaCompression, shouldHashCorrectly)
{

    using FF = AvmFlavor::FF;
    constexpr size_t TRACE_SIZE = 1 << 8;

    std::vector<AvmFullRow<FF>> trace(TRACE_SIZE);

    std::array<uint32_t, 8> init_constants{ 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                                            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

    bb::avm_trace::AvmSha256TraceBuilder sha256_builder;
    std::cerr << "Generating trace of size " << TRACE_SIZE << "..." << std::endl;

    const std::array<uint32_t, 16>& input{ 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150, 160 };
    auto output = sha256_builder.sha256_compression(
        init_constants, input, /*output_offset=*/0, /*state_offset=*/0, /*input_offset=*/0, /*clk*/ 0);
    sha256_builder.sha256_compression(
        output, input, /*output_offset=*/0, /*state_offset=*/0, /*input_offset=*/0, /*clk*/ 1);

    sha256_builder.finalize(trace);
    trace.insert(trace.begin(),
                 bb::AvmFullRow<bb::fr>{ .main_sel_first = FF(1), .mem_lastAccess = FF(1), .sha256_latch = FF(1) });
    // We build the polynomials needed to run "sumcheck".
    AvmCircuitBuilder cb;
    cb.set_trace(std::move(trace));
    auto polys = cb.compute_polynomials();
    const size_t num_rows = cb.get_estimated_num_finalized_gates();
    std::cerr << "Done computing polynomials..." << std::endl;

    std::cerr << "Accumulating relations..." << std::endl;
    using Relation = avm::sha256<FF>;

    typename Relation::SumcheckArrayOfValuesOverSubrelations result;
    for (auto& r : result) {
        r = 0;
    }

    // We set the conditions up there.
    for (size_t r = 0; r < num_rows; ++r) {
        Relation::accumulate(result, polys.get_row(r), {}, 1);
    }

    for (size_t j = 0; j < result.size(); ++j) {
        if (result[j] != 0) {
            EXPECT_EQ(result[j], 0) << "Relation " << Relation::NAME << " subrelation "
                                    << Relation::get_subrelation_label(j) << " was expected to be zero.";
        }
    }

    //     std::cerr << "Accumulating permutation relations..." << std::endl;
    //
    //     const FF gamma = FF::random_element();
    //     const FF beta = FF::random_element();
    //     bb::RelationParameters<typename AvmFlavor::FF> params{
    //         .beta = beta,
    //         .gamma = gamma,
    //     };
    //     using PermRelations = perm_pos2_fixed_pos2_perm_relation<FF>;
    //
    //     // Check the logderivative relation
    //     bb::compute_logderivative_inverse<AvmFlavor, PermRelations>(polys, params, num_rows);
    //
    //     typename PermRelations::SumcheckArrayOfValuesOverSubrelations lookup_result;
    //
    //     for (auto& r : lookup_result) {
    //         r = 0;
    //     }
    //     for (size_t r = 0; r < num_rows; ++r) {
    //         PermRelations::accumulate(lookup_result, polys.get_row(r), params, 1);
    //     }
    //     for (const auto& j : lookup_result) {
    //         if (j != 0) {
    //             EXPECT_EQ(j, 0) << "Lookup Relation " << PermRelations::NAME << " subrelation ";
    //         }
    //     }
}

} // namespace tests_avm

#pragma once
#include <utility>

#include "barretenberg/plonk_honk_shared/execution_trace/mega_execution_trace.hpp"
#include "barretenberg/stdlib_circuit_builders/op_queue/ecc_op_queue.hpp"
#include "barretenberg/trace_to_polynomials/trace_to_polynomials.hpp"
#include "databus.hpp"
#include "ultra_circuit_builder.hpp"

namespace bb {

template <typename FF> class MegaCircuitBuilder_ : public UltraCircuitBuilder_<MegaExecutionTraceBlocks> {
  private:
    DataBus databus; // Container for public calldata/returndata

  public:
    using ExecutionTrace = MegaExecutionTraceBlocks;

    static constexpr CircuitType CIRCUIT_TYPE = CircuitType::ULTRA;
    static constexpr size_t DEFAULT_NON_NATIVE_FIELD_LIMB_BITS =
        UltraCircuitBuilder_<MegaExecutionTraceBlocks>::DEFAULT_NON_NATIVE_FIELD_LIMB_BITS;

    // Stores record of ecc operations and performs corresponding native operations internally
    std::shared_ptr<ECCOpQueue> op_queue;

    // Indices for constant variables corresponding to ECCOpQueue op codes
    uint32_t null_op_idx;
    uint32_t add_accum_op_idx;
    uint32_t mul_accum_op_idx;
    uint32_t equality_op_idx;

    // Functions for adding ECC op queue "gates"
    ecc_op_tuple queue_ecc_add_accum(const g1::affine_element& point);
    ecc_op_tuple queue_ecc_mul_accum(const g1::affine_element& point, const FF& scalar);
    ecc_op_tuple queue_ecc_eq();

    // Metadata for propagating databus return data commitments via the public input mechanism
    DatabusPropagationData databus_propagation_data;

  private:
    ecc_op_tuple populate_ecc_op_wires(const UltraOp& ultra_op);
    void set_goblin_ecc_op_code_constant_variables();
    void create_databus_read_gate(const databus_lookup_gate_<FF>& in, BusId bus_idx);
    void apply_databus_selectors(BusId bus_idx);

  public:
    MegaCircuitBuilder_(const size_t size_hint = 0,
                        std::shared_ptr<ECCOpQueue> op_queue_in = std::make_shared<ECCOpQueue>())
        : UltraCircuitBuilder_<MegaExecutionTraceBlocks>(size_hint)
        , op_queue(std::move(op_queue_in))
    {
        PROFILE_THIS();

        // Set indices to constants corresponding to Goblin ECC op codes
        set_goblin_ecc_op_code_constant_variables();
    };
    MegaCircuitBuilder_(std::shared_ptr<ECCOpQueue> op_queue_in)
        : MegaCircuitBuilder_(0, op_queue_in)
    {}

    /**
     * @brief Constructor from data generated from ACIR
     *
     * @param op_queue_in Op queue to which goblinized group ops will be added
     * @param witness_values witnesses values known to acir
     * @param public_inputs indices of public inputs in witness array
     * @param varnum number of known witness
     *
     * @note The size of witness_values may be less than varnum. The former is the set of actual witness values known at
     * the time of acir generation. The former may be larger and essentially acounts for placeholders for witnesses that
     * we know will exist but whose values are not known during acir generation. Both are in general less than the total
     * number of variables/witnesses that might be present for a circuit generated from acir, since many gates will
     * depend on the details of the bberg implementation (or more generally on the backend used to process acir).
     */
    MegaCircuitBuilder_(std::shared_ptr<ECCOpQueue> op_queue_in,
                        auto& witness_values,
                        const std::vector<uint32_t>& public_inputs,
                        size_t varnum)
        : UltraCircuitBuilder_<MegaExecutionTraceBlocks>(/*size_hint=*/0, witness_values, public_inputs, varnum)
        , op_queue(std::move(op_queue_in))
    {
        // Set indices to constants corresponding to Goblin ECC op codes
        set_goblin_ecc_op_code_constant_variables();
    };

    /**
     * @brief Convert op code to the witness index for the corresponding op index in the builder
     *
     * @param op_code
     * @return uint32_t
     */
    uint32_t get_ecc_op_idx(const EccOpCode& op_code)
    {
        switch (op_code) {
        case NULL_OP: {
            return null_op_idx;
        }
        case ADD_ACCUM: {
            return add_accum_op_idx;
        }
        case MUL_ACCUM: {
            return mul_accum_op_idx;
        }
        case EQUALITY: {
            return equality_op_idx;
        }
        default: {
            ASSERT(false);
            break;
        }
        }
        return null_op_idx;
    }

    void finalize_circuit(const bool ensure_nonzero);
    void add_ultra_and_mega_gates_to_ensure_all_polys_are_non_zero();
    void add_mega_gates_to_ensure_all_polys_are_non_zero();

    size_t get_num_constant_gates() const override { return 0; }

    /**
     * @brief Get the final number of gates in a circuit, which consists of the sum of:
     * 1) Current number number of actual gates
     * 2) Number of public inputs, as we'll need to add a gate for each of them
     * 3) Number of Rom array-associated gates
     * 4) Number of range-list associated gates
     * 5) Number of non-native field multiplication gates.
     *
     * @return size_t
     */
    size_t get_estimated_num_finalized_gates() const override
    {
        auto num_ultra_gates = UltraCircuitBuilder_<MegaExecutionTraceBlocks>::get_estimated_num_finalized_gates();
        auto num_goblin_ecc_op_gates = this->blocks.ecc_op.size();
        return num_ultra_gates + num_goblin_ecc_op_gates;
    }

    /**
     * @brief Dynamically compute the number of gates added by the "add_gates_to_ensure_all_polys_are_non_zero" method
     * @note This does NOT add the gates to the present builder
     *
     */
    size_t get_num_gates_added_to_ensure_nonzero_polynomials()
    {
        MegaCircuitBuilder_<FF> builder; // instantiate new builder

        size_t num_gates_prior = builder.get_estimated_num_finalized_gates();
        builder.add_ultra_and_mega_gates_to_ensure_all_polys_are_non_zero();
        size_t num_gates_post = builder.get_estimated_num_finalized_gates(); // accounts for finalization gates

        return num_gates_post - num_gates_prior;
    }

    /**x
     * @brief Print the number and composition of gates in the circuit
     *
     */
    void print_num_estimated_finalized_gates() const override
    {
        size_t count = 0;
        size_t rangecount = 0;
        size_t romcount = 0;
        size_t ramcount = 0;
        size_t nnfcount = 0;
        UltraCircuitBuilder_<MegaExecutionTraceBlocks>::get_num_estimated_gates_split_into_components(
            count, rangecount, romcount, ramcount, nnfcount);
        auto num_goblin_ecc_op_gates = this->blocks.ecc_op.size();

        size_t total = count + romcount + ramcount + rangecount + num_goblin_ecc_op_gates;
        std::cout << "gates = " << total << " (arith " << count << ", rom " << romcount << ", ram " << ramcount
                  << ", range " << rangecount << ", non native field gates " << nnfcount << ", goblin ecc op gates "
                  << num_goblin_ecc_op_gates << "), pubinp = " << this->public_inputs.size() << std::endl;
    }

    /**
     * @brief Add a witness variable to the public calldata.
     *
     */
    void add_public_calldata(const uint32_t& in) { return append_to_bus_vector(BusId::CALLDATA, in); }

    /**
     * @brief Add a witness variable to secondary_calldata.
     * @details In practice this is used in aztec by the kernel circuit to recieve output from a function circuit
     *
     */
    void add_public_secondary_calldata(const uint32_t& in)
    {
        return append_to_bus_vector(BusId::SECONDARY_CALLDATA, in);
    }

    /**
     * @brief Add a witness variable to the public return_data.
     *
     */
    void add_public_return_data(const uint32_t& in) { return append_to_bus_vector(BusId::RETURNDATA, in); }

    uint32_t read_bus_vector(BusId bus_idx, const uint32_t& read_idx_witness_idx);

    /**
     * @brief Read from calldata and create a corresponding databus read gate
     *
     * @param read_idx_witness_idx Witness index for the calldata read index
     * @return uint32_t Witness index for the result of the read
     */
    uint32_t read_calldata(const uint32_t& read_idx_witness_idx)
    {
        return read_bus_vector(BusId::CALLDATA, read_idx_witness_idx);
    };

    /**
     * @brief Read from secondary_calldata and create a corresponding databus read gate
     *
     * @param read_idx_witness_idx Witness index for the secondary_calldata read index
     * @return uint32_t Witness index for the result of the read
     */
    uint32_t read_secondary_calldata(const uint32_t& read_idx_witness_idx)
    {
        return read_bus_vector(BusId::SECONDARY_CALLDATA, read_idx_witness_idx);
    };

    /**
     * @brief Read from return_data and create a corresponding databus read gate
     *
     * @param read_idx_witness_idx Witness index for the return_data read index
     * @return uint32_t Witness index for the result of the read
     */
    uint32_t read_return_data(const uint32_t& read_idx_witness_idx)
    {
        return read_bus_vector(BusId::RETURNDATA, read_idx_witness_idx);
    };

    void append_to_bus_vector(const BusId bus_idx, const uint32_t& witness_idx)
    {
        databus[static_cast<size_t>(bus_idx)].append(witness_idx);
    }

    const BusVector& get_calldata() const { return databus[static_cast<size_t>(BusId::CALLDATA)]; }
    const BusVector& get_secondary_calldata() const { return databus[static_cast<size_t>(BusId::SECONDARY_CALLDATA)]; }
    const BusVector& get_return_data() const { return databus[static_cast<size_t>(BusId::RETURNDATA)]; }
    uint64_t estimate_memory() const
    {
        vinfo("++Estimating builder memory++");
        uint64_t result{ 0 };

        // gates:
        for (auto [block, label] : zip_view(this->blocks.get(), this->blocks.get_labels())) {
            uint64_t size{ 0 };
            for (const auto& wire : block.wires) {
                size += wire.capacity() * sizeof(uint32_t);
            }
            for (const auto& selector : block.selectors) {
                size += selector.capacity() * sizeof(FF);
            }
            vinfo(label, " size ", size >> 10, " KiB");
            result += size;
        }

        // variables
        size_t to_add{ this->variables.capacity() * sizeof(FF) };
        result += to_add;
        vinfo("variables: ", to_add);

        // public inputs
        to_add = this->public_inputs.capacity() * sizeof(uint32_t);
        result += to_add;
        vinfo("public inputs: ", to_add);

        // other variable indices
        to_add = this->next_var_index.capacity() * sizeof(uint32_t);
        to_add += this->prev_var_index.capacity() * sizeof(uint32_t);
        to_add += this->real_variable_index.capacity() * sizeof(uint32_t);
        to_add += this->real_variable_tags.capacity() * sizeof(uint32_t);
        result += to_add;
        vinfo("variable indices: ", to_add);

        return result;
    }
};
using MegaCircuitBuilder = MegaCircuitBuilder_<bb::fr>;
} // namespace bb
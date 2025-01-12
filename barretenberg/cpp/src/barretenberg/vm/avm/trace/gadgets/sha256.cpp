#include "barretenberg/crypto/sha256/sha256.hpp"
#include "barretenberg/common/assert.hpp"
#include "barretenberg/numeric/uint128/uint128.hpp"
#include "barretenberg/vm/avm/trace/common.hpp"
#include "barretenberg/vm/avm/trace/gadgets/sha256.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace bb::avm_trace {

void AvmSha256TraceBuilder::reset()
{
    sha256_trace.clear();
    sha256_trace.shrink_to_fit(); // Reclaim memory.
}

// Taken from barretenberg/crypto/sha256/sha256.cpp since it is not exposed directly
constexpr uint32_t round_constants[64]{
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Taken from barretenberg/crypto/sha256/sha256.cpp since it is not exposed directly
constexpr uint32_t ror(uint32_t val, uint32_t shift)
{
    return (val >> (shift & 31U)) | (val << (32U - (shift & 31U)));
}

// Taken from barretenberg/crypto/sha256/sha256.cpp since it is not exposed directly
std::array<uint32_t, 8> sha256_block(const std::array<uint32_t, 8>& h_init, const std::array<uint32_t, 16>& input)
{
    std::array<uint32_t, 64> w;

    /**
     * Fill first 16 words with the message schedule
     **/
    for (size_t i = 0; i < 16; ++i) {
        w[i] = input[i];
    }

    /**
     * Extend the input data into the remaining 48 words
     **/
    for (size_t i = 16; i < 64; ++i) {
        uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + w[i - 7] + s0 + s1;
    }

    /**
     * Initialize round variables with previous block output
     **/
    uint32_t a = h_init[0];
    uint32_t b = h_init[1];
    uint32_t c = h_init[2];
    uint32_t d = h_init[3];
    uint32_t e = h_init[4];
    uint32_t f = h_init[5];
    uint32_t g = h_init[6];
    uint32_t h = h_init[7];

    /**
     * Apply SHA-256 compression function to the message schedule
     **/
    for (size_t i = 0; i < 64; ++i) {
        uint32_t S1 = ror(e, 6U) ^ ror(e, 11U) ^ ror(e, 25U);
        uint32_t ch = (e & f) ^ (~e & g); // === (e & f) ^ (~e & g), `+` op is cheaper
        uint32_t temp1 = h + S1 + ch + round_constants[i] + w[i];
        uint32_t S0 = ror(a, 2U) ^ ror(a, 13U) ^ ror(a, 22U);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c); // (a & (b + c - (T0 * 2))) + T0; // === (a & b) ^ (a & c) ^ (b & c)
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    /**
     * Add into previous block output and return
     **/
    std::array<uint32_t, 8> output;
    output[0] = a + h_init[0];
    output[1] = b + h_init[1];
    output[2] = c + h_init[2];
    output[3] = d + h_init[3];
    output[4] = e + h_init[4];
    output[5] = f + h_init[5];
    output[6] = g + h_init[6];
    output[7] = h + h_init[7];
    return output;
}

std::array<uint32_t, 8> AvmSha256TraceBuilder::sha256_compression(const std::array<uint32_t, 8>& h_init,
                                                                  const std::array<uint32_t, 16>& input,
                                                                  const uint32_t output_offset,
                                                                  const uint32_t state_offset,
                                                                  const uint32_t input_offset,
                                                                  uint32_t clk)
{
    auto output = sha256_block(h_init, input);
    sha256_trace.push_back(Sha256TraceEntry{ .clk = clk,
                                             .state = h_init,
                                             .input = input,
                                             .output = output,
                                             .output_offset = output_offset,
                                             .state_offset = state_offset,
                                             .input_offset = input_offset });
    return output;
}

static std::tuple<uint32_t, uint32_t> bit_limbs(const uint32_t a, const uint8_t b)
{
    uint32_t a_lhs = a >> b;
    uint32_t a_rhs = a & ((static_cast<uint32_t>(1) << b) - 1);
    return std::make_tuple(a_lhs, a_rhs);
}

void AvmSha256TraceBuilder::finalize(std::vector<AvmFullRow<FF>>& main_trace)
{

    for (size_t i = 0; i < sha256_trace.size(); i++) {
        const auto& src = sha256_trace.at(i);

        std::array<uint32_t, 64> w;
        for (size_t i = 0; i < 16; ++i) {
            w[i] = src.input[i];
        }
        for (size_t i = 16; i < 64; ++i) {
            uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + w[i - 7] + s0 + s1;
        }

        uint32_t prev_helper_w0 = w[0];
        uint32_t prev_helper_w1 = w[1];
        uint32_t prev_helper_w2 = w[2];
        uint32_t prev_helper_w3 = w[3];
        uint32_t prev_helper_w4 = w[4];
        uint32_t prev_helper_w5 = w[5];
        uint32_t prev_helper_w6 = w[6];
        uint32_t prev_helper_w7 = w[7];
        uint32_t prev_helper_w8 = w[8];
        uint32_t prev_helper_w9 = w[9];
        uint32_t prev_helper_w10 = w[10];
        uint32_t prev_helper_w11 = w[11];
        uint32_t prev_helper_w12 = w[12];
        uint32_t prev_helper_w13 = w[13];
        uint32_t prev_helper_w14 = w[14];
        uint32_t prev_helper_w15 = w[15];

        uint32_t a = src.state[0];
        uint32_t b = src.state[1];
        uint32_t c = src.state[2];
        uint32_t d = src.state[3];
        uint32_t e = src.state[4];
        uint32_t f = src.state[5];
        uint32_t g = src.state[6];
        uint32_t h = src.state[7];

        for (size_t j = 0; j < 65; ++j) {
            auto& dest = main_trace.at((i * 65) + j);
            dest.sha256_clk = FF(static_cast<uint32_t>(src.clk));
            dest.sha256_sel = FF(1);
            if (j == 0) {
                // Temp - do memory load stuff here too
                dest.sha256_start = FF(1);
                dest.sha256_input_offset = FF(static_cast<uint32_t>(src.input_offset));
                dest.sha256_state_offset = FF(static_cast<uint32_t>(src.state_offset));
                dest.sha256_output_offset = FF(static_cast<uint32_t>(src.output_offset));
            }
            // Store initial state
            dest.sha256_init_a = src.state[0];
            dest.sha256_init_b = src.state[1];
            dest.sha256_init_c = src.state[2];
            dest.sha256_init_d = src.state[3];
            dest.sha256_init_e = src.state[4];
            dest.sha256_init_f = src.state[5];
            dest.sha256_init_g = src.state[6];
            dest.sha256_init_h = src.state[7];
            // Set up initial state
            dest.sha256_a = a;
            dest.sha256_b = b;
            dest.sha256_c = c;
            dest.sha256_d = d;
            dest.sha256_e = e;
            dest.sha256_f = f;
            dest.sha256_g = g;
            dest.sha256_h = h;

            // Store previous 16 helper values (these are just the initial input values)
            dest.sha256_is_input_round = j < 16 ? FF(1) : FF::zero();
            dest.sha256_helper_w0 = prev_helper_w0;
            dest.sha256_helper_w1 = prev_helper_w1;
            dest.sha256_helper_w2 = prev_helper_w2;
            dest.sha256_helper_w3 = prev_helper_w3;
            dest.sha256_helper_w4 = prev_helper_w4;
            dest.sha256_helper_w5 = prev_helper_w5;
            dest.sha256_helper_w6 = prev_helper_w6;
            dest.sha256_helper_w7 = prev_helper_w7;
            dest.sha256_helper_w8 = prev_helper_w8;
            dest.sha256_helper_w9 = prev_helper_w9;
            dest.sha256_helper_w10 = prev_helper_w10;
            dest.sha256_helper_w11 = prev_helper_w11;
            dest.sha256_helper_w12 = prev_helper_w12;
            dest.sha256_helper_w13 = prev_helper_w13;
            dest.sha256_helper_w14 = prev_helper_w14;
            dest.sha256_helper_w15 = prev_helper_w15;

            dest.sha256_round_count = FF(j);
            dest.sha256_rounds_remaining = FF(64 - j);
            dest.sha256_rounds_remaining_inv = 64 - j == 0 ? FF(0) : FF(64 - j).invert();

            if (j < 64) {
                // If j is lt 64, then we are performing compression rounds
                dest.sha256_perform_round = FF(1);
                dest.sha256_xor_sel = FF(2);
                dest.sha256_and_sel = FF::zero();

                // Computing w (message schedule) values for the current round
                dest.sha256_w = w[j];
                // Compute ror(w[j - 15], 7), where w[j - 15] == prev_helper_w1
                uint32_t rot_7 = ror(prev_helper_w1, 7);
                std::tie(dest.sha256_lhs_w_7, dest.sha256_rhs_w_7) = bit_limbs(prev_helper_w1, 7);
                dest.sha256_w_15_rotr_7 = FF(rot_7);

                // Compute ror(w[j - 15], 8), where w[j - 15] == prev_helper_w1
                uint32_t rot_18 = ror(prev_helper_w1, 18);
                std::tie(dest.sha256_lhs_w_18, dest.sha256_rhs_w_18) = bit_limbs(prev_helper_w1, 18);
                dest.sha256_w_15_rotr_18 = FF(rot_18);

                // Compute (w[j - 15] >> 3), where w[j - 15] == prev_helper_w1
                uint32_t shr_3 = prev_helper_w1 >> 3;
                std::tie(dest.sha256_lhs_w_3, dest.sha256_rhs_w_3) = bit_limbs(prev_helper_w1, 3);
                dest.sha256_w_15_rshift_3 = FF(shr_3);
                dest.sha256_w_15_rotr_7_xor_w_15_rotr_18 = FF(rot_7 ^ rot_18);

                // Compute s0 = ror(w[j - 15], 7) ^ ror(w[j - 15], 18) ^ (w[j - 15] >> 3);
                dest.sha256_w_s_0 = FF(rot_7 ^ rot_18 ^ shr_3);

                // Compute ror(w[j - 2], 17), where w[j - 2] == prev_helper_w14
                uint32_t rot_17 = ror(prev_helper_w14, 17);
                std::tie(dest.sha256_lhs_w_17, dest.sha256_rhs_w_17) = bit_limbs(prev_helper_w14, 17);
                dest.sha256_w_2_rotr_17 = FF(rot_17);

                // Compute ror(w[j - 2], 19), where w[j - 2] == prev_helper_w14
                uint32_t rot_19 = ror(prev_helper_w14, 19);
                std::tie(dest.sha256_lhs_w_19, dest.sha256_rhs_w_19) = bit_limbs(prev_helper_w14, 19);
                dest.sha256_w_2_rotr_19 = FF(rot_19);

                // Compute (w[j - 2] >> 10), where w[j - 2] == prev_helper_w14
                uint32_t shr_10 = prev_helper_w14 >> 10;
                std::tie(dest.sha256_lhs_w_10, dest.sha256_rhs_w_10) = bit_limbs(prev_helper_w14, 10);
                dest.sha256_w_2_rshift_10 = FF(shr_10);
                dest.sha256_w_2_rotr_17_xor_w_2_rotr_19 = FF(rot_17 ^ rot_19);

                // Compute s1 = ror(w[j - 2], 17) ^ ror(w[j - 2], 19) ^ (w[j - 2] >> 10);
                dest.sha256_w_s_1 = FF(rot_17 ^ rot_19 ^ shr_10);

                // Compute w[j]:= w[j-16] + s0 + w[j-7] + s1
                // The computation of w can overflow 32 bits so we perform modulo reduction
                uint64_t computed_w = prev_helper_w0 + static_cast<uint64_t>(dest.sha256_w_s_0) + prev_helper_w9 +
                                      static_cast<uint64_t>(dest.sha256_w_s_1);

                uint32_t computed_w_lhs = static_cast<uint32_t>(computed_w >> 32);
                uint32_t computed_w_rhs = static_cast<uint32_t>(computed_w);
                dest.sha256_computed_w_lhs = FF(computed_w_lhs);
                dest.sha256_computed_w_rhs = FF(computed_w_rhs);

                // Computing output state values for the round
                // Compute ror(e, 6)
                uint32_t rot_6 = ror(e, 6);
                std::tie(dest.sha256_lhs_e_6, dest.sha256_rhs_e_6) = bit_limbs(e, 6);
                dest.sha256_e_rotr_6 = FF(rot_6);

                // Compute ror(e, 11)
                uint32_t rot_11 = ror(e, 11);
                std::tie(dest.sha256_lhs_e_11, dest.sha256_rhs_e_11) = bit_limbs(e, 11);
                dest.sha256_e_rotr_11 = FF(rot_11);

                // Compute ror(e, 25)
                uint32_t rot_25 = ror(e, 25);
                std::tie(dest.sha256_lhs_e_25, dest.sha256_rhs_e_25) = bit_limbs(e, 25);
                dest.sha256_e_rotr_25 = FF(rot_25);
                dest.sha256_e_rotr_6_xor_e_rotr_11 = FF(rot_6 ^ rot_11);

                // Compute S1 := ror(e, 6U) ^ ror(e, 11U) ^ ror(e, 25U);
                dest.sha256_s_1 = FF(rot_6 ^ rot_11 ^ rot_25);

                // Compute ch := (e & f) ^ (~e & g);
                uint32_t e_and_f = e & f;
                dest.sha256_e_and_f = FF(e_and_f);
                uint32_t not_e = ~e;
                dest.sha256_not_e = FF(not_e);
                uint32_t not_e_and_g = not_e & g;
                dest.sha256_not_e_and_g = FF(not_e_and_g);
                uint32_t ch = e_and_f ^ not_e_and_g;
                dest.sha256_ch = FF(ch);

                // Compute ror(a, 2)
                uint32_t rot_a = ror(a, 2);
                std::tie(dest.sha256_lhs_a_2, dest.sha256_rhs_a_2) = bit_limbs(a, 2);
                dest.sha256_a_rotr_2 = FF(rot_a);

                // Compute ror(a, 13)
                uint32_t rot_13 = ror(a, 13);
                std::tie(dest.sha256_lhs_a_13, dest.sha256_rhs_a_13) = bit_limbs(a, 13);
                dest.sha256_a_rotr_13 = FF(rot_13);

                // Compute ror(a, 22)
                uint32_t rot_22 = ror(a, 22);
                std::tie(dest.sha256_lhs_a_22, dest.sha256_rhs_a_22) = bit_limbs(a, 22);
                dest.sha256_a_rotr_22 = FF(rot_22);
                dest.sha256_a_rotr_2_xor_a_rotr_13 = FF(rot_a ^ rot_13);

                // Compute S0 := ror(a, 2U) ^ ror(a, 13U) ^ ror(a, 22U);
                dest.sha256_s_0 = FF(rot_a ^ rot_13 ^ rot_22);

                // Compute maj := (a & b) ^ (a & c) ^ (b & c);
                uint32_t a_and_b = a & b;
                dest.sha256_a_and_b = FF(a_and_b);
                uint32_t a_and_c = a & c;
                dest.sha256_a_and_c = FF(a_and_c);
                uint32_t b_and_c = b & c;
                dest.sha256_b_and_c = FF(b_and_c);
                dest.sha256_a_and_b_xor_a_and_c = FF(a_and_b ^ a_and_c);
                uint32_t maj = a_and_b ^ a_and_c ^ b_and_c;
                dest.sha256_maj = FF(maj);

                dest.sha256_round_constant = round_constants[j];

                uint32_t S1 = ror(e, 6U) ^ ror(e, 11U) ^ ror(e, 25U);
                uint32_t temp1 = h + S1 + ch + round_constants[j] + w[j];
                uint32_t S0 = ror(a, 2U) ^ ror(a, 13U) ^ ror(a, 22U);
                uint32_t temp2 = S0 + maj;

                // Since temp 1 and temp 2 can overflow 32 bits, we perform modulo reduction
                uint64_t temp1_u64 = static_cast<uint64_t>(h) + static_cast<uint64_t>(S1) + static_cast<uint64_t>(ch) +
                                     static_cast<uint64_t>(round_constants[j]) + static_cast<uint64_t>(w[j]);
                uint64_t temp2_u64 = static_cast<uint64_t>(S0) + static_cast<uint64_t>(maj);
                uint64_t next_a = temp1_u64 + temp2_u64;
                uint32_t next_a_lhs = static_cast<uint32_t>(next_a >> 32);
                uint32_t next_a_rhs = static_cast<uint32_t>(next_a);
                dest.sha256_next_a_lhs = FF(next_a_lhs);
                dest.sha256_next_a_rhs = FF(next_a_rhs);

                ASSERT(next_a_rhs == temp1 + temp2);

                // Handle modulo 2^32 for e
                uint64_t next_e = static_cast<uint64_t>(d) + temp1_u64;
                uint32_t next_e_lhs = static_cast<uint32_t>(next_e >> 32);
                uint32_t next_e_rhs = static_cast<uint32_t>(next_e);
                dest.sha256_next_e_lhs = FF(next_e_lhs);
                dest.sha256_next_e_rhs = FF(next_e_rhs);

                ASSERT(next_e_rhs == d + temp1);

                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;

            } else {
                // At this point we have finished our 64 rounds and we need to add the initial state to the output to
                // get the final hash

                dest.sha256_latch = FF(1);

                // All of the output additions can overflow 32 bits so we perform modulo reduction
                uint64_t out_a = static_cast<uint64_t>(a) + static_cast<uint64_t>(src.state[0]);
                uint32_t out_a_lhs = static_cast<uint32_t>(out_a >> 32);
                uint32_t out_a_rhs = static_cast<uint32_t>(out_a);
                dest.sha256_output_a_lhs = FF(out_a_lhs);
                dest.sha256_output_a_rhs = FF(out_a_rhs);
                uint64_t out_b = static_cast<uint64_t>(b) + static_cast<uint64_t>(src.state[1]);
                uint32_t out_b_lhs = static_cast<uint32_t>(out_b >> 32);
                uint32_t out_b_rhs = static_cast<uint32_t>(out_b);
                dest.sha256_output_b_lhs = FF(out_b_lhs);
                dest.sha256_output_b_rhs = FF(out_b_rhs);
                uint64_t out_c = static_cast<uint64_t>(c) + static_cast<uint64_t>(src.state[2]);
                uint32_t out_c_lhs = static_cast<uint32_t>(out_c >> 32);
                uint32_t out_c_rhs = static_cast<uint32_t>(out_c);
                dest.sha256_output_c_lhs = FF(out_c_lhs);
                dest.sha256_output_c_rhs = FF(out_c_rhs);
                uint64_t out_d = static_cast<uint64_t>(d) + static_cast<uint64_t>(src.state[3]);
                uint32_t out_d_lhs = static_cast<uint32_t>(out_d >> 32);
                uint32_t out_d_rhs = static_cast<uint32_t>(out_d);
                dest.sha256_output_d_lhs = FF(out_d_lhs);
                dest.sha256_output_d_rhs = FF(out_d_rhs);
                uint64_t out_e = static_cast<uint64_t>(e) + static_cast<uint64_t>(src.state[4]);
                uint32_t out_e_lhs = static_cast<uint32_t>(out_e >> 32);
                uint32_t out_e_rhs = static_cast<uint32_t>(out_e);
                dest.sha256_output_e_lhs = FF(out_e_lhs);
                dest.sha256_output_e_rhs = FF(out_e_rhs);
                uint64_t out_f = static_cast<uint64_t>(f) + static_cast<uint64_t>(src.state[5]);
                uint32_t out_f_lhs = static_cast<uint32_t>(out_f >> 32);
                uint32_t out_f_rhs = static_cast<uint32_t>(out_f);
                dest.sha256_output_f_lhs = FF(out_f_lhs);
                dest.sha256_output_f_rhs = FF(out_f_rhs);
                uint64_t out_g = static_cast<uint64_t>(g) + static_cast<uint64_t>(src.state[6]);
                uint32_t out_g_lhs = static_cast<uint32_t>(out_g >> 32);
                uint32_t out_g_rhs = static_cast<uint32_t>(out_g);
                dest.sha256_output_g_lhs = FF(out_g_lhs);
                dest.sha256_output_g_rhs = FF(out_g_rhs);
                uint64_t out_h = static_cast<uint64_t>(h) + static_cast<uint64_t>(src.state[7]);
                uint32_t out_h_lhs = static_cast<uint32_t>(out_h >> 32);
                uint32_t out_h_rhs = static_cast<uint32_t>(out_h);
                dest.sha256_output_h_lhs = FF(out_h_lhs);
                dest.sha256_output_h_rhs = FF(out_h_rhs);

                // We add the state here so we can do a final sanity check
                a += src.state[0];
                b += src.state[1];
                c += src.state[2];
                d += src.state[3];
                e += src.state[4];
                f += src.state[5];
                g += src.state[6];
                h += src.state[7];
            }

            // This is the sliding window updates for the last 16 values of the message schedule
            prev_helper_w0 = static_cast<uint32_t>(dest.sha256_helper_w1);
            prev_helper_w1 = static_cast<uint32_t>(dest.sha256_helper_w2);
            prev_helper_w2 = static_cast<uint32_t>(dest.sha256_helper_w3);
            prev_helper_w3 = static_cast<uint32_t>(dest.sha256_helper_w4);
            prev_helper_w4 = static_cast<uint32_t>(dest.sha256_helper_w5);
            prev_helper_w5 = static_cast<uint32_t>(dest.sha256_helper_w6);
            prev_helper_w6 = static_cast<uint32_t>(dest.sha256_helper_w7);
            prev_helper_w7 = static_cast<uint32_t>(dest.sha256_helper_w8);
            prev_helper_w8 = static_cast<uint32_t>(dest.sha256_helper_w9);
            prev_helper_w9 = static_cast<uint32_t>(dest.sha256_helper_w10);
            prev_helper_w10 = static_cast<uint32_t>(dest.sha256_helper_w11);
            prev_helper_w11 = static_cast<uint32_t>(dest.sha256_helper_w12);
            prev_helper_w12 = static_cast<uint32_t>(dest.sha256_helper_w13);
            prev_helper_w13 = static_cast<uint32_t>(dest.sha256_helper_w14);
            prev_helper_w14 = static_cast<uint32_t>(dest.sha256_helper_w15);
            prev_helper_w15 = static_cast<uint32_t>(dest.sha256_w);
        }

        // Sanity check
        std::array<uint32_t, 8> output{ a, b, c, d, e, f, g, h };
        ASSERT(output == src.output);
    }
}

} // namespace bb::avm_trace

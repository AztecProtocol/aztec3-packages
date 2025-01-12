#include "barretenberg/vm/avm/trace/gadgets/fixed_sha256_constants.hpp"
#include "barretenberg/vm/aztec_constants.hpp"
#include <cstdint>
#include <unordered_map>

namespace bb::avm_trace {

namespace {

constexpr auto make_entry(uint8_t round, uint32_t constant)
{
    return FixedSha256ConstantsTable::Sha256ConstantsRow{ .round_index = round, .round_constant = constant };
}

const std::array<FixedSha256ConstantsTable::Sha256ConstantsRow, 64> SHA256_ROUND_CONSTANTS = {
    make_entry(0, 0x428a2f98),  make_entry(1, 0x71374491),  make_entry(2, 0xb5c0fbcf),  make_entry(3, 0xe9b5dba5),
    make_entry(4, 0x3956c25b),  make_entry(5, 0x59f111f1),  make_entry(6, 0x923f82a4),  make_entry(7, 0xab1c5ed5),
    make_entry(8, 0xd807aa98),  make_entry(9, 0x12835b01),  make_entry(10, 0x243185be), make_entry(11, 0x550c7dc3),
    make_entry(12, 0x72be5d74), make_entry(13, 0x80deb1fe), make_entry(14, 0x9bdc06a7), make_entry(15, 0xc19bf174),
    make_entry(16, 0xe49b69c1), make_entry(17, 0xefbe4786), make_entry(18, 0x0fc19dc6), make_entry(19, 0x240ca1cc),
    make_entry(20, 0x2de92c6f), make_entry(21, 0x4a7484aa), make_entry(22, 0x5cb0a9dc), make_entry(23, 0x76f988da),
    make_entry(24, 0x983e5152), make_entry(25, 0xa831c66d), make_entry(26, 0xb00327c8), make_entry(27, 0xbf597fc7),
    make_entry(28, 0xc6e00bf3), make_entry(29, 0xd5a79147), make_entry(30, 0x06ca6351), make_entry(31, 0x14292967),
    make_entry(32, 0x27b70a85), make_entry(33, 0x2e1b2138), make_entry(34, 0x4d2c6dfc), make_entry(35, 0x53380d13),
    make_entry(36, 0x650a7354), make_entry(37, 0x766a0abb), make_entry(38, 0x81c2c92e), make_entry(39, 0x92722c85),
    make_entry(40, 0xa2bfe8a1), make_entry(41, 0xa81a664b), make_entry(42, 0xc24b8b70), make_entry(43, 0xc76c51a3),
    make_entry(44, 0xd192e819), make_entry(45, 0xd6990624), make_entry(46, 0xf40e3585), make_entry(47, 0x106aa070),
    make_entry(48, 0x19a4c116), make_entry(49, 0x1e376c08), make_entry(50, 0x2748774c), make_entry(51, 0x34b0bcb5),
    make_entry(52, 0x391c0cb3), make_entry(53, 0x4ed8aa4a), make_entry(54, 0x5b9cca4f), make_entry(55, 0x682e6ff3),
    make_entry(56, 0x748f82ee), make_entry(57, 0x78a5636f), make_entry(58, 0x84c87814), make_entry(59, 0x8cc70208),
    make_entry(60, 0x90befffa), make_entry(61, 0xa4506ceb), make_entry(62, 0xbef9a3f7), make_entry(63, 0xc67178f2)

};

} // namespace

size_t FixedSha256ConstantsTable::size() const
{
    return 64;
}

const FixedSha256ConstantsTable::Sha256ConstantsRow& FixedSha256ConstantsTable::at(size_t o) const
{
    return SHA256_ROUND_CONSTANTS.at(o);
}

// Singleton.
const FixedSha256ConstantsTable& FixedSha256ConstantsTable::get()
{
    static FixedSha256ConstantsTable table;
    return table;
}

} // namespace bb::avm_trace

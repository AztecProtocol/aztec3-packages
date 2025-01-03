use std::hash::poseidon;
use std::verify_proof_with_type;

fn main(
    verification_key: [Field; 114],
    proof: [Field; 93],
    public_inputs: [Field; 1],
    key_hash: Field,
) {
    for _ in 0..250 {
        let _ = poseidon::bn254::hash_1([1]);
    }

    verify_proof_with_type(
        verification_key,
        proof,
        public_inputs,
        key_hash,
        0 // PROOF_TYPE_PLONK
    );
}

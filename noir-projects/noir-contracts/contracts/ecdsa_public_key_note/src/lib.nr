use dep::aztec::prelude::{NoteHeader, NoteInterface, NullifiableNote, PrivateContext};

use dep::aztec::{
    note::utils::compute_note_hash_for_nullify, keys::getters::{get_nsk_app, get_public_keys},
    protocol_types::{address::AztecAddress, constants::{GENERATOR_INDEX__NOTE_NULLIFIER, GENERATOR_INDEX__NOTE_HASH}, hash::poseidon2_hash_with_separator, utils::arrays::array_concat},
    macros::notes::note_custom_interface
};

global ECDSA_PUBLIC_KEY_NOTE_LEN: u32 = 5;

// Stores an ECDSA public key composed of two 32-byte elements
// TODO: Do we need to include a nonce, in case we want to read/nullify/recreate with the same pubkey value?
#[note_custom_interface]
pub struct EcdsaPublicKeyNote {
    x: [u8; 32],
    y: [u8; 32],
    owner: AztecAddress,
}

impl NoteInterface<ECDSA_PUBLIC_KEY_NOTE_LEN> for EcdsaPublicKeyNote {
    // Cannot use the automatic packing since x and y don't fit. Pack the note as 5 fields where:
    // [0] = x[0..31] (upper bound excluded)
    // [1] = x[31]
    // [2] = y[0..31]
    // [3] = y[31]
    // [4] = owner
    fn pack_content(self) -> [Field; ECDSA_PUBLIC_KEY_NOTE_LEN] {
        let mut x: Field = 0;
        let mut y: Field = 0;
        let mut mul: Field = 1;

        for i in 1..32 {
            let byte_x: Field = self.x[31 - i] as Field;
            x = x + (byte_x * mul);
            let byte_y: Field = self.y[31 - i] as Field;
            y = y + (byte_y * mul);
            mul *= 256;
        }

        let last_x = self.x[31] as Field;
        let last_y = self.y[31] as Field;

        [x, last_x, y, last_y, self.owner.to_field()]
    }

    // Cannot use the automatic unpacking for the aforementioned reasons
    fn unpack_content(packed_content: [Field; ECDSA_PUBLIC_KEY_NOTE_LEN]) -> EcdsaPublicKeyNote {
        let mut x: [u8; 32] = [0; 32];
        let mut y: [u8; 32] = [0; 32];

        let part_x:[u8; 32] = packed_content[0].to_be_bytes();
        for i in 0..31 {
            x[i] = part_x[i + 1];
        }
        x[31] = packed_content[1].to_be_bytes::<32>()[31];

        let part_y:[u8; 32] = packed_content[2].to_be_bytes();
        for i in 0..31 {
            y[i] = part_y[i + 1];
        }
        y[31] = packed_content[3].to_be_bytes::<32>()[31];

        EcdsaPublicKeyNote { x, y, owner: AztecAddress::from_field(packed_content[4]), header: NoteHeader::empty() }
    }

    fn get_note_type_id() -> Field {
        // randomly chosen note type id --> has to fit within 7 bits
        76
    }

    fn get_header(self) -> NoteHeader {
        self.header
    }

    fn set_header(&mut self, header: NoteHeader) {
        self.header = header;
    }

    fn compute_note_hash(self, storage_slot: Field) -> Field {
        // We use Poseidon2 instead of multi-scalar multiplication (MSM) here since this is not a partial note
        // and therefore does not require MSM's additive homomorphism property. Additionally, Poseidon2 uses fewer
        // constraints.
        let inputs = array_concat(self.pack_content(), [storage_slot]);
        poseidon2_hash_with_separator(inputs, GENERATOR_INDEX__NOTE_HASH)
    }
}

impl NullifiableNote for EcdsaPublicKeyNote {

    fn compute_nullifier(self, context: &mut PrivateContext, note_hash_for_nullify: Field) -> Field {
        let owner_npk_m_hash = get_public_keys(self.owner).npk_m.hash();
        let secret = context.request_nsk_app(owner_npk_m_hash);
        poseidon2_hash_with_separator(
            [
            note_hash_for_nullify,
            secret
        ],
            GENERATOR_INDEX__NOTE_NULLIFIER as Field
        )
    }

    unconstrained fn compute_nullifier_without_context(self, storage_slot: Field) -> Field {
        let note_hash_for_nullify = compute_note_hash_for_nullify(self, storage_slot);
        let owner_npk_m_hash = get_public_keys(self.owner).npk_m.hash();
        let secret = get_nsk_app(owner_npk_m_hash);
        poseidon2_hash_with_separator(
            [
            note_hash_for_nullify,
            secret
        ],
            GENERATOR_INDEX__NOTE_NULLIFIER as Field
        )
    }
}

impl EcdsaPublicKeyNote {
    pub fn new(x: [u8; 32], y: [u8; 32], owner: AztecAddress) -> Self {
        EcdsaPublicKeyNote { x, y, owner, header: NoteHeader::empty() }
    }
}

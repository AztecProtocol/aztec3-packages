use crate::state_vars::shared_mutable::{
    scheduled_delay_change::{SCHEDULED_DELAY_CHANGE_PCKD_LEN, ScheduledDelayChange},
    scheduled_value_change::ScheduledValueChange,
};
use dep::protocol_types::traits::Packable;
use std::meta::derive;

/// SharedMutableValues is just a wrapper around ScheduledValueChange and ScheduledDelayChange that then allows us
/// to wrap both of these values in WithHash. WithHash allows for efficient read of values in private.
///
/// Note that the WithHash optimization does not work in public (due to there being no unconstrained). But we also want
/// to be able to read the values efficiently in public and we want to be able to read each value separately. For that
/// reason we expose `get_delay_change_storage_slot` and `get_value_change_storage_slot` which point to the correct
/// location in the storage. This is "hacky" as we pack and store the values together but there is no way around it.
#[derive(Eq)]
pub(crate) struct SharedMutableValues<T, let INITIAL_DELAY: u32> {
    svc: ScheduledValueChange<T>,
    sdc: ScheduledDelayChange<INITIAL_DELAY>,
}

impl<T, let INITIAL_DELAY: u32> SharedMutableValues<T, INITIAL_DELAY> {
    pub(crate) fn new(
        svc: ScheduledValueChange<T>,
        sdc: ScheduledDelayChange<INITIAL_DELAY>,
    ) -> Self {
        SharedMutableValues { svc, sdc }
    }

    pub fn get_delay_change_storage_slot(shared_mutable_storage_slot: Field) -> Field {
        shared_mutable_storage_slot
    }

    pub fn get_value_change_storage_slot(shared_mutable_storage_slot: Field) -> Field {
        shared_mutable_storage_slot + (SCHEDULED_DELAY_CHANGE_PCKD_LEN as Field)
    }
}

global TWO_POW_32: Field = 2.pow_32(32);
global TWO_POW_8: Field = 2.pow_32(8);

// We pack to `2 * N` fields because ScheduledValueChange contains T twice
// The first field contains both ScheduledDelayChange and block_of_change from ScheduledValueChange
impl<T, let INITIAL_DELAY: u32, let N: u32> Packable<2 * N + 1> for SharedMutableValues<T, INITIAL_DELAY>
where
    T: Packable<N>,
{
    fn pack(self) -> [Field; 2 * N + 1] {
        let mut result = [0; 2 * N + 1];

        // We pack sdc.pre, sdc.post, sdc.block_of_change and svc.block_of_change into a single field.
        // [ sdc.pre_inner: u32 | sdc.pre_is_some: u8 | sdc.post_inner: u32 | sdc.post_is_some: u8 | sdc.block_of_change: u32 | svc.block_of_change: u32 ]
        result[0] = self.svc.block_of_change as Field
            + ((self.sdc.block_of_change as Field) * 2.pow_32(32))
            + ((self.sdc.post.is_some() as Field) * 2.pow_32(64))
            + ((self.sdc.post.unwrap_unchecked() as Field) * 2.pow_32(72))
            + ((self.sdc.pre.is_some() as Field) * 2.pow_32(104))
            + ((self.sdc.pre.unwrap_unchecked() as Field) * 2.pow_32(112));

        // Pack the pre and post values from ScheduledValueChange
        let svc_pre_packed = self.svc.pre.pack();
        let svc_post_packed = self.svc.post.pack();
        for i in 0..N {
            result[i + 1] = svc_pre_packed[i];
            result[i + 1 + N] = svc_post_packed[i];
        }
        result
    }

    fn unpack(fields: [Field; 2 * N + 1]) -> Self {
        // Unpack the first field containing sdc and svc block_of_change values
        let svc_block_of_change = fields[0] as u32;
        let mut tmp = (fields[0] - svc_block_of_change as Field) / TWO_POW_32;

        let sdc_block_of_change = tmp as u32;

        tmp = (tmp - sdc_block_of_change as Field) / TWO_POW_32;
        let sdc_post_is_some = tmp as bool;

        tmp = (tmp - sdc_post_is_some as Field) / TWO_POW_8;
        let sdc_post_inner = tmp as u32;

        tmp = (tmp - sdc_post_inner as Field) / TWO_POW_32;
        let sdc_pre_is_some = tmp as bool;

        tmp = (tmp - sdc_pre_is_some as Field) / TWO_POW_8;
        let sdc_pre_inner = tmp as u32;

        // Unpack the pre and post values for ScheduledValueChange
        let mut svc_pre_packed = [0; N];
        let mut svc_post_packed = [0; N];
        for i in 0..N {
            svc_pre_packed[i] = fields[i + 1];
            svc_post_packed[i] = fields[i + N + 1];
        }

        SharedMutableValues {
            svc: ScheduledValueChange {
                pre: T::unpack(svc_pre_packed),
                post: T::unpack(svc_post_packed),
                block_of_change: svc_block_of_change,
            },
            sdc: ScheduledDelayChange {
                pre: if sdc_pre_is_some {
                    Option::some(sdc_pre_inner)
                } else {
                    Option::none()
                },
                post: if sdc_post_is_some {
                    Option::some(sdc_post_inner)
                } else {
                    Option::none()
                },
                block_of_change: sdc_block_of_change,
            },
        }
    }
}

mod test {
    use crate::state_vars::shared_mutable::{
        scheduled_delay_change::ScheduledDelayChange, scheduled_value_change::ScheduledValueChange,
        shared_mutable_values::SharedMutableValues,
    };
    use crate::test::mocks::mock_struct::MockStruct;

    #[test]
    unconstrained fn test_pack_unpack() {
        let svc =
            ScheduledValueChange::new(MockStruct { a: 1, b: 2 }, MockStruct { a: 3, b: 4 }, 5);
        let sdc = ScheduledDelayChange::new(Option::some(6), Option::some(7), 8);
        let shared_mutable_values = SharedMutableValues::new(svc, sdc);
        let packed = shared_mutable_values.pack();
        let unpacked = SharedMutableValues::unpack(packed);
        assert_eq(shared_mutable_values, unpacked);
    }
}

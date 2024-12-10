use crate::Test;

#[test]
unconstrained fn test_storage_slot_allocation() {
    // This tests that sufficient storage slots are assigned to each state variable so that they do not interfere with
    // one another. The space a state variable needs is determined by the N value in its implementation of the Storage
    // trait. Most state variables bind N to the serialization length of the type they hold.
    //
    // This is the storage declaration:
    //
    // #[storage]
    // struct Storage<Context> {
    //     example_constant: PrivateImmutable<TestNote, Context>,
    //     example_set: PrivateSet<TestNote, Context>,
    //     example_struct: PrivateImmutable<ExampleStruct, Context>,
    //     example_struct_in_map: Map<AztecAddress, PrivateImmutable<ExampleStruct, Context>, Context>,
    //     another_example_struct: PrivateImmutable<ExampleStruct, Context>,
    // }

    // We can't directly see how many slots are allocated to each variable, but we can look at the slot increments for
    // each and deduct the allocation size based off of that. In other words, given a struct with two members a and b,
    // the number of slots allocated to a will be b.storage_slot - a.storage_slot.

    // The first slot is always 1.
    let mut expected_slot = 1;
    assert_eq(Test::storage_layout().example_constant.slot, expected_slot);

    // Even though example_constant holds TestNote, which serializes to a length larger than 1, notes always reserve a
    // single slot.
    expected_slot += 1;
    assert_eq(Test::storage_layout().example_set.slot, expected_slot);

    // example_set also held a note, so it should have only allocated a single slot.
    expected_slot += 1;
    assert_eq(Test::storage_layout().example_struct.slot, expected_slot);

    // example_struct allocates 5 slots because it is not a note and it's serialization length is 5.
    expected_slot += 5;
    assert_eq(Test::storage_layout().example_struct_in_map.slot, expected_slot);

    // example_struct_in_map should allocate a single note because it's a map, regardless of whatever it holds. The Map
    // type is going to deal with its own dynamic allocation based on keys
    expected_slot += 1;
    assert_eq(Test::storage_layout().another_example_struct.slot, expected_slot);
}

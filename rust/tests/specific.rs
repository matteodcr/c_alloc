//! Tests where the allocator is used in specific locations, instead of globally. Due to the way it
//! is implemented in C, 2 allocators cannot coexist as all "instances" actually use the same
//! backing memory. If our C init/allocating/freeing functions took a pointer to a byte array as
//! their first parameter, it would very much be possible to have multiple _different_ allocators
//! at the same time.

#![feature(allocator_api)]

use info3_allocateur::Info3Allocateur;

#[test]
fn alloc_vec() {
    let allocator = Info3Allocateur::default();

    let mut list = Vec::new_in(allocator);
    list.resize(99, 42);
    list.push(0);
    assert_eq!(&list[98..], &[42, 0]);
}

/// Une [Box] correspond à un pointeur vers des données allouées sur le tas. Comme pour un [Vec] ou
/// une [String], elle est libérée implicitement et automatiquement lorsqu'elle sort de portée.
#[test]
fn alloc_box() {
    let mut boxed_number = Box::new_in(40, Info3Allocateur::default());
    *boxed_number += 2;
    assert_eq!(*boxed_number, 42);
}

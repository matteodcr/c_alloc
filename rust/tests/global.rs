//! Tests where the global Rust allocator is substituted with our own, having effects in all
//! allocations.

use info3_allocateur::Info3AllocateurGlobal;

#[global_allocator]
static ALLOCATOR: Info3AllocateurGlobal = Info3AllocateurGlobal;

#[test]
fn alloc_global_string() {
    let mut foo_on_heap = String::from("foo");
    foo_on_heap += "barr";
    foo_on_heap.remove(6);
    assert_eq!(foo_on_heap, "foobar");
}

#[test]
fn alloc_global_vec() {
    let vec = (1..=42).collect::<Vec<_>>();
    assert_eq!(vec.last(), Some(&42));
}

use info3_allocateur::{FitFunction, Info3AllocateurGlobal};

#[global_allocator]
static ALLOCATOR: Info3AllocateurGlobal = Info3AllocateurGlobal;

#[test]
fn alloc_fit() {
    ALLOCATOR.set_fit_function(FitFunction::Worst);
    assert_eq!(*Box::new(42), 42);
}

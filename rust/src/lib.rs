#![feature(allocator_api, extern_types, ptr_metadata)]

mod fit;

use crate::fit::FitFn;
pub use crate::fit::FitFunction;
use std::alloc::{AllocError, Allocator, GlobalAlloc, Layout};
use std::ops::Deref;
use std::ptr::{null_mut, NonNull};

extern "C" {
    fn get_memory_size() -> usize;
    fn get_memory_adr() -> *mut u8;
    fn mem_init(memory: *mut u8, size: usize);
    fn mem_fit(f: FitFn);

    fn mem_alloc(size: usize) -> *mut u8;
    fn mem_free(ptr: *mut u8);
}

/// Non-global allocator
///
/// Can be used for a specific instance of a type, such as with:
///  * [Vec] → [`Vec::new_in(allocator: A)`][Vec::new_in]
///  * [Box] → [`Box::new_in(value: T, allocator: A)`][Box::new_in]
#[derive(Clone, Copy)]
pub struct Info3Allocateur([(); 0]);

lazy_static::lazy_static! {
    static ref INSTANCE: Info3Allocateur = {
        unsafe {
            mem_init(get_memory_adr(), get_memory_size());
        }

        Info3Allocateur([])
    };
}

impl Default for Info3Allocateur {
    fn default() -> Self {
        *INSTANCE
    }
}

impl Info3Allocateur {
    pub fn size(self) -> usize {
        unsafe { get_memory_size() }
    }

    /// Use an alternative fit function
    ///
    /// By default, allocators use the [`FitFunction::First`] strategy
    pub fn set_fit_function(self, fit: FitFunction) {
        unsafe {
            mem_fit(fit.to_fn());
        }
    }
}

unsafe impl Allocator for Info3Allocateur {
    fn allocate(&self, layout: Layout) -> Result<NonNull<[u8]>, AllocError> {
        let (size, align) = (layout.size(), layout.align());
        assert!(align <= 16);

        let ptr = unsafe { mem_alloc(size) };
        assert_eq!(ptr as usize % 16, 0);
        NonNull::new(ptr)
            .map(|non_null| NonNull::from_raw_parts(non_null.cast(), size))
            .ok_or(AllocError)
    }

    unsafe fn deallocate(&self, ptr: NonNull<u8>, _layout: Layout) {
        mem_free(ptr.as_ptr());
    }
}

/// Global allocator, can be used with
/// [`#[global_allocator]`][std::alloc#the-global_allocator-attribute]
pub struct Info3AllocateurGlobal;

unsafe impl GlobalAlloc for Info3AllocateurGlobal {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        INSTANCE
            .allocate(layout)
            .map(|non_null| non_null.as_ptr().cast())
            .unwrap_or(null_mut())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        if let Some(ptr) = NonNull::new(ptr) {
            INSTANCE.deallocate(ptr, layout);
        }
    }
}

impl Deref for Info3AllocateurGlobal {
    type Target = Info3Allocateur;

    fn deref(&self) -> &Self::Target {
        &*INSTANCE
    }
}

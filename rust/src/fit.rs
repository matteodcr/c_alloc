pub type FitFn = unsafe extern "C" fn(*const Fb, usize) -> *const Fb;

extern "C" {
    /// Opaque `struct fb` type
    pub type Fb;

    fn mem_fit_first(head: *const Fb, size: usize) -> *const Fb;
    fn mem_fit_best(head: *const Fb, size: usize) -> *const Fb;
    fn mem_fit_worst(head: *const Fb, size: usize) -> *const Fb;
}

/// Fit functions provided by the C implementation
#[derive(Clone, Copy, Debug, Eq, PartialEq)]
#[non_exhaustive]
#[repr(usize)]
pub enum FitFunction {
    /// Finds the first free space that is big enough for the required size to fit
    First,

    /// Finds the best free space, that is the space that leaves the least amount of residue
    Best,

    /// Finds the worst free space, that is space that leaves the biggest residue
    Worst,
}

impl Default for FitFunction {
    fn default() -> Self {
        FitFunction::First
    }
}

impl FitFunction {
    pub(crate) fn to_fn(self) -> FitFn {
        match self {
            Self::First => mem_fit_first,
            Self::Best => mem_fit_best,
            Self::Worst => mem_fit_worst,
        }
    }
}

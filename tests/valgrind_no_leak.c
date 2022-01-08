#include <assert.h>
#include "../malloc_stub.h"

int main() {
    assert(malloc_info3 == malloc);

    void* ptr = malloc(42);
    // Pointer won't be leaked
    free(ptr);
}

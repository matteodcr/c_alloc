#include <assert.h>
#include "../malloc_stub.h"

/**
 * Test contrôle: on s'assure que valgrind détecte bien les fuites de mémoire quand il y en a
 */
int main() {
    assert(malloc_info3 == malloc);

    void* ptr1 = malloc(42);
    void* ptr2 = malloc(42);

    free(ptr1);

    // We leak one of the pointers pointer
    (void) ptr2;
}

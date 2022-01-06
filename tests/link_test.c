#include <assert.h>
# include "stdio.h"
# include "../malloc_stub.h"
# include "../mem.h"


int main() {
    void *ptr = malloc(8);
    if (mem_free(ptr) != true)
        return 1;

    ptr = malloc(8);
    size_t *fb_size = (size_t *) (ptr - 16);
    *fb_size = 37; // 16 > 8

    if (mem_free(ptr) == false)
        if (LAST_ERROR == FB_LINK_BROKEN)
            return 0;

    return 1;
}

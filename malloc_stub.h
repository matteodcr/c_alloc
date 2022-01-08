#ifndef __MALLOC_STUB_H__
#define __MALLOC_STUB_H__
#include <stdlib.h>
/// Cette variable permet de s'assurer qu'on utilise bien notre allocateur dans les tests, et pas celui de base.
/// Il suffit de vérifier que ce pointeur est le même que la fonction `malloc`.
extern void* malloc_info3;
void *malloc(size_t s);
void *calloc(size_t count, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);
#endif

#ifndef __MEM_H
#define __MEM_H
#include <stddef.h>
#include <stdbool.h>

#define MEM_GET_SIZE_ERROR ((size_t) -1)

extern enum error_code {
    NOT_ALLOCATED,
    FB_LINK_BROKEN,
} LAST_ERROR;

struct fb;

/* fonctions principales de l'allocateur */
void mem_init(void* mem, size_t taille);
void* mem_alloc(size_t size);
bool mem_free(void* ptr);
size_t mem_get_size(void *zone);
void* mem_realloc(void *old, size_t new_size);

/* Itération sur le contenu de l'allocateur */
/* nécessaire pour le mem_shell */
void mem_show(void (*print)(void *adr, size_t size, int free));

/* Choix de la stratégie et strategies usuelles */
/* Si vous avez le temps... */
typedef struct fb* (mem_fit_function_t)(struct fb*, size_t);

void mem_fit(mem_fit_function_t*);
mem_fit_function_t mem_fit_first;
mem_fit_function_t mem_fit_worst;
mem_fit_function_t mem_fit_best;

#endif

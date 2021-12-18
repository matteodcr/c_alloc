/* On inclut l'interface publique */
#include "mem.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

/* Définition de l'alignement recherché
 * Avec gcc, on peut utiliser __BIGGEST_ALIGNMENT__
 * sinon, on utilise 16 qui conviendra aux plateformes qu'on cible
 */
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

/* structure placée au début de la zone de l'allocateur

   Elle contient toutes les variables globales nécessaires au
   fonctionnement de l'allocateur

   Elle peut bien évidemment être complétée
*/
struct allocator_header {
    size_t memory_size;
    mem_fit_function_t *fit;
};


/* La seule variable globale autorisée
 * On trouve à cette adresse le début de la zone à gérer
 * (et une structure 'struct allocator_header)
 */
static void *memory_addr;

static inline void *get_system_memory_addr() {
    return memory_addr;
}

static inline struct allocator_header *get_header() {
    struct allocator_header *h;
    h = get_system_memory_addr();
    return h;
}

static inline struct fb *get_fb_head() {
    return (struct fb *) (get_system_memory_addr() + sizeof(struct allocator_header));
}

static inline size_t get_system_memory_size() {
    return get_header()->memory_size;
}

void mem_fit(mem_fit_function_t *f) {
    get_header()->fit = f;
}

void mem_size(size_t size) {
    get_header()->memory_size = size;
}

// Voir nos schémas pour comprendre notre utilisation de fb
struct fb {
    size_t size;
    struct fb *next;
};


void mem_init(void *mem, size_t taille) {
    // On met en place fb
    memory_addr = mem;
    //On met en place allocator header
    *(size_t *) memory_addr = taille;
    /* On vérifie qu'on a bien enregistré les infos et qu'on
     * sera capable de les récupérer par la suite
     */
    assert(mem == get_system_memory_addr());
    assert(taille == get_system_memory_size());

    struct fb *head = get_fb_head();
    head->size = taille - sizeof(struct allocator_header);
    head->next = NULL;

    mem_fit(&mem_fit_first);
}

void mem_show(void (*print)(void *, size_t, int)) {
    for (struct fb *free_zone = get_fb_head(); free_zone; free_zone = free_zone->next) {
        struct fb *next = free_zone->next;
        print(free_zone, free_zone->size, true);
        if (next) {
            print(((void *) free_zone) + free_zone->size, ((void *) next) - ((void *) free_zone) - free_zone->size,
                  false);
        }
    }
}


void align_by_8(size_t *val) {
    size_t x = *val % (size_t) 8;
    *val += x ? 8 - x : 0;
}


void *mem_alloc(size_t size) {
    if (size == 0) {
        // On peut retourner n'importe quel pointeur mais pour éviter les UB, il faut qu'il soit non-nul et aligné à la
        // taille d'un registre. Ce cas sera géré dans `mem_free`.
        return get_fb_head();
    }

    // On aligne par 8, c'est plus prudent car cela garantit que tous les fb sont alignés (le contraire serait
    // potentiellement problématique sur certaines architectures).
    align_by_8(&size);

    struct fb *fb = get_header()->fit(get_fb_head(), size);

    if (fb) {
        struct fb *new_fb = ((void *) fb) + sizeof(struct fb) + size;
        new_fb->size = fb->size - size;
        new_fb->next = fb->next;

        fb->size = sizeof(struct fb);
        fb->next = ((void *) fb) + sizeof(struct fb) + size;

        return (void *) fb + sizeof(struct fb);
    } else {
        return NULL;
    }
}


void mem_free(void *mem) {
    if (mem == get_fb_head()) {
        // Special case of `mem_alloc(0)`
        return;
    }

    for (struct fb *cell = get_fb_head(); cell; cell = cell->next) {
        if (((void *) cell) + cell->size == mem) {
            cell->size = (size_t) ((void *) cell->next - ((void *) cell)) + cell->next->size;
            cell->next = cell->next->next;
            return;
        }
    }

    exit(1);
    // TODO: aïe aïe aïe
}


struct fb *mem_fit_first(struct fb *list, size_t size) {
    for (struct fb *cell = list; cell; cell = cell->next) {
        ssize_t free_space = (ssize_t) cell->size - (ssize_t) 2 * (ssize_t) sizeof(struct fb);
        if ((ssize_t) size <= free_space) {
            return cell;
        }
    }

    return NULL;
}

/* Fonction à faire dans un second temps
 * - utilisée par realloc() dans malloc_stub.c
 * - nécessaire pour remplacer l'allocateur de la libc
 * - donc nécessaire pour 'make test_ls'
 * Lire malloc_stub.c pour comprendre son utilisation
 * (ou en discuter avec l'enseignant)
 */
size_t mem_get_size(void *zone) {
    /* zone est une adresse qui a été retournée par mem_alloc() */

    /* la valeur retournée doit être la taille maximale que
     * l'utilisateur peut utiliser dans cette zone */
    return 0;
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb *mem_fit_best(struct fb *list, size_t size) {
    return NULL;
}

struct fb *mem_fit_worst(struct fb *list, size_t size) {
    return NULL;
}

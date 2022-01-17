/* On inclut l'interface publique */
#include "mem.h"
#include "common.h"

#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <valgrind/valgrind.h>

/* Définition de l'alignement recherché
 * Avec gcc, on peut utiliser __BIGGEST_ALIGNMENT__
 * sinon, on utilise 16 qui conviendra aux plateformes qu'on cible
 */
#ifdef __BIGGEST_ALIGNMENT__
#define ALIGNMENT __BIGGEST_ALIGNMENT__
#else
#define ALIGNMENT 16
#endif

#define FB_VALID_OR(x, val)  if (!is_fb_link_valid(x)) {\
                        set_error_code(FB_LINK_BROKEN);\
                        return val;}

// Définition du type et de la valeur du garde, en fonction de l'alignement
// Un garde fait la taille de l'alignement, ce qui évite de devoir réaligner plus tard
// La valeur du garde est plus ou moins aléatoire. Par contre, on évite d'avoir des zéro car c'est une valeur qui est
// statistiquement fréquente en informatique, et on pourrait facilement laisser passer des écrasements du garde par des
// zéros.
#if ALIGNMENT == 16
typedef __uint128_t guard;
#define GUARD_VALUE ((guard) (((__uint128_t) 0xe91f8f0551fce198 << 64) | 0xcdd22bcadb23621a))
#elif ALIGNMENT == 8
typedef uint64_t guard;
#define GUARD_VALUE ((guard) 0xe91f8f0551fce198)
#elif ALIGNMENT == 4
typedef uint32_t guard;
#define GUARD_VALUE ((guard) 0xe91f8f05)
#endif

enum error_code LAST_ERROR;

static inline void set_error_code(enum error_code x) {
    LAST_ERROR = x;
}

/* structure placée au début de la zone de l'allocateur

   Elle contient toutes les variables globales nécessaires au
   fonctionnement de l'allocateur

   Elle peut bien évidemment être complétée
*/
struct allocator_header {
    size_t memory_size;
    mem_fit_function_t *fit;
    bool guards_enabled;
} __attribute__ ((aligned (16))); // Essentiel au bon fonctionnement de l'allocateur


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

bool is_fb_link_valid(struct fb *x) {
    if (x->next == NULL) {
        return true;
    } else {
        struct fb *y = x->next;
        size_t dif = (size_t) ((void *) y - (void *) x) -8 ;
        return x->size <= dif;
    }
}


void mem_init(void *mem, size_t taille, bool enable_guards) {
    // On s'assure que l'attribut ((aligned)) ci-dessus marche bien avec notre compilateur
    // Un bon compilateur optimisera sans aucun doute la ligne ci-dessous en l'enlevant
    assert(sizeof(struct allocator_header) % 16 == 0);

    // On met en place fb
    memory_addr = mem;
    //On met en place allocator header
    *get_header() = (struct allocator_header) {
        .memory_size = taille,
        .guards_enabled = enable_guards,
    };
    /* On vérifie qu'on a bien enregistré les infos et qu'on
     * sera capable de les récupérer par la suite
     */
    assert(mem == get_system_memory_addr());
    assert(taille == get_system_memory_size());

    VALGRIND_CREATE_MEMPOOL(mem, sizeof(struct fb), false);

    struct fb *head = get_fb_head();
    head->size = taille - sizeof(struct allocator_header);
    head->next = NULL;

    mem_fit(&mem_fit_first);
}


void mem_init_auto(bool enable_guards) {
    mem_init(get_memory_adr(), get_memory_size(), enable_guards);
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


void align_correctly(size_t *val) {
    size_t x = *val % (size_t) ALIGNMENT;
    *val += x ? ALIGNMENT - x : 0;
}


void *mem_alloc(size_t requested_size) {
    if (requested_size == 0) {
        // On peut retourner n'importe quel pointeur mais pour éviter les UB, il faut qu'il soit non-nul et aligné à la
        // taille d'un registre. Ce cas sera géré dans `mem_free`.
        return get_fb_head();
    }

    // On aligne, c'est plus prudent, car cela garantit que tous les fb sont alignés (le contraire serait
    // potentiellement problématique sur certaines architectures).
    align_correctly(&requested_size);

    bool guards_enabled = get_header()->guards_enabled;
    size_t actual_size = requested_size + (!guards_enabled ? 0 : 2*sizeof(guard));

    struct fb *fb = get_header()->fit(get_fb_head(), actual_size);

    if (fb) {
        struct fb *new_fb = ((void *) fb) + sizeof(struct fb) + actual_size;
        new_fb->size = fb->size - actual_size - sizeof(struct fb);
        new_fb->next = fb->next;

        fb->size = sizeof(struct fb);
        fb->next = ((void *) fb) + sizeof(struct fb) + actual_size;

        void* allocated = (void *) fb + sizeof(struct fb);

        if (guards_enabled) {
            *((guard*) allocated) = GUARD_VALUE;
            allocated += sizeof(guard);
            *((guard*) (allocated + requested_size)) = GUARD_VALUE;
        }

        VALGRIND_MEMPOOL_ALLOC(get_system_memory_addr(), allocated, requested_size);
        return allocated;
    } else {
        return NULL;
    }
}


bool mem_free(void *mem) {
    if (mem == get_fb_head()) {
        // Special case of `mem_alloc(0)`
        return true;
    }

    bool guards_enabled = get_header()->guards_enabled;
    if (guards_enabled) {
        mem -= sizeof(guard);
    }

    for (struct fb *cell = get_fb_head(); cell; cell = cell->next) {
        // détection de chaînages invalides causés par un écrasement des données de l'allocateur
        FB_VALID_OR(cell, false);
        if (((void *) cell) + cell->size == mem) {
            if (guards_enabled) {
                bool left_guard_violation = ((guard*) mem)[0] != GUARD_VALUE;
                bool right_guard_violation = ((guard*) cell->next)[-1] != GUARD_VALUE;
                if (left_guard_violation || right_guard_violation) {
                    LAST_ERROR = GUARD_VIOLATION;
                    return false;
                }
            }

            cell->size = (size_t) ((void *) cell->next - ((void *) cell)) + cell->next->size;
            cell->next = cell->next->next;
            VALGRIND_MEMPOOL_FREE(get_system_memory_addr(), mem + (guards_enabled ? sizeof(guard) : 0));
            return true;
        }
    }

    set_error_code(NOT_ALLOCATED);
    return false; // on essaie de libérer une zone mémoire non allouée
}


struct fb *mem_fit_first(struct fb *list, size_t size) {
    for (struct fb *cell = list; cell; cell = cell->next) {
        // détection de chaînages invalides causés par un écrasement des données de l'allocateur
        FB_VALID_OR(cell, NULL);
        if (!is_fb_link_valid(cell)) {
            set_error_code(FB_LINK_BROKEN);
            return NULL;
        }
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
    if (zone == get_fb_head()) {
        // Special case of `mem_alloc(0)`
        return 0;
    }

    for (struct fb *cell = get_fb_head(); cell; cell = cell->next) {
        // détection de chaînages invalides causés par un écrasement des données de l'allocateur
        FB_VALID_OR(cell, MEM_GET_SIZE_ERROR);
        if (((void *) cell) + cell->size == zone) {
            // Ne devrait pas être nul si la mémoire est dans un état valide et que la condition ci-dessus est vérifiée
            struct fb *next = cell->next;

            return ((void *) next) - ((void *) cell) - cell->size;
        }
    }

    set_error_code(NOT_ALLOCATED);
    return MEM_GET_SIZE_ERROR; // On retourne la val. max d'un size_t pour signifier une erreur
}

/* Fonctions facultatives
 * autres stratégies d'allocation
 */
struct fb *mem_fit_best(struct fb *list, size_t size) {
    bool is_min = false;
    size_t size_min;
    struct fb *cell_min = NULL;

    for (struct fb *cell = list; cell; cell = cell->next) {
        // détection de chaînages invalides causés par un écrasement des données de l'allocateur
        FB_VALID_OR(cell, NULL);

        ssize_t free_space = (ssize_t) cell->size - (ssize_t) 2 * (ssize_t) sizeof(struct fb);

        if ((ssize_t) size <= free_space) {
            if (is_min && free_space < size_min) {
                cell_min = cell;
                size_min = free_space;
            }
            if (!is_min) {
                cell_min = cell;
                size_min = free_space;
                is_min = true;
            }
        }
    }
    return cell_min;
}

struct fb *mem_fit_worst(struct fb *list, size_t size) {
    bool is_max = false;
    size_t size_max;
    struct fb *cell_max = NULL;

    for (struct fb *cell = list; cell; cell = cell->next) {
        // détection de chaînages invalides causés par un écrasement des données de l'allocateur
        FB_VALID_OR(cell, NULL);
        ssize_t free_space = (ssize_t) cell->size - (ssize_t) 2 * (ssize_t) sizeof(struct fb);

        if ((ssize_t) size <= free_space) {
            if (is_max && free_space > size_max) {
                cell_max = cell;
                size_max = free_space;
            }
            if (!is_max) {
                cell_max = cell;
                size_max = free_space;
                is_max = true;
            }
        }
    }
    return cell_max;
}

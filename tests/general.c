#include <assert.h>
#include <stdio.h>
#include "../mem.h"

#define TEST(function) { void function(); test_function(function, #function); }
#define assert_eq(a, b) { size_t av = (size_t) (a), bv = (size_t) (b); if (av != bv) { fprintf(stderr, "%s = %ld\n%s = %ld\n", #a, av, #b, bv); assert(0); } }
#define UNUSED __attribute__((unused))

char* RESULT_OK = "\033[0;32m✓\033[0m";
char* RESULT_ERR = "\033[0;31m✗\033[0m";

void* get_memory_adr();

void test_function(void (*function)(), const char* name) {
    fprintf(stderr, "%s %s\n", RESULT_ERR, name);
    mem_init_auto();
    function();
    fprintf(stderr, "\033[1A%s %s\n", RESULT_OK, name);
}

int main() {
    // Certains tests dépendent de ça
    assert(__BIGGEST_ALIGNMENT__ == 16);

    fprintf(stderr, "Si les tests s'affichent avec simultanément une croix et une coche (sur deux lignes), c'est que le terminal utilisé ne supporte pas certains codes ANSI. La coche prévaut.\n");

    TEST(comme_le_schema);
    TEST(alloc_free_alloc_free_same_pointer);
    TEST(double_free);
    TEST(free_not_allocated);
    TEST(broken_chain);

    TEST(fit_first);
    TEST(fit_best);
    TEST(fit_worst);
}

void comme_le_schema() {
    void* a = mem_alloc(16);
    void* b = mem_alloc(32);
    mem_free(a);

    void* premier_fb = get_memory_adr() + 16;
    assert_eq(b - premier_fb, 48);
}

void alloc_free_alloc_free_same_pointer() {
    void* a1 = mem_alloc(16);
    mem_free(a1);
    void* a2 = mem_alloc(16);
    mem_free(a2);

    assert_eq(a1, a2);
}

void double_free() {
    void* a = mem_alloc(1024);

    assert(mem_free(a));
    assert(!mem_free(a));

    assert_eq(LAST_ERROR, NOT_ALLOCATED);
}

void free_not_allocated() {
    void* a = mem_alloc(16);
    void* b = mem_alloc(32);

    // On essaye de libérer un pointeur avant notre tas
    void *ptr = NULL;
    assert(!mem_free(ptr));
    assert_eq(LAST_ERROR, NOT_ALLOCATED);

    // On provoque une libération normale pour réinitialiser l'état d'erreur
    assert(mem_free(a));

    // On essaye de libérer un pointeur sur notre tas, qui ne correspond à rien
    ptr = (void*) 512;
    assert(!mem_free(ptr));
    assert_eq(LAST_ERROR, NOT_ALLOCATED);

    // On provoque une libération normale pour réinitialiser l'état d'erreur
    assert(mem_free(b));

    // On essaye de libérer un pointeur après notre tas
    ptr = (void*) -1; // 0xffffff...
    assert(!mem_free(ptr));
    assert_eq(LAST_ERROR, NOT_ALLOCATED);
}

void broken_chain() {
    void *ptr = mem_alloc(8);
    assert(mem_free(ptr));

    ptr = mem_alloc(8);
    size_t *fb_size = (size_t *) (ptr - 16);
    *fb_size = 37; // 37 > 8

    assert(!mem_free(ptr));
    assert_eq(LAST_ERROR, FB_LINK_BROKEN);
}

void fit_first() {
    mem_fit(mem_fit_first);

    void* a = mem_alloc(16);
    UNUSED void* b = mem_alloc(16);
    void* c = mem_alloc(16);

    mem_free(a);
    mem_free(c);

    // On a un petit trou avant et un grand trou après b
    // Si on réalloue, le premier devrait être rempli

    void* a_bis = mem_alloc(16);
    assert_eq(a_bis, a);
}

void fit_best() {
    mem_fit(mem_fit_best);

    void* a = mem_alloc(32);
    UNUSED void* b = mem_alloc(16);
    void* c = mem_alloc(16);
    UNUSED void* d = mem_alloc(16);

    mem_free(a);
    mem_free(c);

    // On a un moyen trou (a, 32 octets), suivi d'un moyen (c, 16 octets), suivi du reste, avec des blocs occupés entre
    // On va essayer d'allouer 16 octets, on devrait nous renvoyer la même adresse que l'ex-c

    void* c_bis = mem_alloc(16);
    assert_eq(c_bis, c);
}

void fit_worst() {
    mem_fit(mem_fit_worst);

    void* a = mem_alloc(32);
    UNUSED void* b = mem_alloc(16);
    void* c = mem_alloc(16);

    mem_free(a);
    mem_free(c);

    // On a un moyen trou (a, 32 octets), suivi d'un moyen (c, 16 octets), suivi du reste, avec des blocs occupés entre
    // On va essayer d'allouer 16 octets, on devrait nous renvoyer la même adresse que l'ex-c

    void* c_bis = mem_alloc(32);
    assert_eq(c_bis, c);
}

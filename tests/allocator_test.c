#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

static uint32_t state = 1849;
static uint32_t random_word(void) {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

int main(void) {
    void *pointers[128] = {0};
    size_t sizes[128] = {0};
    mem_init();
    assert(mm_init() == 0);
    assert(mm_malloc(0) == NULL);
    assert(mm_calloc(SIZE_MAX, 2) == NULL);
    mm_free(NULL);
    /* Grow the final block by one alignment unit, then split it repeatedly. */
    void *tail = mm_malloc(1020);
    memset(tail, 123, 1020);
    tail = mm_realloc(tail, 1028);
    assert(tail);
    for (unsigned i = 0; i < 1020; ++i) assert(((unsigned char *)tail)[i] == 123);
    mm_checkheap(0);
    tail = mm_realloc(tail, 12);
    mm_free(tail);
    for (unsigned step = 0; step < 100000; ++step) {
        unsigned i = random_word() % 128;
        unsigned char pattern = (unsigned char)(i + 1);
        if (pointers[i]) {
            for (size_t j = 0; j < sizes[i]; ++j)
                assert(((unsigned char *)pointers[i])[j] == pattern);
        }
        size_t size = random_word() % 8192;
        if (random_word() % 3 == 0) size = random_word() % 32;
        if (pointers[i] && random_word() % 3 == 0) {
            mm_free(pointers[i]);
            pointers[i] = NULL;
            sizes[i] = 0;
        } else {
            void *p;
            if (!pointers[i] && step % 2) {
                p = mm_calloc(size, 1);
                for (size_t j = 0; j < size; ++j) assert(((unsigned char *)p)[j] == 0);
            } else {
                p = mm_realloc(pointers[i], size);
                for (size_t j = 0; j < size && j < sizes[i]; ++j)
                    assert(((unsigned char *)p)[j] == pattern);
            }
            assert(!size || (p && ((uintptr_t)p % 8 == 0)));
            pointers[i] = p;
            sizes[i] = size;
            if (size) memset(p, pattern, size);
        }
        mm_checkheap(0);
    }
    for (unsigned i = 0; i < 128; ++i) mm_free(pointers[i]);
    mm_checkheap(0);
    mem_reset_brk();
    assert(mm_init() == 0);
    mm_checkheap(0);
    mem_deinit();
    puts("allocator stress test passed");
}

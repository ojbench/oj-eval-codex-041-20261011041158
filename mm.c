/*
 * Segregated explicit free lists with immediate boundary-tag coalescing.
 * Each block has a 4-byte size/allocated/previous-allocated header. Only
 * free blocks have a size footer and two 32-bit links relative to heap_base.
 * Payloads are 8-byte aligned; the minimum block is 16 bytes. List heads
 * reside in the simulated heap, so all allocator storage is accounted for.
 */
#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

#ifdef DRIVER
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif

#define BINS 32
#define USED 1u
#define PREV_USED 2u
#define MIN_BLOCK 16u
static unsigned char *heap_base;
static unsigned char *first;
static uint32_t *heads;
static uint32_t nonempty;

/* Access naturally aligned words in the simulated heap. */
static inline uint32_t word(const void *p) { return *(const uint32_t *)p; }
static inline void put(void *p, uint32_t x) { *(uint32_t *)p = x; }
static inline uint32_t size_of(const unsigned char *p) { return word(p) & ~7u; }
static inline uint32_t offset(const unsigned char *p) { return (uint32_t)(p - heap_base); }
static inline unsigned char *at(uint32_t n) { return heap_base + n; }

/* Exact small-size bins followed by logarithmic size classes. */
static unsigned bin(uint32_t n) {
    if (n <= 128) return n / 8 - 2;
    unsigned b = 15;
    n >>= 8;
    while (n && b < BINS - 1) { ++b; n >>= 1; }
    return b;
}

/* Remove a known free block in constant time. */
static void unlink_block(unsigned char *p) {
    uint32_t prev = word(p + 4), next = word(p + 8);
    unsigned b = bin(size_of(p));
    if (prev) put(at(prev) + 8, next);
    else heads[b] = next;
    if (next) put(at(next) + 4, prev);
    if (!heads[b]) nonempty &= ~(1u << b);
}

/* Insert at the front of the corresponding size class. */
static void link_block(unsigned char *p) {
    unsigned b = bin(size_of(p));
    uint32_t next = heads[b];
    put(p + 4, 0);
    put(p + 8, next);
    if (next) put(at(next) + 4, offset(p));
    heads[b] = offset(p);
    nonempty |= 1u << b;
}

/* Merge physical free neighbors, then publish the resulting free block. */
static unsigned char *coalesce(unsigned char *p) {
    uint32_t n = size_of(p), flags = word(p) & PREV_USED;
    if (!flags) {
        unsigned char *prev = p - word(p - 4);
        unlink_block(prev);
        n += size_of(prev);
        p = prev;
        flags = word(p) & PREV_USED;
    }
    unsigned char *next = p + n;
    if (!(word(next) & USED)) {
        unlink_block(next);
        n += size_of(next);
    }
    put(p, n | flags);
    put(p + n - 4, n);
    put(p + n, word(p + n) & ~PREV_USED);
    link_block(p);
    return p;
}

/* Extend the heap, replacing its epilogue with a free block. */
static unsigned char *extend(uint32_t n) {
    if (n > INT_MAX) return NULL;
    unsigned char *q = mem_sbrk((int)n);
    if (q == (void *)-1) return NULL;
    unsigned char *p = q - 4;
    put(p, n | (word(p) & PREV_USED));
    put(p + n - 4, n);
    put(p + n, USED);
    return coalesce(p);
}

/* Initialize heap-resident bins and an allocated zero-size epilogue. */
int mm_init(void) {
    heap_base = mem_sbrk(BINS * 4 + 8);
    if (heap_base == (void *)-1) return -1;
    heads = (uint32_t *)heap_base;
    memset(heads, 0, BINS * 4);
    nonempty = 0;
    first = heap_base + BINS * 4 + 4;
    put(first - 4, USED);
    put(first, USED | PREV_USED);
    return 0;
}

/* Convert payload length to a representable aligned block size. */
static uint32_t adjusted(size_t n) {
    if (!n || n > (size_t)INT_MAX - 11) return 0;
    uint32_t result = ((uint32_t)n + 11) & ~7u;
    return result < MIN_BLOCK ? MIN_BLOCK : result;
}

/* Allocate a prefix, returning any useful remainder to the free lists. */
static void place(unsigned char *p, uint32_t need, uint32_t total) {
    uint32_t flags = word(p) & PREV_USED;
    if (total - need >= MIN_BLOCK) {
        put(p, need | flags | USED);
        unsigned char *rest = p + need;
        put(rest, (total - need) | PREV_USED);
        put(p + total - 4, total - need);
        coalesce(rest);
    } else {
        put(p, total | flags | USED);
        put(p + total, word(p + total) | PREV_USED);
    }
}

/* Bounded best-fit within each nonempty class; extend only if no fit exists. */
void *malloc(size_t size) {
    uint32_t need = adjusted(size);
    if (!need) return NULL;
    unsigned char *best = NULL;
    uint32_t mask = nonempty & (~0u << bin(need));
    while (mask) {
        unsigned b = (unsigned)__builtin_ctz(mask);
        uint32_t best_size = UINT32_MAX;
        unsigned candidates = 0;
        for (uint32_t off = heads[b]; off; off = word(at(off) + 8)) {
            unsigned char *p = at(off);
            uint32_t n = size_of(p);
            if (n >= need) {
                if (n < best_size) { best = p; best_size = n; }
                if (n == need || ++candidates == 12) break;
            }
        }
        if (best) break;
        mask &= mask - 1;
    }
    if (!best) {
        unsigned char *end = (unsigned char *)mem_heap_hi() - 3;
        uint32_t tail = (word(end) & PREV_USED) ? 0 : word(end - 4);
        uint32_t growth = need - tail;
        if (growth < 1024) growth = 1024;
        best = extend(growth);
        if (!best) return NULL;
    }
    uint32_t total = size_of(best);
    unlink_block(best);
    place(best, need, total);
    return best + 4;
}

/* Release a block and immediately eliminate adjacent free blocks. */
void free(void *ptr) {
    if (!ptr) return;
    unsigned char *p = (unsigned char *)ptr - 4;
    put(p, word(p) & ~USED);
    coalesce(p);
}

/* Reuse storage when shrinking or when the successor can supply growth. */
void *realloc(void *ptr, size_t size) {
    if (!ptr) return malloc(size);
    if (!size) { free(ptr); return NULL; }
    uint32_t need = adjusted(size);
    if (!need) return NULL;
    unsigned char *p = (unsigned char *)ptr - 4;
    uint32_t old = size_of(p);
    if (need <= old) { place(p, need, old); return ptr; }
    unsigned char *next = p + old;
    uint32_t available = old + ((word(next) & USED) ? 0 : size_of(next));
    if (available < need && size_of(p + available) == 0) {
        uint32_t growth = need - available;
        if (growth < MIN_BLOCK) growth = MIN_BLOCK;
        if (extend(growth)) available = old + size_of(next);
    }
    if (available >= need) {
        unlink_block(next);
        place(p, need, available);
        return ptr;
    }
    void *result = malloc(size);
    if (!result) return NULL;
    memcpy(result, ptr, old - 4);
    free(ptr);
    return result;
}

/* Reject multiplication overflow before allocating and zeroing the payload. */
void *calloc(size_t count, size_t size) {
    if (size && count > SIZE_MAX / size) return NULL;
    size_t bytes = count * size;
    void *result = malloc(bytes);
    if (result) memset(result, 0, bytes);
    return result;
}

/*
 * Verify every invariant: aligned payloads, sizes >=16 and divisible by 8,
 * heap bounds, epilogue allocation, previous-allocation bits, free footers,
 * no adjacent free blocks, valid reciprocal links, correct bins, and exactly
 * one list entry per physical free block. A step bound also detects cycles.
 */
void mm_checkheap(int verbose) {
    (void)verbose;
    unsigned char *end = (unsigned char *)mem_heap_hi() - 3;
    size_t free_count = 0, listed = 0;
    uint32_t previous_used = PREV_USED;
    unsigned char *p = first;
    for (; p < end; p += size_of(p)) {
        uint32_t n = size_of(p);
        assert(((uintptr_t)(p + 4) & 7) == 0);
        assert(n >= MIN_BLOCK && n % 8 == 0 && n <= (size_t)(end - p));
        assert((word(p) & PREV_USED) == previous_used);
        if (!(word(p) & USED)) {
            assert(previous_used && word(p + n - 4) == n);
            ++free_count;
            size_t occurrences = 0, steps = 0;
            for (uint32_t off = heads[bin(n)]; off; off = word(at(off) + 8)) {
                assert(off >= (uint32_t)(first - heap_base) && at(off) < end);
                assert(++steps <= mem_heapsize() / MIN_BLOCK);
                if (at(off) == p) ++occurrences;
            }
            assert(occurrences == 1);
        }
        previous_used = (word(p) & USED) ? PREV_USED : 0;
    }
    assert(p == end && size_of(end) == 0 && (word(end) & USED));
    assert((word(end) & PREV_USED) == previous_used);
    for (unsigned b = 0; b < BINS; ++b) {
        assert(!!heads[b] == !!(nonempty & (1u << b)));
        uint32_t prev = 0;
        for (uint32_t off = heads[b]; off; off = word(at(off) + 8)) {
            assert(off >= (uint32_t)(first - heap_base) && at(off) < end);
            p = at(off);
            assert(((uintptr_t)(p + 4) & 7) == 0);
            assert(!(word(p) & USED) && bin(size_of(p)) == b);
            assert(word(p + 4) == prev);
            assert(++listed <= free_count);
            prev = off;
        }
    }
    assert(listed == free_count);
}

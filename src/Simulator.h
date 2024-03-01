#ifndef SIMULADOR_H
#define SIMULADOR_H

#include <inttypes.h>

typedef struct _result_t {
    int misses;
    int hits;
    int capacityMisses;
    int conflictMisses;
    int compulsoryMisses;
    int accesses;
} result_t;

enum replacementPolicy_t {
    RANDOM,
    LRU,
    FIFO
};

result_t simulateDirectMapping( uint32_t * addresses, size_t size, uint32_t bsize, uint32_t nsets );
result_t simulate( uint32_t * addresses, size_t size, uint32_t nsets, uint32_t bsize, uint32_t assoc, int replacementPolicy );
#endif
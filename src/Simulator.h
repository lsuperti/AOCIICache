#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <inttypes.h>
#include "CacheConfig.h"

typedef struct _result_t {
    int  hits;
    int  capacityMisses;
    int  conflictMisses;
    int  compulsoryMisses;
    int  accesses;
} result_t;

enum replacementPolicy_t {
    RANDOM,
    LRU,
    FIFO
};

result_t simulateDirectMapping( uint32_t * addresses, size_t addressesSize, uint32_t bsize, uint32_t nsets );
result_t * simulate( uint32_t * addresses, size_t addressesSize, cacheConfigList_t * cacheConfigList );
#endif
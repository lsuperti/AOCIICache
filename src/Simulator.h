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

typedef struct _cacheLine_t {
    bool          valid;
    uint32_t      tag;
    unsigned int  lastUsed; // For LRU
    unsigned int  inserted; // For FIFO
} cacheLine_t;

typedef struct _cacheSet_t {
    cacheLine_t * lines;
} cacheSet_t;

typedef struct _cache_t {   
    // Cache configuration
    cacheConfig_t      cacheConfig;
    
    // Runtime parameters
    uint32_t           validLines;
    
    // Replacement policy parameters
    unsigned int       lruCounter;
    unsigned int       fifoCounter;
    
    // Statistics
    result_t           result;

    // Cache structure
    cacheSet_t *       sets;

    // Next level cache
    struct _cache_t *  nextLevel;
} cache_t;

cache_t * initializeCache( cacheConfigList_t * cacheConfigList );
void accessCache_r( cache_t * cache, uint32_t address );
void destroyCache( cache_t * cache );
result_t simulateDirectMapping( uint32_t * addresses, size_t addressesSize, uint32_t bsize, uint32_t nsets );
result_t * simulate( uint32_t * addresses, size_t addressesSize, cacheConfigList_t * cacheConfigList );

#endif
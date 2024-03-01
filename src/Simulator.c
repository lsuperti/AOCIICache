#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "Simulator.h"

result_t simulateDirectMapping( uint32_t * addresses, size_t size, uint32_t bsize, uint32_t nsets ) {
    uint32_t     tag;
    uint32_t     indice;
    int          missCompulsorio = 0;
    int          missConflito = 0;
    int          hit = 0;
    int          miss = 0;
    int nBitsOffset = log2( bsize );
    int nBitsIndice = log2( nsets );

    // Intellisense (MSVC) doesn't support variable length arrays, so a placeholder is used in the editor.
    #ifndef __INTELLISENSE__
    bool cacheVal[ nsets ];
    uint32_t cacheTag[ nsets ];
    #else
    bool cacheVal[ 1 ];
    uint32_t cacheTag[ 1 ];
    #endif
    
    for ( size_t i = 0; i < size; i++ ) {

        tag = addresses[ i ] >> ( nBitsIndice + nBitsOffset );
        indice = ( addresses[ i ] >> nBitsOffset ) & ( ( 1 << nBitsIndice ) - 1 );

        // para o mapeamento direto
        if ( cacheVal[ indice ] == 0 ) {

            missCompulsorio++;
            miss++;
            cacheTag[ indice ] = tag;
            cacheVal[ indice ] = 1;
            // estas duas últimas instruções representam o tratamento da falta.

        } else if ( cacheTag[ indice ] == tag ) {

            hit++;

        } else {
            
            miss++;
            missConflito++;
            cacheVal[ indice ] = 1;
            cacheTag[ indice ] = tag;
        
        }
    
    }

    result_t result = { .hits = hit, .compulsoryMisses = missCompulsorio, .capacityMisses = 0, .conflictMisses = missConflito, .accesses = size };
    
    return result;
}

typedef struct {
    bool          valid;
    uint32_t      tag;
    unsigned int  lastUsed; // For LRU
    unsigned int  inserted; // For FIFO
} cacheLine_t;

typedef struct {
    cacheLine_t * lines; // This will have size equal to the assoc
} cacheSet_t;

typedef struct {
    uint32_t      nsets;
    uint32_t      bsize;
    uint32_t      assoc;
    cacheSet_t *  sets;
    unsigned int  lruCounter;
    unsigned int  fifoCounter;
} cache_t;

cache_t * initializeCache( uint32_t nsets, uint32_t bsize, uint32_t assoc ) {
    cache_t * cache = malloc( sizeof( cache_t ) );
    cache->nsets = nsets;
    cache->bsize = bsize;
    cache->assoc = assoc;
    cache->sets = malloc( sizeof( cacheSet_t ) * nsets );
    cache->lruCounter = 0;
    cache->fifoCounter = 0;

    for ( size_t i = 0; i < nsets; i++ ) {
        cache->sets[ i ].lines = malloc( sizeof( cacheLine_t ) * assoc );
        for ( size_t j = 0; j < assoc; j++ ) {
            cache->sets[ i ].lines[ j ].valid = false;
            cache->sets[ i ].lines[ j ].lastUsed = 0;
        }
    }
    
    return cache;
}

void parseAddress( cache_t * cache, uint32_t address, uint32_t * tag, uint32_t * setIndex, uint32_t * blockOffset ) {
    uint32_t offsetBits = log2( cache->bsize );
    uint32_t setBits = log2( cache->nsets );

    *blockOffset = address & ( ( 1 << offsetBits ) - 1 );
    *setIndex = ( address >> offsetBits ) & ( ( 1 << setBits ) - 1 );
    *tag = address >> ( offsetBits + setBits );
}

void destroyCache( cache_t * cache ) {
    for ( size_t i = 0; i < cache->nsets; i++ ) {
        free( cache->sets[ i ].lines );
    }
    
    free( cache->sets );
    free( cache );
}

bool cacheFull( cache_t * cache ) {
    for ( uint32_t i = 0; i < cache->nsets; i++ ) {
        for ( uint32_t j = 0; j < cache->assoc; j++ ) {
            if ( !cache->sets[ i ].lines[ j ].valid ) {
                return false;
            }
        }
    }
    
    return true;
}

void updateCapacityConflictMissStats( cache_t * cache, result_t * result, bool * cacheKnownFull ) {
    if ( *cacheKnownFull ) {
        result->capacityMisses++;
    } else {
        if ( cacheFull( cache ) ) {
            *cacheKnownFull = true;
            result->capacityMisses++;
        } else {
            result->conflictMisses++;
        }
    }
}

void accessRandomCache( cache_t * cache, uint32_t address, result_t * result, bool * cacheKnownFull ) {
    uint32_t tag, setIndex, blockOffset;
    parseAddress(cache, address, &tag, &setIndex, &blockOffset);

    cacheSet_t * set = &cache->sets[ setIndex ];
    int emptyLineIndex = -1; // Keep track of an empty line, if any

    result->accesses++; // Increment the number of accesses in all cases

    // First, try to find a cache hit or an empty line
    for ( uint32_t i = 0; i < cache->assoc; i++ ) {
        if ( set->lines[ i ].valid && set->lines[ i ].tag == tag ) {
            // Cache hit
            result->hits++;
            
            return;
        }
        
        if ( !set->lines[ i ].valid && emptyLineIndex == -1 ) {
            emptyLineIndex = i; // Remember the first empty line
        }
    }

    // Handle cache miss
    if ( emptyLineIndex != -1 ) {
        // If there's an empty line, use it
        set->lines[ emptyLineIndex ].valid = true;
        set->lines[ emptyLineIndex ].tag = tag;

        result->compulsoryMisses++;
    } else {
        // If no empty line, select a random line to replace
        uint32_t replaceIndex = rand() % cache->assoc;
        set->lines[ replaceIndex ].tag = tag;
        set->lines[ replaceIndex ].valid = true;
        
        updateCapacityConflictMissStats( cache, result, cacheKnownFull );
    }
}

void accessLRUCache( cache_t * cache, uint32_t address, result_t * result, bool * cacheKnownFull ) {
    uint32_t  tag;
    uint32_t  setIndex;
    uint32_t  blockOffset;
    
    parseAddress( cache, address, &tag, &setIndex, &blockOffset );

    cacheSet_t * set = &cache->sets[ setIndex ];
    
    int emptyLineIndex = -1;
    unsigned int oldestTime = UINT_MAX;
    int32_t lruIndex = -1; // Index of the LRU line

    result->accesses++; // Increment the number of accesses in all cases

    // Update LRU counter and find a cache hit, an empty line, or the LRU line
    for ( uint32_t i = 0; i < cache->assoc; i++ ) {
        if ( set->lines[i].valid ) {
            if ( set->lines[i].tag == tag ) {
                // Cache hit
                set->lines[i].lastUsed = ++cache->lruCounter; // Update usage time
                result->hits++;
                
                return;
            }
            
            if ( set->lines[ i ].lastUsed < oldestTime ) {
                oldestTime = set->lines[ i ].lastUsed;
                lruIndex = i; // Remember the LRU line
            }
        } else if ( emptyLineIndex == -1 ) {
            emptyLineIndex = i; // Remember the first empty line
        }
    }

    // Cache miss handling
    if ( emptyLineIndex != -1 ) {
        // Use the empty line
        set->lines[ emptyLineIndex ].valid = 1;
        set->lines[ emptyLineIndex ].tag = tag;
        set->lines[ emptyLineIndex ].lastUsed = ++cache->lruCounter; // Update usage time

        result->compulsoryMisses++;
    } else {
        // Replace the LRU line
        set->lines[ lruIndex ].tag = tag;
        set->lines[ lruIndex ].lastUsed = ++cache->lruCounter; // Update usage time

        updateCapacityConflictMissStats( cache, result, cacheKnownFull );
    }
}

void accessCacheFIFO( cache_t * cache, unsigned address, result_t * result, bool * cacheKnownFull ) {
    uint32_t  tag;
    uint32_t  setIndex;
    uint32_t  blockOffset;
    
    parseAddress( cache, address, &tag, &setIndex, &blockOffset );

    cacheSet_t * set = &cache->sets[ setIndex ];
    int emptyLineIndex = -1;
    unsigned int oldestInsertion = UINT_MAX;
    int fifoIndex = -1; // Index for FIFO replacement

    result->accesses++; // Increment the number of accesses in all cases

    // Search for a cache hit, an empty line, or the oldest line for FIFO
    for ( uint32_t i = 0; i < cache->assoc; i++ ) {
        if ( set->lines[ i ].valid ) {
            if ( set->lines[ i ].tag == tag ) {
                // Cache hit, FIFO doesn't update on access, only on insertion
                result->hits++;

                return;
            }
            
            if ( set->lines[ i ].inserted < oldestInsertion ) {
                oldestInsertion = set->lines[ i ].inserted;
                fifoIndex = i; // Remember the oldest line for possible replacement
            }
        } else if ( emptyLineIndex == -1 ) {
            emptyLineIndex = i; // Remember the first empty line found
        }
    }

    // Cache miss handling
    if ( emptyLineIndex != -1 ) {
        // Use the empty line for new data
        set->lines[ emptyLineIndex ].valid = 1;
        set->lines[ emptyLineIndex ].tag = tag;
        set->lines[ emptyLineIndex ].inserted = ++cache->fifoCounter; // Set insertion time

        result->compulsoryMisses++;
    } else {
        // No empty line, replace the oldest line based on FIFO policy
        set->lines[ fifoIndex ].tag = tag;
        set->lines[ fifoIndex ].inserted = ++cache->fifoCounter; // Update insertion time for replaced line

        updateCapacityConflictMissStats( cache, result, cacheKnownFull );
    }
}

result_t simulate( uint32_t * addresses, size_t size, uint32_t nsets, uint32_t bsize, uint32_t assoc, int replacementPolicy ) {
    cache_t *  cache = initializeCache( nsets, bsize, assoc );
    result_t   result = { .accesses = 0, .capacityMisses = 0, .compulsoryMisses = 0, .hits = 0, .conflictMisses = 0 };
    bool       cacheKnownFull = false;

    if ( replacementPolicy == RANDOM ) {
        for ( size_t i = 0; i < size; i++ ) {
            accessRandomCache( cache, addresses[ i ], &result, &cacheKnownFull );
        }
        
        destroyCache( cache );
    } else if ( replacementPolicy == LRU ) {
        for ( size_t i = 0; i < size; i++ ) {
            accessLRUCache( cache, addresses[ i ], &result, &cacheKnownFull );
        }
        
        destroyCache( cache );
    } else if ( replacementPolicy == FIFO ) {
        for ( size_t i = 0; i < size; i++ ) {
            accessCacheFIFO( cache, addresses[ i ], &result, &cacheKnownFull );
        }
        
        destroyCache( cache );
    } else {
        fprintf( stderr, "Politica de substituição inválida ou não implementada." );
        destroyCache( cache );
        exit( EXIT_FAILURE );
    }

    return result;
}
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>

#include "Simulator.h"

/*
 * Calculates the base 2 logarithm of a number that is a power of 2.
 *
 * Since all properties of a cache are powers of 2, this function can be used instead of the math library's log2 function, which can cause issues with some compilers.
 */
unsigned int log2PowerOf2( unsigned int n ) {
    unsigned int log2 = 0;

    // A switch statement is used for the most common cases to improve performance
    switch ( n ) {
        case 2:
            return 1;
            break;
        case 4:
            return 2;
            break;
        case 8:
            return 3;
            break;
        case 16:
            return 4;
            break;
        case 32:
            return 5;
            break;
        case 64:
            return 6;
            break;
        case 128:
            return 7;
            break;
        case 256:
            return 8;
            break;
        case 512:
            return 9;
            break;
        case 1024:
            return 10;
            break;
        default:
            while ( n >>= 1 ) {
                log2++;
            }
            break;
    }
    
    return log2;
}

/*
 * This function simulates a directly mapped cache.
 *
 * Unlike the the other simulations, while using this function the replacement policy is not applicable and is not taken.
 * 
 * Any valid number of sets and block size can be used.
 */
result_t simulateDirectMapping( uint32_t * addresses, size_t addressesSize, uint32_t bsize, uint32_t nsets ) {
    uint32_t  tag;
    uint32_t  indice;
    int       missCompulsorio = 0;
    int       missConflito = 0;
    int       hit = 0;
    int       miss = 0;
    int       nBitsOffset = log2PowerOf2( bsize );
    int       nBitsIndice = log2PowerOf2( nsets );

    // Intellisense (MSVC) doesn't support variable length arrays, so a placeholder is used in the editor.
    #ifndef __INTELLISENSE__
    bool cacheVal[ nsets ];
    uint32_t cacheTag[ nsets ];
    #else
    bool cacheVal[ 1 ];
    uint32_t cacheTag[ 1 ];
    #endif

    /* Initialize the arrays with zero. It appears all test compilations initialized them with 0 already, but this is
    not guaranteed behaviour, so they are explicitly initialized. */
    memset( cacheVal, 0, sizeof( cacheVal ) );
    memset( cacheTag, 0, sizeof( cacheTag ) );
    
    for ( size_t i = 0; i < addressesSize; i++ ) {

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

    result_t result = { .hits = hit, .compulsoryMisses = missCompulsorio, .capacityMisses = 0, .conflictMisses = missConflito, .accesses = addressesSize };
    
    return result;
}

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
    // General parameters
    uint32_t      nsets;
    uint32_t      bsize;
    uint32_t      assoc;
    cacheSet_t *  sets;
    
    // Runtime parameters
    bool          cacheKnownFull;
    
    // Replacement policy parameters
    unsigned int  lruCounter;
    unsigned int  fifoCounter;
    
    // Statistics
    result_t      result;
} cache_t;

/*
 * This function initializes a cache structure with the given parameters.
 */
cache_t * initializeCache( uint32_t nsets, uint32_t bsize, uint32_t assoc ) {
    cache_t * cache = malloc( sizeof( cache_t ) );
    cache->nsets = nsets;
    cache->bsize = bsize;
    cache->assoc = assoc;
    cache->sets = malloc( sizeof( cacheSet_t ) * nsets );
    
    cache->cacheKnownFull = false;

    cache->lruCounter = 0;
    cache->fifoCounter = 0;

    cache->result = ( result_t ){ .hits = 0, .capacityMisses = 0, .conflictMisses = 0, .compulsoryMisses = 0, .accesses = 0 };

    for ( size_t i = 0; i < nsets; i++ ) {
        cache->sets[ i ].lines = malloc( sizeof( cacheLine_t ) * assoc );
        
        for ( size_t j = 0; j < assoc; j++ ) {
            cache->sets[ i ].lines[ j ].valid = false;
            cache->sets[ i ].lines[ j ].lastUsed = 0;
        }
    }
    
    return cache;
}

/*
 * This function parses a cache address into its tag, set index, and block offset.
 */
void parseAddress( cache_t * cache, uint32_t address, uint32_t * tag, uint32_t * setIndex, uint32_t * blockOffset ) {
    uint32_t offsetBits = log2PowerOf2( cache->bsize );
    uint32_t setBits = log2PowerOf2( cache->nsets );

    *blockOffset = address & ( ( 1 << offsetBits ) - 1 );
    *setIndex = ( address >> offsetBits ) & ( ( 1 << setBits ) - 1 );
    *tag = address >> ( offsetBits + setBits );
}

/*
 * This function destroys the cache structure.
 */
void destroyCache( cache_t * cache ) {
    for ( size_t i = 0; i < cache->nsets; i++ ) {
        free( cache->sets[ i ].lines );
    }
    
    free( cache->sets );
    free( cache );
}

/*
 * This function checks if the cache is full by checking if all the lines are valid.
 *
 * This function has a time complexity of O(n * m), where n is the number of sets and m is the associativity, so unecessary calls should be avoided.
 */
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

/*
 * This function determines if a cache miss is a capacity miss or a conflict miss and updates the statistics accordingly.
 *
 * It uses the slow cacheFull function, but it also keeps track of the known full status of the cache so it won't call it when the cache is already known to be full.
 */
void updateCapacityConflictMissStats( cache_t * cache ) {
    if ( cache->cacheKnownFull ) {
        cache->result.capacityMisses++;
    } else {
        if ( cacheFull( cache ) ) {
            cache->cacheKnownFull = true;
            cache->result.capacityMisses++;
        } else {
            cache->result.conflictMisses++;
        }
    }
}

/*
 * This function simulates a cache access using the random replacement policy.
 */
void accessCacheRandom( cache_t * cache, uint32_t address ) {
    uint32_t tag, setIndex, blockOffset;
    parseAddress(cache, address, &tag, &setIndex, &blockOffset);

    cacheSet_t * set = &cache->sets[ setIndex ];
    int emptyLineIndex = -1; // Keep track of an empty line, if any

    cache->result.accesses++; // Increment the number of accesses in all cases

    // Find a cache hit or an empty line
    for ( uint32_t i = 0; i < cache->assoc; i++ ) {
        if ( set->lines[ i ].valid && set->lines[ i ].tag == tag ) {
            // Hit
            cache->result.hits++;
            
            return;
        }
        
        if ( !set->lines[ i ].valid && emptyLineIndex == -1 ) {
            emptyLineIndex = i; // Remember the first empty line
        }
    }

    // Miss
    if ( emptyLineIndex != -1 ) {
        // If there's an empty line, use it
        set->lines[ emptyLineIndex ].valid = true;
        set->lines[ emptyLineIndex ].tag = tag;

        cache->result.compulsoryMisses++;
    } else {
        // If no empty line, replace a random line
        uint32_t replaceIndex = rand() % cache->assoc;
        set->lines[ replaceIndex ].tag = tag;
        set->lines[ replaceIndex ].valid = true;
        
        updateCapacityConflictMissStats( cache );
    }
}

/*
 * This function simulates a cache access using the LRU replacement policy.
 */
void accessCacheLRU( cache_t * cache, uint32_t address ) {
    uint32_t  tag;
    uint32_t  setIndex;
    uint32_t  blockOffset;
    
    parseAddress( cache, address, &tag, &setIndex, &blockOffset );

    cacheSet_t * set = &cache->sets[ setIndex ];
    
    int emptyLineIndex = -1;
    unsigned int oldestTime = UINT_MAX;
    int32_t lruIndex = -1;

    cache->result.accesses++; // Increment the number of accesses in all cases

    // Update LRU counter and find a cache hit, an empty line, or the LRU line
    for ( uint32_t i = 0; i < cache->assoc; i++ ) {
        if ( set->lines[i].valid ) {
            if ( set->lines[i].tag == tag ) {
                // Hit
                set->lines[i].lastUsed = ++cache->lruCounter; // Update usage time
                cache->result.hits++;
                
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

    // Miss
    if ( emptyLineIndex != -1 ) {
        // Use the empty line if there is one
        set->lines[ emptyLineIndex ].valid = 1;
        set->lines[ emptyLineIndex ].tag = tag;
        set->lines[ emptyLineIndex ].lastUsed = ++cache->lruCounter; // Update usage time

        cache->result.compulsoryMisses++;
    } else {
        // Replace the LRU line if there isn't an empty line
        set->lines[ lruIndex ].tag = tag;
        set->lines[ lruIndex ].lastUsed = ++cache->lruCounter; // Update usage time

        updateCapacityConflictMissStats( cache );
    }
}

/*
 * This function simulates a cache access using the FIFO replacement policy.
 */
void accessCacheFIFO( cache_t * cache, unsigned address ) {
    uint32_t  tag;
    uint32_t  setIndex;
    uint32_t  blockOffset;
    
    parseAddress( cache, address, &tag, &setIndex, &blockOffset );

    cacheSet_t * set = &cache->sets[ setIndex ];
    int emptyLineIndex = -1;
    unsigned int oldestInsertion = UINT_MAX;
    int fifoIndex = -1;

    cache->result.accesses++; // Increment the number of accesses in all cases

    // Search for a cache hit, an empty line, or the oldest line for FIFO
    for ( uint32_t i = 0; i < cache->assoc; i++ ) {
        if ( set->lines[ i ].valid ) {
            if ( set->lines[ i ].tag == tag ) {
                // Hit
                cache->result.hits++;

                return;
            }
            
            if ( set->lines[ i ].inserted < oldestInsertion ) {
                oldestInsertion = set->lines[ i ].inserted;
                fifoIndex = i; // Remember the oldest line for possible replacement
            }
        } else if ( emptyLineIndex == -1 ) {
            emptyLineIndex = i; // Remember the first empty line
        }
    }

    // Miss
    if ( emptyLineIndex != -1 ) {
        // Use the empty line if there is one
        set->lines[ emptyLineIndex ].valid = 1;
        set->lines[ emptyLineIndex ].tag = tag;
        set->lines[ emptyLineIndex ].inserted = ++cache->fifoCounter; // Set the insertion time

        cache->result.compulsoryMisses++;
    } else {
        // If there isn't an empty line, replace the oldest line
        set->lines[ fifoIndex ].tag = tag;
        set->lines[ fifoIndex ].inserted = ++cache->fifoCounter; // Update insertion time for the replaced line

        updateCapacityConflictMissStats( cache );
    }
}

/*
 * Simulates the behaviour of a cache accessing an array of addresses.
 *
 * It accepts any valid number of sets, block size, and associativity for 32-bit cache.
 * 
 * The supported replacement policies are RANDOM, LRU, and FIFO.
 */
result_t simulate( uint32_t * addresses, size_t addressesSize, uint32_t nsets, uint32_t bsize, uint32_t assoc, int replacementPolicy ) {
    cache_t *  cache = initializeCache( nsets, bsize, assoc );
    result_t   result;

    if ( replacementPolicy == RANDOM ) {
        for ( size_t i = 0; i < addressesSize; i++ ) {
            accessCacheRandom( cache, addresses[ i ] );
        }
    } else if ( replacementPolicy == LRU ) {
        for ( size_t i = 0; i < addressesSize; i++ ) {
            accessCacheLRU( cache, addresses[ i ] );
        }
    } else if ( replacementPolicy == FIFO ) {
        for ( size_t i = 0; i < addressesSize; i++ ) {
            accessCacheFIFO( cache, addresses[ i ] );
        }
    } else {
        fputs( "Politica de substituição inválida.\n", stderr );
        exit( EXIT_FAILURE );
    }

    result = cache->result;
    destroyCache( cache );
    
    return result;
}
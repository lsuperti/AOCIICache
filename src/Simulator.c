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

        //printf( "0x%08" PRIX32 "\n", addresses[ i ] );

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

	result_t result = { .misses = miss, .hits = hit, .compulsoryMisses = missCompulsorio, .capacityMisses = 0, .conflictMisses = missConflito, .accesses = size };
	
	return result;
}

typedef struct {
    bool valid;
    uint32_t tag;
    // Add a field for LRU, timestamp, or other metadata for replacement policy
} cacheLine_t;

typedef struct {
    cacheLine_t * lines; // This will have size equal to the assoc
} cacheSet_t;

typedef struct {
    unsigned nsets;
    unsigned bsize;
    unsigned assoc;
    cacheSet_t * sets;
    // Add fields for statistics (hits, misses, etc.)
} cache_t;

cache_t * initializeCache( uint32_t nsets, uint32_t bsize, uint32_t assoc ) {
    cache_t * cache = malloc( sizeof( cache_t ) );
    cache->nsets = nsets;
    cache->bsize = bsize;
    cache->assoc = assoc;
    cache->sets = malloc( sizeof( cacheSet_t ) * nsets );

    for ( size_t i = 0; i < nsets; i++ ) {
        cache->sets[ i ].lines = malloc( sizeof( cacheLine_t ) * assoc );
        for ( size_t j = 0; j < assoc; j++ ) {
            cache->sets[ i ].lines[ j ].valid = false;
            // Initialize other fields (e.g., tag, LRU metadata)
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

void accessRandomCache( cache_t * cache, uint32_t address, result_t * result ) {
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

		result->misses++;
		result->compulsoryMisses++;
    } else {
        // If no empty line, select a random line to replace
        uint32_t replaceIndex = rand() % cache->assoc;
        set->lines[ replaceIndex ].tag = tag;
        set->lines[ replaceIndex ].valid = true;

		result->misses++;
		if ( cacheFull( cache ) ) {
			result->capacityMisses++;
		} else {
			result->conflictMisses++;
		}
    }
}

result_t simulate( uint32_t * addresses, size_t size, uint32_t nsets, uint32_t bsize, uint32_t assoc, int replacementPolicy ) {
	cache_t * cache = initializeCache( nsets, bsize, assoc );
	result_t result = { .accesses = 0, .capacityMisses = 0, .compulsoryMisses = 0, .hits = 0, .misses = 0, .conflictMisses = 0 };

	if ( replacementPolicy == RANDOM ) {
		for ( size_t i = 0; i < size; i++ ) {
			accessRandomCache( cache, addresses[ i ], &result );
		}
		
		destroyCache( cache );
		
		return result;
	} else {
		printf( "Politica de substituição inválida ou não implementada." );
		destroyCache( cache );
		exit( EXIT_FAILURE );
		
		return result;
	}
}
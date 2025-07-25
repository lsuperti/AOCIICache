#include <stdlib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>

#include "CacheSimulator.h"
#include "CacheConfig.h"

void initializeCacheConfigList( cacheConfigList_t ** head, cacheConfig_t * cacheConfig ) {
    *head = malloc( sizeof( cacheConfigList_t ) );
    
    if ( *head == NULL ) {
        fputs( "Sem memória.\n", stderr );
        exit( EXIT_FAILURE );
    }

    ( *head )->cacheConfig = *cacheConfig;
    ( *head )->next = NULL;
}

/*
 * Pushes a cache configuration to a list of cache configurations.
 *
 * The cache configurations are sorted by level in ascending order, so lower levels are down the list. 
 */
void pushCacheConfig( cacheConfigList_t ** head, cacheConfig_t * cacheConfig ) {
    cacheConfigList_t * newCacheConfig = malloc( sizeof( cacheConfigList_t ) );

    if ( newCacheConfig == NULL ) {
        fputs( "Sem memória.\n", stderr );
        exit( EXIT_FAILURE );
    }
    
    newCacheConfig->cacheConfig = *cacheConfig;
    newCacheConfig->next = NULL;

    if ( *head == NULL ) {
        *head = newCacheConfig;
    } else {
        cacheConfigList_t *  current = *head;
        cacheConfigList_t *  prev = NULL;

        while ( current != NULL && current->cacheConfig.level < cacheConfig->level ) {
            prev = current;
            current = current->next;
        }

        if ( prev == NULL ) {
            newCacheConfig->next = *head;
            *head = newCacheConfig;
        } else {
            prev->next = newCacheConfig;
            newCacheConfig->next = current;
        }
    }
}

/*
 * Check if a number is a power of 2.
 */
bool isPowerOfTwo(uint32_t num) {
    return (num != 0) && ((num & (num - 1)) == 0);
}

/*
 * Verifies if a list of cache configurations is valid.
 */
void verifyCacheConfig( cacheConfigList_t * head ) {
    uint32_t             size;
    uint32_t             previousSize = 0;
    unsigned long        currentLevel = 1;
    cacheConfigList_t *  current = head;

    if ( head == NULL ) {
        fputs( "Não há nenhum nível de cache configurado.\n", stderr );
        exit( EXIT_FAILURE );
    }

    // The first cache level must be L1
    if ( head->cacheConfig.level != 1 ) {
        fputs( "A cache L1 não foi configurada.\n", stderr );
        exit( EXIT_FAILURE );
    }

    while ( current != NULL ) {
        // If there are cache levels missing, the cache configuration is invalid
        if ( current->cacheConfig.level != currentLevel ) {
            fprintf( stderr, "Cache L%lu não está configurada, enquanto L%lu está.\n", currentLevel, current->cacheConfig.level );
            exit( EXIT_FAILURE );
        }

        size = current->cacheConfig.nsets * current->cacheConfig.bsize * current->cacheConfig.assoc;

        // The size of the cache must not be zero
        if ( size == 0 ) {
            fprintf( stderr, "O tamanho da cache L%lu é zero.\n", current->cacheConfig.level );
            exit( EXIT_FAILURE );
        }

        // The size of the cache must not be smaller than the previous level
        if ( size < previousSize ) {
            fprintf( stderr, "O tamanho da cache L%lu (%" PRIu32 ") é menor que o tamanho da cache L%lu (%" PRIu32 ").\n", current->cacheConfig.level, size, current->cacheConfig.level - 1, previousSize );
            exit( EXIT_FAILURE );
        }

        // bsize must be a power of 2
        if ( !isPowerOfTwo(current->cacheConfig.bsize ) ) {
            fprintf( stderr, "O valor de <bsize> (%" PRIu32 ") da cache L%lu não é uma potência de 2.\n", current->cacheConfig.bsize, current->cacheConfig.level );
            exit( EXIT_FAILURE );
        }

        // assoc must be a power of 2
        if ( !isPowerOfTwo(current->cacheConfig.assoc ) ) {
            fprintf( stderr, "O valor de <assoc> (%" PRIu32 ") da cache L%lu não é uma potência de 2.\n", current->cacheConfig.assoc, current->cacheConfig.level );
            exit( EXIT_FAILURE );
        }

        // nsets must be a power of 2
        if ( !isPowerOfTwo( current->cacheConfig.nsets ) ) {
            fprintf( stderr, "O valor de <nsets> (%" PRIu32 ") da cache L%lu não é uma potência de 2.\n", current->cacheConfig.nsets, current->cacheConfig.level );
            exit( EXIT_FAILURE );
        }

        currentLevel++;
        current = current->next;
        previousSize = size;
    }
}

/*
 * Destroys a list of cache configurations.
 */
void destroyCacheConfigList( cacheConfigList_t * head ) {
    cacheConfigList_t * current = head;
    cacheConfigList_t * next;

    while ( current != NULL ) {
        next = current->next;
        free( current );
        current = next;
    }
}

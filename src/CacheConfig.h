#ifndef CACHE_CONFIG_H
#define CACHE_CONFIG_H

#include <inttypes.h>

typedef struct _cacheConfig_t {
    uint32_t       nsets;
    uint32_t       bsize;
    uint32_t       assoc;
    int            replacementPolicy;
    unsigned long  level;
} cacheConfig_t;

typedef struct _cacheConfigList_t {
    cacheConfig_t                cacheConfig;
    struct _cacheConfigList_t *  next;
} cacheConfigList_t;

void initializeCacheConfigList( cacheConfigList_t ** head, cacheConfig_t * cacheConfig );
void pushCacheConfig( cacheConfigList_t ** head, cacheConfig_t * cacheConfig );
void verifyCacheConfig( cacheConfigList_t * head );
void destroyCacheConfigList( cacheConfigList_t * head );

#endif
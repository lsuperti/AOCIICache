#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include "Simulator.h"

result_t directMapping( uint32_t * addresses, size_t size, uint32_t bsize, uint32_t nsets ) {
    int *        value;
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
		indice = ( addresses[ i ] >> nBitsOffset ) && ( pow( 2, nBitsIndice) - 1 );

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
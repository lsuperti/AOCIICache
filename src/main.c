#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>

#include "FileHandler.h"
#include "Simulator.h"

enum outFlag_t {
	FREEFORM_OUT = 0,
	STANDARDIZED_OUT = 1
};

void printOutput( int flagOut, result_t result );

int main( int argc, char *argv[] ) {
	char * quote = strchr( argv[ 0 ], ' ' ) == NULL ? "" : "\"";
	
	if ( argc != 7 ) {
		printf( "Número de argumentos incorreto. Utilize:\n" );
		printf( "%s%s%s <nsets> <bsize> <assoc> <substituição> <flag_saída> arquivo_de_entrada\n", quote, argv[ 0 ], quote );
		exit( EXIT_FAILURE );
	}

	int          nsets = atoi( argv[ 1 ] );
	int          bsize = atoi( argv[ 2 ] );
	int          assoc = atoi( argv[ 3 ] );
	char *       subst = argv[ 4 ];
	int          flagOut = atoi( argv[ 5 ] );
	char *       arquivoEntrada = argv[ 6 ];
	result_t	 result;

	printf( "nsets = %d\n", nsets );
	printf( "bsize = %d\n", bsize );
	printf( "assoc = %d\n", assoc );
	printf( "subst = %s\n", subst );
	printf( "flagOut = %d\n", flagOut );
	printf( "arquivo = %s\n", arquivoEntrada );

    uint32_t * addresses;
    size_t size;
    
    handleFile( arquivoEntrada, &addresses, &size );

	if ( assoc == 1 ) {
		result = directMapping( addresses, size, bsize, nsets );
	}

	printOutput( flagOut, result );

    free( addresses );

	return 0;
}

void printOutput( int flagOut, result_t result ) {
	const float hitRate = ( ( float ) result.hits / result.accesses );
	const float missRate = ( ( float ) result.misses / result.accesses );
	const float compulsoryMissRate = ( ( float ) result.compulsoryMisses / result.misses );
	const float capacityMissRate = ( ( float ) result.capacityMisses / result.misses );
	const float conflictMissRate = ( ( float ) result.conflictMisses / result.misses );
	
	if ( flagOut == FREEFORM_OUT ) {
		printf( "Hits: %d\n"
				"Misses: %d\n"
				"Compulsory Misses: %d\n"
				"Capacity Misses: %d\n"
				"Conflict Misses: %d\n"
				"Accesses: %d\n"
				"Hit rate: %f\n"
				"Miss rate: %f\n"
				"Compulsory miss rate: %f\n"
				"Capacity miss rate: %f\n"
				"Conflict miss rate: %f\n",
				result.hits,
				result.misses,
				result.compulsoryMisses,
				result.capacityMisses,
				result.conflictMisses,
				result.accesses,
				hitRate,
				missRate,
				compulsoryMissRate,
				capacityMissRate,
				conflictMissRate );
	} else {
		printf( "%d, %f, %f, %f, %f, %f\n", result.accesses, hitRate, missRate, compulsoryMissRate, capacityMissRate, conflictMissRate );
	}
}
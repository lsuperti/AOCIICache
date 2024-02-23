#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "FileHandler.h"
#include "Simulator.h"

enum outFlag_t {
	FREEFORM_OUT = 0,
	STANDARDIZED_OUT = 1
};

void printOutput( int flagOut, result_t result );
long handleNumberInput( char * input, int index );

int main( int argc, char *argv[] ) {
	char * quote = strchr( argv[ 0 ], ' ' ) == NULL ? "" : "\"";
	
	if ( argc != 7 ) {
		printf( "Número de argumentos incorreto. Utilize:\n" );
		printf( "%s%s%s <nsets> <bsize> <assoc> <substituição> <flag_saída> <arquivo_de_entrada>\n", quote, argv[ 0 ], quote );
		exit( EXIT_FAILURE );
	}

	uint32_t  nsets = ( uint32_t )handleNumberInput( argv[ 1 ], 1 );
	int       bsize = ( uint32_t )handleNumberInput( argv[ 2 ], 2 );
	int       assoc = ( uint32_t )handleNumberInput( argv[ 3 ], 3 );
	char *    subst = argv[ 4 ];
	int       flagOut = ( uint32_t )handleNumberInput( argv[ 5 ], 5 );
	char *    arquivoEntrada = argv[ 6 ];

	printf( "nsets = %d\n", nsets );
	printf( "bsize = %d\n", bsize );
	printf( "assoc = %d\n", assoc );
	printf( "subst = %s\n", subst );
	printf( "flagOut = %d\n", flagOut );
	printf( "arquivo = %s\n", arquivoEntrada );

    uint32_t * addresses;
    size_t size;
    
    handleFile( arquivoEntrada, &addresses, &size );

	result_t  result;
	
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

/* This function handles number parameter parsing and error checking.
 * 
 * It will use the strtol function to parse the input string.
 * 
 * The strtol function is considered safe to parse user-provided strings, as it will do error and security checks. (See commit #4d1c2bc (the commit that added this function) for more information and examples of potential exploits.)
 * 
 * The strtol function will automatically handle hexadecimal and octal numbers if the input string starts with "0x" or "0" respectively (and binary (0b) if the program is compiled with C2x).
 */
long handleNumberInput( char * input, int index ) {
	long number;
	char * endptr;
	const char * args[] = { NULL, "<nsets>", "<bsize>", "<assoc>", "<substituição>", "<flag_saída>", "<arquivo_de_entrada>" };

	static_assert( sizeof( long ) >= sizeof( uint32_t ), "A long isn't a least 32 bits that allow any acceptable parameter values parameters to be taken. Consider using long long instead." );
	static_assert( ERANGE != 0, "ERANGE (range-related errors) is 0, that means it will always report an error when handling the number input, since it checks if errno is ERANGE and errno is set to 0 before the string handling that could cause the error to ensure a previous error is not caught inadvertently. A solution is to set the errno to some other value, but that may cause problems elsewhere." );

	errno = 0;

	// Use base 0 to automatically allow hexadecimal and octal numbers (and binary if compiling with C2x).
	number = strtol( input, &endptr, 0 );
	
	if ( *endptr != '\0' || endptr == input || errno == ERANGE ) {
		printf( "Erro: argumento \"%s\" no parâmetro %s não é um número válido ou aceitável.\n", input, args[ index ] );
		exit( EXIT_FAILURE );
	}
	
	return number;
}
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include "FileHandler.h"
#include "Simulator.h"

enum outFlag_t {
    FREEFORM_OUT = 0,
    STANDARDIZED_OUT = 1
};

void  printOutput( result_t result, int flagOut );
long  parseNumberInput( char * input, int index );
int   parseReplacementPolicy( char * subst );

int main( int argc, char *argv[] ) {
    srand( time( NULL ) );
    char * quote = strchr( argv[ 0 ], ' ' ) == NULL ? "" : "\"";
    
    if ( argc != 7 ) {
        fprintf( stderr, "Número de argumentos incorreto. Utilize:\n"
                         "%s%s%s <nsets> <bsize> <assoc> <substituição> <flag_saída> <arquivo_de_entrada>\n", quote, argv[ 0 ], quote );
        exit( EXIT_FAILURE );
    }

    uint32_t    nsets = ( uint32_t )parseNumberInput( argv[ 1 ], 1 );
    uint32_t    bsize = ( uint32_t )parseNumberInput( argv[ 2 ], 2 );
    uint32_t    assoc = ( uint32_t )parseNumberInput( argv[ 3 ], 3 );
    char *      substString = argv[ 4 ];
    int         flagOut = ( int )parseNumberInput( argv[ 5 ], 5 );
    char *      arquivoEntrada = argv[ 6 ];
    uint32_t *  addresses;
    size_t      size;
    result_t    result;
    
    handleFile( arquivoEntrada, &addresses, &size );

    if ( assoc == 1 ) {
        result = simulateDirectMapping( addresses, size, bsize, nsets );
    } else {       
        int replacementPolicy = parseReplacementPolicy( substString );
        result = simulate( addresses, size, nsets, bsize, assoc, replacementPolicy );
    }

    printOutput( result, flagOut );

    free( addresses );

    return 0;
}

/*
 * This function prints the output of the simulation.
 *
 * It will print the output in a freeform or standardized format, depending on the flagOut parameter.
 * 
 * The freeform format is a human-readable format that prints the results in a more verbose way.
 * 
 * The standardized format is a machine-readable format that prints the results in a more concise way defined by the specification.
 */
void printOutput( result_t result, int flagOut ) {
    const float  hitRate = ( ( float ) result.hits / result.accesses );
    const int    totalMisses = result.capacityMisses + result.conflictMisses + result.compulsoryMisses;
    const float  missRate = ( ( float ) totalMisses / result.accesses );
    const float  compulsoryMissRate = ( ( float ) result.compulsoryMisses / totalMisses );
    const float  capacityMissRate = ( ( float ) result.capacityMisses / totalMisses );
    const float  conflictMissRate = ( ( float ) result.conflictMisses / totalMisses );
    
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
                totalMisses,
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
        printf( "%d, %.4f, %.4f, %.2f, %.2f, %.2f\n", result.accesses, hitRate, missRate, compulsoryMissRate, capacityMissRate, conflictMissRate );
    }
}

/* This function parses number parameters with and error checking.
 * 
 * It will use the strtoul function to parse the input string.
 * 
 * The strtoul function is considered safe to parse user-provided strings, as it will do error and security checks.
 * 
 * The strtoul function will automatically handle hexadecimal and octal numbers if the input string starts with "0x" or "0" respectively (and binary ("0b") if the program is compiled with C2x).
 */
long parseNumberInput( char * input, int index ) {
    unsigned long number;
    char *        endptr;
    const char *  args[] = { NULL, "<nsets>", "<bsize>", "<assoc>", "<substituição>", "<flag_saída>", "<arquivo_de_entrada>" };

    static_assert( sizeof( unsigned long ) >= sizeof( uint32_t ), "A unsigned long isn't a least 32 bits that allow any acceptable parameter values parameters to be taken. Consider using unsigned long long instead." );
    static_assert( ERANGE != 0, "ERANGE (range-related errors) is 0, that means it will always report an error when parsing the number input, since it checks if errno is ERANGE and errno is set to 0 before the string handling that could cause the error to ensure a previous error is not caught inadvertently. A solution is to set the errno to some other value, but that may cause problems elsewhere." );

    errno = 0;

    // Use base 0 to automatically allow hexadecimal and octal numbers (and binary if compiling with C2x).
    number = strtoul( input, &endptr, 0 );
    
    if ( *endptr != '\0' || endptr == input || errno == ERANGE ) {
        fprintf( stderr, "Erro: argumento \"%s\" no parâmetro %s não é um número válido ou aceitável.\n", input, args[ index ] );
        exit( EXIT_FAILURE );
    }
    
    return number;
}

/*
 * This function parses the replacement policy string and returns the corresponding enum value.
 * 
 * It accepts either uppercase or lowercase letters as it seems the specification requires that.
 * 
 * The accepted letters are: 'R' for RANDOM, 'L' for LRU, and 'F' for FIFO.
 */
int parseReplacementPolicy( char * subst ) {
    #define ASCII_CHAR_TO_UPPER( c ) ( ( c ) & 0xDF )
    
    if ( ASCII_CHAR_TO_UPPER( subst[ 0 ] ) == 'R' && subst[ 1 ] == '\0' ) {
        return RANDOM;
    } else if ( ASCII_CHAR_TO_UPPER( subst[ 0 ] ) == 'L' && subst[ 1 ] == '\0' ) {
        return LRU;
    } else if ( ASCII_CHAR_TO_UPPER( subst[ 0 ] ) == 'F' && subst[ 1 ] == '\0' ) {
        return FIFO;
    } else {
        fprintf( stderr, "Erro: política de substituição \"%s\" não é suportada.\n", subst );
        exit( EXIT_FAILURE );
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <time.h>

#include "CacheSimulator.h"
#include "FileHandler.h"
#include "Simulator.h"
#include "CacheConfig.h"

enum outFlag_t {
    FREEFORM_OUT = 0,
    STANDARDIZED_OUT = 1
};

void           printOutput( result_t * results, unsigned long cacheLevels, int flagOut );
unsigned long  parseNumberInput( char * input, int index, int level );
int            parseReplacementPolicy( char * subst );
unsigned long  parseCacheLevelSpecifier( char * input );

int main( int argc, char *argv[] ) {
    // Seed the random number generator
    srand( time( NULL ) );
    
    // Quote the executable path if it has spaces
    char * quote = strchr( argv[ 0 ], ' ' ) == NULL ? "" : "\"";
    
    #if COMPLIANCE_LEVEL < 2
    if ( argc < 7 ) {
        fprintf( stderr, "Número de argumentos incorreto. Utilize:\n"
                         "%s%s%s <nsets> <bsize> <assoc> <substituição> <flag_saída> <arquivo_de_entrada> [-l<level> <nsets> <bsize> <assoc> <substituição>]*\n", quote, argv[ 0 ], quote );
        exit( EXIT_FAILURE );
    }
    #else
    if ( argc != 7 ) {
        fprintf( stderr, "Número de argumentos incorreto. Utilize:\n"
                         "%s%s%s <nsets> <bsize> <assoc> <substituição> <flag_saída> <arquivo_de_entrada>\n", quote, argv[ 0 ], quote );
        exit( EXIT_FAILURE );
    }
    #endif

    // Get the parameters and first cache level configuration
    uint32_t             nsets = ( uint32_t )parseNumberInput( argv[ 1 ], 1, 1 );
    uint32_t             bsize = ( uint32_t )parseNumberInput( argv[ 2 ], 2, 1 );
    uint32_t             assoc = ( uint32_t )parseNumberInput( argv[ 3 ], 3, 1 );
    char *               substString = argv[ 4 ];
    int                  flagOut = ( int )parseNumberInput( argv[ 5 ], 5, 0 );
    char *               arquivoEntrada = argv[ 6 ];
    uint32_t *           addresses;
    size_t               size;
    result_t *           results;
    unsigned long        cacheLevel;
    cacheConfig_t        cacheConfig = { .nsets = nsets, .bsize = bsize, .assoc = assoc, .replacementPolicy = parseReplacementPolicy( substString ), .level = 1 };
    cacheConfigList_t *  cacheConfigList;
    unsigned long        numberOfCacheLevels = 1;
    
    initializeCacheConfigList( &cacheConfigList, &cacheConfig );

    #if COMPLIANCE_LEVEL < 2
    // Get lower cache levels
    for ( int i = 7; i < argc; i += 5 ) {
        if ( argv[ i ][ 0 ] == '-' && argv[ i ][ 1 ] == 'l' ) {
            cacheLevel = parseCacheLevelSpecifier( argv[ i ] );

            if ( argc < i + 5 ) {
                argv[ i ][ 1 ] -= 32; // Make the L uppercase to print the level correctly
                fprintf( stderr, "Número de argumentos incorreto na cache %s. Utilize:\n"
                                 "[-l<level> <nsets> <bsize> <assoc> <substituição>]\n", argv[ i ] + 1 );
                exit( EXIT_FAILURE );
            }

            nsets = ( uint32_t )parseNumberInput( argv[ i + 1 ], 1, cacheLevel );
            bsize = ( uint32_t )parseNumberInput( argv[ i + 2 ], 2, cacheLevel );
            assoc = ( uint32_t )parseNumberInput( argv[ i + 3 ], 3, cacheLevel );
            substString = argv[ i + 4 ];

            cacheConfig = ( cacheConfig_t ){ .nsets = nsets, .bsize = bsize, .assoc = assoc, .replacementPolicy = parseReplacementPolicy( substString ), .level = cacheLevel };

            pushCacheConfig( &cacheConfigList, &cacheConfig );

            numberOfCacheLevels++;
        }
    }
    #else
    // Avoid unused variable warnings on very strict compliance level, where cache levels are not supported
    ( void )cacheLevel;
    #endif

    verifyCacheConfig( cacheConfigList );
    
    handleFile( arquivoEntrada, &addresses, &size );

    if ( assoc == 1 && cacheConfigList->next == NULL ) {
        results = malloc( sizeof( result_t ) );

        if ( results == NULL ) {
            fputs( "Sem memória.\n", stderr );
            exit( EXIT_FAILURE );
        }   

        results[ 0 ] = simulateDirectMapping( addresses, size, cacheConfigList->cacheConfig.bsize, cacheConfigList->cacheConfig.nsets );
    } else {
        results = simulate( addresses, size, cacheConfigList );
    }

    printOutput( results, numberOfCacheLevels, flagOut );

    free( addresses );
    destroyCacheConfigList( cacheConfigList );
    free( results );

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
void printOutput( result_t * results, unsigned long cacheLevels, int flagOut ) {
    float  hitRate;
    int    totalMisses;
    float  missRate;
    float  compulsoryMissRate;
    float  capacityMissRate;
    float  conflictMissRate;
    
    for ( unsigned long i = 0; i < cacheLevels; i++ ) {
        hitRate = ( ( float )results[ i ].hits / results[ i ].accesses );
        totalMisses = results[ i ].capacityMisses + results[ i ].conflictMisses + results[ i ].compulsoryMisses;
        missRate = ( ( float ) totalMisses / results[ i ].accesses );
        compulsoryMissRate = ( ( float )results[ i ].compulsoryMisses / totalMisses );
        capacityMissRate = ( ( float )results[ i ].capacityMisses / totalMisses );
        conflictMissRate = ( ( float )results[ i ].conflictMisses / totalMisses );
        
        if ( flagOut == FREEFORM_OUT ) {
                printf( "========== L%lu ==========\n"
                        "Hits: %d\n"
                        "Misses: %d\n"
                        "Accesses: %d\n"
                        "Compulsory Misses: %d\n"
                        "Capacity Misses: %d\n"
                        "Conflict Misses: %d\n"
                        "Hit rate: %f\n"
                        "Miss rate: %f\n"
                        "Compulsory miss rate: %f\n"
                        "Capacity miss rate: %f\n"
                        "Conflict miss rate: %f\n",
                        i + 1,
                        results[ i ].hits,
                        totalMisses,
                        results[ i ].accesses,
                        results[ i ].compulsoryMisses,
                        results[ i ].capacityMisses,
                        results[ i ].conflictMisses,
                        hitRate,
                        missRate,
                        compulsoryMissRate,
                        capacityMissRate,
                        conflictMissRate );
        } else {
            printf( "%d, %.4f, %.4f, %.2f, %.2f, %.2f\n", results[ i ].accesses, hitRate, missRate, compulsoryMissRate, capacityMissRate, conflictMissRate );
        }
    }
}

/* This function parses number parameters with and error checking.
 * 
 * It will use the strtoul function to parse the input string.
 * 
 * The strtoul function is considered safe to parse user-provided strings, as it will do error and security checks.
 * 
 * The strtoul function will automatically handle hexadecimal and octal numbers if the input string starts with "0x" or
 * "0" respectively (and binary ("0b") if the program is compiled with C2x).
 */
unsigned long parseNumberInput( char * input, int index, int level ) {
    unsigned long  number;
    char *         endptr;
    const char *   args[] = { NULL, "<nsets>", "<bsize>", "<assoc>", "<substituição>", "<flag_saída>", "<arquivo_de_entrada>" };

    static_assert( sizeof( unsigned long ) >= sizeof( uint32_t ), "An unsigned long isn't a least 32 bits that allow any acceptable parameter values parameters to be taken. Consider using unsigned long long instead." );
    static_assert( ERANGE != 0, "ERANGE (range-related errors) is 0, that means it will always report an error when parsing the number input, since it checks if errno is ERANGE and errno is set to 0 before the string handling that could cause the error to ensure a previous error is not caught inadvertently. A solution is to set the errno to some other value, but that may cause problems elsewhere." );

    errno = 0;

    #if COMPLIANCE_LEVEL < 2
    if ( strlen( input ) >= 2 ) {
        // Binary
        if ( input[ 0 ] == '0' && ( input[ 1 ] == 'b' || input[ 1 ] == 'B' ) ) {
            number = strtoul( input + 2, &endptr, 2 );
        // Octal with prefix "0o"
        } else if ( input[ 0 ] == '0' && ( input[ 1 ] == 'o' || input[ 1 ] == 'O' ) ) {
            number = strtoul( input + 2, &endptr, 8 );
        // Hexadecimal
        } else if ( input[ 0 ] == '0' && ( input[ 1 ] == 'x' || input[ 1 ] == 'X' ) ) {
            number = strtoul( input + 2, &endptr, 16 );
        } else {
            #if COMPLIANCE_LEVEL < 1
            // Accept 0 as octal prefix
            number = strtoul( input, &endptr, 0 );
            #else
            // Accept only decimals if a prefix is not present
            number = strtoul( input, &endptr, 10 );
            #endif
        }
    } else {
        #if COMPLIANCE_LEVEL < 1
        // Accept 0 as octal prefix
        number = strtoul( input, &endptr, 0 );
        #else
        // Do not accept a single 0 as an octal prefix
        number = strtoul( input, &endptr, 10 );
        #endif
    }
    #else
    // Only decimals allowed in very strict compliance level
    number = strtoul( input, &endptr, 10 );
    #endif
    
    if ( *endptr != '\0' || endptr == input || errno == ERANGE ) {
        if ( level != 0 ) {
            fprintf( stderr, "Erro: argumento \"%s\" no parâmetro %s da cache L%d não é um número válido ou aceitável.\n", input, args[ index ], level );
            exit( EXIT_FAILURE );
        } else {
            fprintf( stderr, "Erro: argumento \"%s\" no parâmetro %s não é um número válido ou aceitável.\n", input, args[ index ] );
            exit( EXIT_FAILURE );
        }
    }
    
    return number;
}

/*
 * Parses a cache level specifier argument string and returns the corresponding cache level number.
 */
unsigned long parseCacheLevelSpecifier( char * input ) {
    unsigned long  number;
    char *         endptr;

    static_assert( ERANGE != 0, "ERANGE (range-related errors) is 0, that means it will always report an error when parsing the number input, since it checks if errno is ERANGE and errno is set to 0 before the string handling that could cause the error to ensure a previous error is not caught inadvertently. A solution is to set the errno to some other value, but that may cause problems elsewhere." );

    errno = 0;

    number = strtoul( input + 2, &endptr, 10 );
    
    if ( *endptr != '\0' || endptr == input || errno == ERANGE ) {
        fprintf( stderr, "Erro: especificador de nível de cache \"%s\" é inválido.\n", input );
        exit( EXIT_FAILURE );
    }
    
    return number;
}

/*
 * This function parses the replacement policy string and returns the corresponding enum value.
 * 
 * Accepts lowercase letters in addition to uppercase letters as an additional feature if the compliance level is not
 * very strict.
 * 
 * The accepted letters are: 'R' for RANDOM, 'L' for LRU, and 'F' for FIFO.
 */
int parseReplacementPolicy( char * subst ) {
    #if COMPLIANCE_LEVEL < 2
    #define ASCII_CHAR_TO_UPPER( c ) ( ( c ) & 0xDF )
    #else
    #define ASCII_CHAR_TO_UPPER( c ) ( c )
    #endif
    
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
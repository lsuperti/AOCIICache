#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

/*
 * Gets the size of a file.
*/
size_t getFileSize( FILE * file ) {
    size_t size = 0;
    
    if ( file != NULL ) {
        fseek( file, 0, SEEK_END );
        size = ftell( file );
        rewind( file );
    }
    
    return size;
}

/*
 * Gets the basename of a file path.
 *
 * Works with forward slash and backslash paths.
 *
 * The returned string is a pointer to a position in the original string.
 */
char * getFileBasename( char * filePath ) {
    // Get the last substring after a slash, which should be the file name
    char * basename = strrchr( filePath, '/' );

    // If there is no slash try the same with backslashes
    if ( basename == NULL ) {
        basename = strchr( filePath, '\\' );
    }

    // If there aren't any slashes or backslashes, the path is just the file name
    if ( basename == NULL ) {
        basename = filePath;
    }

    // Return the basename, if the full path is just the basename return it in full, if not, return the calculated basename + 1 so the slash or backslash is not included
    return basename != filePath ? basename + 1 : basename;
}

/*
 * Converts a 32-bit big-endian value to little-endian.
 */
static inline uint32_t bigEndianToLittleEndian( uint32_t value ) {
    return ( value >> 24 ) | ( ( value << 8 ) & 0x00FF0000 ) | ( ( value >> 8 ) & 0x0000FF00 ) | ( value << 24 );
}

/*
 * Reads a binary file containing 32-bit addressed and stores them in an array.
 *
 * The array is dynamically allocated, caller is responsible for freeing it.
 * 
 * Addresses is dereferenced with the newly allocated array and size is dereferenced with the number of elements in the array.
 */
void handleBinaryFile( char * filePath, uint32_t ** addresses, size_t * size ) {
    FILE * file = fopen( filePath, "rb" );
    
    if ( file == NULL ) {
        perror( filePath );
        exit( EXIT_FAILURE );
        return;
    }
    
    size_t fileSize = getFileSize( file );
    *size = fileSize / 4;

    // Checks if the file is composed of a whole number of 32-bit addresses
    if ( fileSize % 4 != 0 ) {
        fprintf( stderr, "%s: binary file is not composed of a whole number of 32-bit addressed.\n", filePath );
        exit( EXIT_FAILURE );
    }
    
    *addresses = malloc( *size * sizeof( uint32_t ) );

    if ( *addresses == NULL ) {
        fputs( "Sem memória.\n", stderr );
        exit( EXIT_FAILURE );
    }

    // Copies all the addresses to the array correcting the endianess
    for ( size_t i = 0; i < *size; i++ ) {
        uint32_t value;
        
        if ( fread( &value, sizeof( uint32_t ), 1, file ) < 1 ) {
            perror( filePath );
            exit( EXIT_FAILURE );
        }
        
        value = bigEndianToLittleEndian( value );
        ( *addresses )[ i ] = value;
    }

    fclose( file );
}

/*
 * Reads a text file containing 32-bit addresses in base 10 and stores them in an array.
 *
 * The array is dynamically allocated, caller is responsible for freeing it.
 * 
 * Values is dereferenced with the newly allocated array and size is dereferenced with the number of elements in the array.
 */
void handleTextFile( char * filePath, uint32_t ** values, size_t * size ) {
    FILE * file = fopen( filePath, "r" );
    
    if ( file == NULL ) {
        perror( filePath );
        exit( EXIT_FAILURE );
    }

    // The number of elements in the array is unknown since it can't be derived from the file size like in binary files, so 128 values are allocated initially
    size_t allocatedSize = 128;
    *values = malloc( allocatedSize * sizeof( uint32_t ) );

    if ( *values == NULL ) {
        fputs( "Sem memória.\n", stderr );
        exit( EXIT_FAILURE );
    }

    uint32_t value;
    *size = 0;

    // Reads all the values in the file and stores them in the array
    while ( fscanf( file, "%u", &value ) > 0 ) {     
        // Checks if there is space left in the allocated array, if there isn't, the array is reallocated with double the size
        if ( *size + 1 > allocatedSize ) {
            allocatedSize *= 2;
            *values = realloc( *values, allocatedSize * sizeof( uint32_t ) );

            if ( *values == NULL ) {
                fputs( "Sem memória.\n", stderr );
                exit( EXIT_FAILURE );
            }
        }

        ( *values )[ *size ] = value;
        ( *size )++;
    };

    fclose( file );
}

/*
 * Reads a binary or text file containing 32-bit addresses and stores them in an array.
 *
 * The file type is automatically detected based on the file extension, .bin for binary and .txt for text.
 *
 * The array is dynamically allocated, caller is responsible for freeing it.
 * 
 * values is dereferenced with the newly allocated array and size is dereferenced with the number of elements in the array.
 */
void handleFile( char * filePath, uint32_t ** values, size_t * size ) {
    // Get the file extension and base name
    char *  extension = strrchr( filePath, '.' );
    char *  basename = getFileBasename( filePath );
    
    if ( extension != NULL ) {
        // Handle binary files
        if ( strcmp( extension, ".bin" ) == 0 ) {
            handleBinaryFile( filePath, values, size );
        }
        // Handle text files
        else if ( strcmp( extension, ".txt" ) == 0 ) {
            handleTextFile( filePath, values, size );
        }
        // Unsupported file extension
        else {
            fprintf( stderr, "%s: a extensão \"%s\" não é suportada.\n", basename, extension );
            exit( EXIT_FAILURE );
        }
    // No file extension
    } else {
        fprintf( stderr, "%s: nome de arquivo inválido.\n", basename );
        exit( EXIT_FAILURE );
    }
}
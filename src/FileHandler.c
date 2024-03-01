#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

size_t getFileSize( FILE * file ) {
    size_t size = 0;
    
    if ( file != NULL ) {
        fseek( file, 0, SEEK_END );
        size = ftell( file );
        rewind( file );
    }
    
    return size;
}

static inline void bigEndianToLittleEndian( uint32_t * value ) {
    *value = ( *value >> 24 ) | ( ( *value << 8 ) & 0x00FF0000 ) | ( ( *value >> 8 ) & 0x0000FF00 ) | ( *value << 24 );
}

void handleBinaryFile( char * filename, uint32_t ** addresses, size_t * size ) {
    FILE * file = fopen( filename, "rb" );
    
    if ( file == NULL ) {
        perror( filename );
        exit( EXIT_FAILURE );
        return;
    }
    
    size_t fileSize = getFileSize( file );
    *size = fileSize / 4;

    if ( fileSize % 4 != 0 ) {
        fprintf( stderr, "Binary file is not composed of a whole number of 32-bit addressed.\n" );
        exit( EXIT_FAILURE );
    }
    
    *addresses = malloc( *size * sizeof( uint32_t ) );

    for ( size_t i = 0; i < *size; i++ ) {
        uint32_t value;
        
        fread( &value, sizeof( uint32_t ), 1, file );
        bigEndianToLittleEndian( &value );
        ( *addresses )[ i ] = value;
    }

    fclose( file );
}

void handleTextFile( char * filename, uint32_t ** values, size_t * size ) {
    FILE * file = fopen( filename, "r" );
    
    if ( file == NULL ) {
        perror( filename );
        exit( EXIT_FAILURE );
    }

    *values = malloc( 64 * sizeof( uint32_t ) );
    size_t allocatedSize = 64;

    *size = 0;
    do {     
        if ( *size + 1 > allocatedSize ) {
            allocatedSize *= 2;
            *values = realloc( *values, allocatedSize * sizeof( uint32_t ) );

            if ( *values == NULL ) {
                fprintf( stderr, "Out of memory.\n" );
                exit( EXIT_FAILURE );
            }
        }
    } while ( fscanf( file, "%u", &( *values )[ ( *size )++ ] ) > 0 );
    ( *size )--;

    fclose( file );
}

// Detects the file extension and calls the appropriate function to handle the file.
void handleFile( char* filename, uint32_t** values, size_t* size ) {
    
    // Get the file extension
    char * extension = strrchr( filename, '.' );
    
    if ( extension != NULL ) {
        // Check if the extension is .bin
        if ( strcmp( extension, ".bin" ) == 0 ) {
            handleBinaryFile( filename, values, size );
        }
        // Check if the extension is .txt
        else if ( strcmp(extension, ".txt" ) == 0) {
            handleTextFile( filename, values, size );
        }
        else {
            fprintf( stderr, "Unsupported file extension.\n" );
            exit(EXIT_FAILURE);
        }
    }
    else {
        fprintf( stderr, "Invalid file name.\n" );
        exit( EXIT_FAILURE );
    }
}
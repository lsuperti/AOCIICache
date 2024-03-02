#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include <inttypes.h>

void handleBinaryFile( char * filename, uint32_t ** addresses, size_t * size );
void handleTextFile( char * filename, uint32_t ** values, size_t * size );
void handleFile( char * filename, uint32_t ** values, size_t * size );
#endif
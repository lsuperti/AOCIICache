#ifndef SIMULADOR_H
#define SIMULADOR_H
typedef struct _result_t {
    int misses;
    int hits;
    int capacityMisses;
    int conflictMisses;
    int compulsoryMisses;
    int accesses;
} result_t;

result_t directMapping( uint32_t * addresses, size_t size, int bsize, int nsets );
#endif
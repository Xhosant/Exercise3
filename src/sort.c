#include <string.h>
#include "record.h"
#include "sort.h"
#include <math.h>
#include "chunk.h"

bool shouldSwap(Record* rec1,Record* rec2){
    int nameComp = strcmp(rec1->name, rec2->name);
    return nameComp > 0 || (nameComp == 0 && strcmp(rec1->surname, rec2->surname) > 0);
}

void sort_FileInChunks(int file_desc, int numBlocksInChunk){
    CHUNK_Iterator Cit = CHUNK_CreateIterator(file_desc, numBlocksInChunk);
    CHUNK ch;
    int rc = CHUNK_GetNext(&Cit, &ch);
    while(rc != -1) {
        sort_Chunk(&ch);
        for (int i = ch.from_BlockId; i <= ch.to_BlockId; i++) {
            HP_Unpin(file_desc,i);
        }
        rc = CHUNK_GetNext(&Cit, &ch);
    }

}

void sort_Chunk(CHUNK* chunk){
    int gap = chunk->recordsInChunk;
    Record rec1, rec2;
    //combsort
    double shrink = 1.3; ///experimentally ideal by wikipedia
    bool done = false;
    while(!done){
        gap = (int) floor(gap / shrink);
        if (gap < 1) {
            gap = 1;
            done = true;
        }
        else if (gap == 9 || gap == 10) {
            gap = 11;
        }
        int i = 0;
        int loopMax = chunk->recordsInChunk-gap;
        while (i < loopMax) {
            CHUNK_GetIthRecordInChunk(chunk, i, &rec1);
            CHUNK_GetIthRecordInChunk(chunk, i+gap, &rec2);
            if (shouldSwap(&rec1, &rec2)) {
                CHUNK_UpdateIthRecord(chunk, i, rec2);
                CHUNK_UpdateIthRecord(chunk, i+gap, rec1);
                done = false;
            }
            i++;
        }
    }
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bf.h"
#include "hp_file.h"
#include "record.h"
#include "sort.h"

#include <math.h>

#include "merge.h"
#include "chunk.h"

bool shouldSwap(Record* rec1,Record* rec2){
    int nameComp = strcmp(rec1->name, rec2->name);
    if(nameComp > 0) return true;
    if (nameComp < 0) return false;
    nameComp = strcmp(rec1->surname, rec2->surname);
    if(nameComp > 0) return true;
    return false;
}

void sort_FileInChunks(int file_desc, int numBlocksInChunk){
    CHUNK_Iterator Cit = CHUNK_CreateIterator(file_desc, numBlocksInChunk);
    CHUNK* ch = malloc(sizeof(CHUNK));
    CHUNK_GetNext(&Cit, ch);
    while(ch != NULL) {
        sort_Chunk(ch);
        CHUNK_GetNext(&Cit, ch);
    }
}

void sort_Chunk(CHUNK* chunk){
    int gap = chunk->recordsInChunk;
    Record* rec1 = malloc(sizeof(Record));
    Record* rec2 = malloc(sizeof(Record));
    //combsort
    double shrink = 1.3; ///experimentally ideal by wikipedia
    bool done = false;
    while(!done){
        gap = floor(gap / shrink);
        if (gap < 1) {
            gap = 1;
            done = true;
        }
        else if (gap == 9 || gap == 10) {
            gap = 11;
        }
        int i = 0;
        while (i+gap < chunk->recordsInChunk) {
            CHUNK_GetIthRecordInChunk(chunk, i, rec1);
            CHUNK_GetIthRecordInChunk(chunk, i+gap, rec2);
            if (shouldSwap(rec1, rec2)) {
                CHUNK_UpdateIthRecord(chunk, i, *rec2);
                CHUNK_UpdateIthRecord(chunk, i+gap, *rec1);
                done = false;
            }
            i++;
        }
    }
}
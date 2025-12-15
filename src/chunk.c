#include <merge.h>
#include <stdio.h>
#include "chunk.h"


CHUNK_Iterator CHUNK_CreateIterator(int fileDesc, int blocksInChunk){
    int block_id = 0; //TODO get proper first block ID
    int last_id = 0; //TODO get proper last block ID
    return {.file_desc = fileDesc, .current = block_id, .lastBlocksID = last_id ,.blocksInChunk = blocksInChunk};
}

int CHUNK_GetNext(CHUNK_Iterator *iterator,CHUNK* chunk){
    //must set chunk to NULL if there's no next
}

int CHUNK_GetIthRecordInChunk(CHUNK* chunk,  int i, Record* record){

}

int CHUNK_UpdateIthRecord(CHUNK* chunk,  int i, Record record){

}

void CHUNK_Print(CHUNK chunk){

}


CHUNK_RecordIterator CHUNK_CreateRecordIterator(CHUNK *chunk){

}

int CHUNK_GetNextRecord(CHUNK_RecordIterator *iterator,Record* record){
    //must set record to NULL if there's no next
}

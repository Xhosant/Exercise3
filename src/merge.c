#include <merge.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

typedef struct MERGE_inlet {
    Record* topRecord;
    CHUNK_RecordIterator recIt;
} MERGE_inlet;

int getNextRecordIndex(MERGE_inlet* inputArray, int arraySize) {
    int topIndex = 0;
    while (topIndex < arraySize && inputArray[topIndex].topRecord==NULL) {
        topIndex++;
    }
    if (topIndex == arraySize) {
       return -1;
    }
    Record* topRecord = inputArray[topIndex].topRecord;
    for (int i = topIndex+1; i<arraySize; i++) {
        if (inputArray[i].topRecord==NULL) continue;
        if (shouldSwap(topRecord, inputArray[i].topRecord)) {
            topRecord = inputArray[i].topRecord;
            topIndex=i;
        }
    }
    return topIndex;
}

Record* pop_inlet(MERGE_inlet in) {
    Record* rec = in.topRecord;
    CHUNK_GetNextRecord(&in.recIt,in.topRecord);
    return rec;
}

bool shouldSwap(Record* rec1,Record* rec2){
    int nameComp = strcmp(rec1->name, rec2->name);
    if(nameComp > 0) return true;
    if (nameComp < 0) return false;
    nameComp = strcmp(rec1->surname, rec2->surname);
    if(nameComp > 0) return true;
    return false;
}

void merge(int input_FileDesc, int chunkSize, int bWay, int output_FileDesc ){
    MERGE_inlet* inputArray = (MERGE_inlet*)malloc(sizeof(MERGE_inlet)*bWay);
    CHUNK_Iterator cIt = CHUNK_CreateIterator(input_FileDesc, chunkSize);
    bool done = false;
    CHUNK* chunk = malloc(sizeof(CHUNK));
    while (!done)
    {
        for (int i=0; i<bWay; i++) {
            CHUNK_GetNext(&cIt, chunk);
            if (chunk == NULL) {
                bWay = i-1;
                done = true;
                break;
            }
            inputArray[i].recIt = CHUNK_CreateRecordIterator(chunk);
        }
        //TODO open output block
        while (true) {
            int index = getNextRecordIndex(inputArray,bWay);
            if (index==-1) break;
            Record* rec = pop_inlet(inputArray[index]);
            //TODO add rec to output block
            //TODO if output block is now full, unpin it and replace it with the next output block
        }
    }
    free(inputArray);
    free(chunk);
}
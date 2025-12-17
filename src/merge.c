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

Record* pop_inlet(MERGE_inlet *in) {
    Record next;

    Record *old = in->topRecord;

    int rc = CHUNK_GetNextRecord(&in->recIt, &next);
    if (rc == -1) {
        in->topRecord = NULL;
    } else if (rc == 0) {
        *(in->topRecord) = next;   /* copy data */
    } else {
        in->topRecord = NULL;      /* error */
        return NULL;
    }

    return old;
}

bool shouldSwap(Record* rec1,Record* rec2){
    int nameComp = strcmp(rec1->name, rec2->name);
    return nameComp > 0 || (nameComp == 0 && strcmp(rec1->surname, rec2->surname) > 0);
}

void merge(int input_FileDesc, int chunkSize, int bWay, int output_FileDesc ){
    MERGE_inlet* inputArray = (MERGE_inlet*)malloc(sizeof(MERGE_inlet)*bWay);
    CHUNK_Iterator cIt = CHUNK_CreateIterator(input_FileDesc, chunkSize);
    bool done = false;
    CHUNK chunk;
    while (!done)
    {
        for (int i=0; i<bWay; i++) {
            if (CHUNK_GetNext(&cIt, &chunk)==-1){
                bWay = i;
                done = true;
                break;
            }
            inputArray[i].recIt = CHUNK_CreateRecordIterator(&chunk);
            inputArray[i].topRecord = malloc(sizeof(Record));
            pop_inlet(&inputArray[i]);
        }

        // Allocate the first output block
        BF_Block* outBlock;
        BF_Block_Init(&outBlock);
        CALL_BF(BF_AllocateBlock(output_FileDesc, outBlock));

        Record* outData = (Record*) BF_Block_GetData(outBlock);
        int recordsInOutBlock = 0; // number of records written into current block

        while (true) {
            int index = getNextRecordIndex(inputArray,bWay);
            if (index==-1) break;
            Record* rec = pop_inlet(&inputArray[index]);

            // Copy record into output block
            outData[recordsInOutBlock] = *rec;
            recordsInOutBlock++;

            // If the block is full, unpin it and allocate the next one
            if (recordsInOutBlock >= HP_GetMaxRecordsInBlock(output_FileDesc)) {
                BF_Block_SetDirty(outBlock);   // mark block dirty so it is written to disk
                CALL_BF(BF_UnpinBlock(outBlock));

                // Allocate next block
                CALL_BF(BF_AllocateBlock(output_FileDesc, outBlock));
                outData = (Record*) BF_Block_GetData(outBlock);
                recordsInOutBlock = 0;
            }
        }
        for (int i=0; i<bWay; i++) {
            free(inputArray[i].topRecord);
        }

        if (recordsInOutBlock > 0) {
            BF_Block_SetDirty(outBlock);
            CALL_BF(BF_UnpinBlock(outBlock));
        }
        BF_Block_Destroy(&outBlock);
    }
    free(inputArray);
}
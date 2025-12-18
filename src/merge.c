#include <merge.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sort.h>

typedef struct MERGE_inlet {
    Record* topRecord;
    CHUNK_RecordIterator recIt;
} MERGE_inlet;

bool merge_shouldSwap(Record* rec1,Record* rec2){
    int nameComp = strcmp(rec1->name, rec2->name);
    return nameComp > 0 || (nameComp == 0 && strcmp(rec1->surname, rec2->surname) > 0);
}

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
        if (merge_shouldSwap(topRecord, inputArray[i].topRecord)) {
            topRecord = inputArray[i].topRecord;
            topIndex=i;
        }
    }
    return topIndex;
}

Record* pop_inlet(MERGE_inlet *in) {
    Record next;

    int lastBlockID = in->recIt.currentBlockId;

    Record *old = in->topRecord;

    int rc = CHUNK_GetNextRecord(&in->recIt, &next);

    if (lastBlockID != in->recIt.currentBlockId) {
        HP_Unpin(in->recIt.chunk.file_desc, lastBlockID);
    }

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

static void inner_merge(int input_FileDesc, int chunkSize, int bWay, int output_FileDesc ){
    MERGE_inlet* inputArray = (MERGE_inlet*)malloc(sizeof(MERGE_inlet)*bWay);
    CHUNK_Iterator cIt = CHUNK_CreateIterator(input_FileDesc, chunkSize);
    bool done = false;
    CHUNK chunk;
    while (!done)
    {
        for (int j=chunk.from_BlockId; j<=chunk.to_BlockId; j++) {
            HP_Unpin(input_FileDesc, j);
        }
        for (int i=0; i<bWay; i++) {
            if (i>0) {

            }
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

        int recordsInOutBlock = 0; // number of records written into current block

        while (true) {
            int index = getNextRecordIndex(inputArray,bWay);
            if (index==-1) break;
            Record* rec = pop_inlet(&inputArray[index]);

            // Copy record into output block
            HP_InsertEntry(output_FileDesc, *rec);
            recordsInOutBlock++;

            // If the block is full, unpin it and allocate the next one
            if (recordsInOutBlock >= HP_GetMaxRecordsInBlock(output_FileDesc)) {
                HP_Unpin(output_FileDesc, HP_GetIdOfLastBlock(output_FileDesc));
                recordsInOutBlock = 0;
            }
        }
        for (int i=0; i<bWay; i++) {
            free(inputArray[i].topRecord);
        }

        HP_Unpin(output_FileDesc, HP_GetIdOfLastBlock(output_FileDesc));
    }
    free(inputArray);
}

// --- Merge wrapper: sorts then repeatedly merges ---
void merge(int input_FileDesc, int chunkSize, int bWay, int output_FileDesc) {
    int currentInput;
    int currentChunkSize = chunkSize;
    int pass = 0;

    // --- Step 0a: copy input file to temporary file ---
    char tempInput[256];
    snprintf(tempInput, sizeof(tempInput), "merge_pass_0_input.tmp");
    unlink(tempInput);
    int tempInputDesc;
    if (HP_CreateFile(tempInput) != 0) {
        fprintf(stderr, "Error creating temporary input file %s\n", tempInput);
        exit(EXIT_FAILURE);
    }
    if (HP_OpenFile(tempInput, &tempInputDesc) != 0) {
        fprintf(stderr, "Error opening temporary input file %s\n", tempInput);
        exit(EXIT_FAILURE);
    }

    int lastBlock = HP_GetIdOfLastBlock(input_FileDesc);
    for (int blk = 1; blk <= lastBlock; blk++) {
        int numRecords = HP_GetRecordCounter(input_FileDesc, blk);
        Record rec;
        for (int i = 0; i < numRecords; i++) {
            HP_GetRecord(input_FileDesc, blk, i, &rec);
            HP_InsertEntry(tempInputDesc, rec);
        }
        HP_Unpin(input_FileDesc, blk);
    }

    HP_CloseFile(tempInputDesc);

    currentInput = HP_OpenFile(tempInput, &tempInputDesc) == 0 ? tempInputDesc : -1;
    if (currentInput == -1) {
        fprintf(stderr, "Error reopening temporary input file\n");
        exit(EXIT_FAILURE);
    }

    // --- Step 0b: initial chunk sort ---
    sort_FileInChunks(tempInputDesc, currentChunkSize);

    // --- Iterative merge passes ---
    while (1) {
        char tempOutput[256];
        snprintf(tempOutput, sizeof(tempOutput), "merge_pass_%d.tmp", pass);

        unlink(tempOutput);
        int outputDesc;
        if (HP_CreateFile(tempOutput) != 0) {
            fprintf(stderr, "Error creating temp output file %s\n", tempOutput);
            exit(EXIT_FAILURE);
        }
        if (HP_OpenFile(tempOutput, &outputDesc) != 0) {
            fprintf(stderr, "Error opening temp output file %s\n", tempOutput);
            exit(EXIT_FAILURE);
        }

        inner_merge(currentInput, currentChunkSize, bWay, outputDesc);

        HP_CloseFile(currentInput);

        lastBlock = HP_GetIdOfLastBlock(outputDesc);

        if (lastBlock + 1 <= currentChunkSize) {
            // Fully merged: copy directly into the provided output_FileDesc
            for (int blk = 1; blk <= lastBlock; blk++) {
                int numRecords = HP_GetRecordCounter(outputDesc, blk);
                Record rec;
                for (int i = 0; i < numRecords; i++) {
                    HP_GetRecord(outputDesc, blk, i, &rec);
                    HP_InsertEntry(output_FileDesc, rec);
                }
                HP_Unpin(outputDesc, blk);
            }

            HP_CloseFile(outputDesc);

            // Delete the last temporary merge output
            char lastTemp[256];
            snprintf(lastTemp, sizeof(lastTemp), "merge_pass_%d.tmp", pass);
            unlink(lastTemp);

            break;
        }
        // Delete previous temporary input file
        if (pass == 0) {
            unlink(tempInput);
        } else {
            char prevTemp[256];
            snprintf(prevTemp, sizeof(prevTemp), "merge_pass_%d.tmp", pass - 1);
            unlink(prevTemp);
        }

        currentInput = outputDesc;
        currentChunkSize *= bWay;
        pass++;
    }
}
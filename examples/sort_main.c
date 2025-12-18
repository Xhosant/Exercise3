#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"
#include "sort.h"
#include "merge.h"

#define RECORDS_NUM 500
#define INPUT_NAME  "data.db"

static int createAndPopulateHeapFile(const char* filename);
static void printHeapFile(int file_desc, int maxPrint);

static void make_temp_name(char *buf, size_t n, int pass) {
    snprintf(buf, n, "tmp_pass_%d.db", pass);
}

int main() {
    int chunkSize = 5;   /* in blocks */
    int bWay = 4;

    BF_Init(LRU);

    unlink(INPUT_NAME);

    int inDesc = createAndPopulateHeapFile(INPUT_NAME);

    /* PASS 0: sort each chunk in-place */
    sort_FileInChunks(inDesc, chunkSize);

    /* MULTI-PASS MERGE:
       runSizeBlocks = chunkSize initially
       each pass: merge bWay runs -> new runSizeBlocks *= bWay
    */
    int runSizeBlocks = chunkSize;
    int pass = 1;

    while (1) {
        int lastBlock = HP_GetIdOfLastBlock(inDesc);
        int dataBlocks = lastBlock; /* block0 is metadata, blocks [1..lastBlock] are data */

        if (dataBlocks <= runSizeBlocks) {
            /* one run remains => fully sorted */
            break;
        }

        char outName[64];
        make_temp_name(outName, sizeof(outName), pass);
        unlink(outName);

        int outDesc;
        HP_CreateFile(outName);
        HP_OpenFile(outName, &outDesc);

        merge(inDesc, runSizeBlocks, bWay, outDesc);

        HP_CloseFile(inDesc);
        inDesc = outDesc;

        runSizeBlocks *= bWay;
        pass++;
    }

    printf("=== Sorted Records (GLOBAL) ===\n");
    printHeapFile(inDesc, -1);

    HP_CloseFile(inDesc);
    CALL_BF(BF_Close());
    return 0;
}

static int createAndPopulateHeapFile(const char* filename) {
    HP_CreateFile((char*)filename);

    int fd;
    HP_OpenFile((char*)filename, &fd);

    srand(12569874);
    for (int i = 0; i < RECORDS_NUM; i++) {
        Record record = randomRecord();
        HP_InsertEntry(fd, record);
    }

    return fd;
}

static void printHeapFile(int file_desc, int maxPrint) {
    int printed = 0;
    int lastBlock = HP_GetIdOfLastBlock(file_desc);

    for (int blk = 1; blk <= lastBlock && (maxPrint == -1 || printed < maxPrint); blk++) {
        int numRecords = HP_GetRecordCounter(file_desc, blk);

        for (int i = 0; i < numRecords && (maxPrint == -1 || printed < maxPrint); i++) {
            Record rec;
            if (HP_GetRecord(file_desc, blk, i, &rec) == 0) {
                HP_Unpin(file_desc, blk);
                printf("Record: %-10s %-14s %d\n", rec.name, rec.surname, rec.id);
                printed++;
            } else {
                HP_Unpin(file_desc, blk);
            }
        }
    }
}

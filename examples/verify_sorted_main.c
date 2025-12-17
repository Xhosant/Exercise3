#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"
#include "sort.h"
#include "merge.h"

#define RECORDS_NUM 50000

static void create_and_populate(const char *filename) {
    int fd;
    HP_CreateFile((char*)filename);
    HP_OpenFile((char*)filename, &fd);

    srand(12569874);
    for (int i = 0; i < RECORDS_NUM; i++) {
        Record r = randomRecord();
        HP_InsertEntry(fd, r);
    }
    HP_CloseFile(fd);
}

static int is_single_run(int file_desc, int runSizeBlocks) {
    int lastBlock = HP_GetIdOfLastBlock(file_desc);
    int dataBlocks = lastBlock;
    return dataBlocks <= runSizeBlocks;
}

/* Compare ONLY by (name, surname) as required by the assignment */
static int cmp_records(const Record *a, const Record *b) {
    int c = strcmp(a->name, b->name);
    if (c != 0) return c;
    return strcmp(a->surname, b->surname);
}

static int verify_sorted(int file_desc) {
    int lastBlock = HP_GetIdOfLastBlock(file_desc);

    Record prev, cur;
    int havePrev = 0;

    for (int blk = 1; blk <= lastBlock; blk++) {
        int nrec = HP_GetRecordCounter(file_desc, blk);

        for (int i = 0; i < nrec; i++) {
            if (HP_GetRecord(file_desc, blk, i, &cur) != 0) {
                HP_Unpin(file_desc, blk);
                printf("FAIL: could not read record blk=%d i=%d\n", blk, i);
                return 0;
            }
            HP_Unpin(file_desc, blk);

            if (havePrev && cmp_records(&prev, &cur) > 0) {
                printf("FAIL: not sorted by (name, surname)!\n");
                printf("Prev: %s %s %d\n", prev.name, prev.surname, prev.id);
                printf("Cur : %s %s %d\n", cur.name, cur.surname, cur.id);
                return 0;
            }
            prev = cur;
            havePrev = 1;
        }
    }

    printf("OK: file is globally sorted by (name, surname).\n");
    return 1;
}

int main(void) {
    BF_Init(LRU);

    unlink("verify_in.db");
    create_and_populate("verify_in.db");

    int inFd;
    HP_OpenFile((char*)"verify_in.db", &inFd);

    int chunkSize = 5;
    int bWay = 10;

    sort_FileInChunks(inFd, chunkSize);

    int runSizeBlocks = chunkSize;
    int passes = 0;

    while (!is_single_run(inFd, runSizeBlocks)) {
        char outName[64];
        snprintf(outName, sizeof(outName), "verify_out_pass_%d.db", passes);
        unlink(outName);

        int outFd;
        HP_CreateFile(outName);
        HP_OpenFile(outName, &outFd);

        merge(inFd, runSizeBlocks, bWay, outFd);

        HP_CloseFile(inFd);
        HP_CloseFile(outFd);

        HP_OpenFile(outName, &inFd);

        runSizeBlocks *= bWay;
        passes++;
    }

    printf("merge passes: %d\n", passes);
    verify_sorted(inFd);

    HP_CloseFile(inFd);
    CALL_BF(BF_Close());
    return 0;
}

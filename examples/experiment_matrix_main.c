#include <stdio.h>
#include <stdlib.h>
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

static void make_name(char *buf, size_t n, const char *prefix, int pass) {
    snprintf(buf, n, "%s_pass_%d.db", prefix, pass);
}

static int is_single_run(int file_desc, int runSizeBlocks) {
    int lastBlock = HP_GetIdOfLastBlock(file_desc);
    int dataBlocks = lastBlock; /* blocks [1..lastBlock] */
    return dataBlocks <= runSizeBlocks;
}

static int run_once(const char *baseInput, int chunkSize, int bWay) {
    char workName[64];
    snprintf(workName, sizeof(workName), "work_c%d_b%d.db", chunkSize, bWay);
    unlink(workName);

    /* regenerate deterministic input for fairness */
    create_and_populate(workName);

    int inFd;
    HP_OpenFile((char*)workName, &inFd);

    sort_FileInChunks(inFd, chunkSize);

    int runSizeBlocks = chunkSize;
    int passes = 0;

    while (!is_single_run(inFd, runSizeBlocks)) {
        char outName[64];
        make_name(outName, sizeof(outName), "tmp_matrix", passes);
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

    HP_CloseFile(inFd);
    return passes;
}

int main(void) {
    BF_Init(LRU);

    /* Choose sets you want to compare */
    int chunkSizes[] = { 3, 5, 8, 10 };
    int bWays[]      = { 2, 3, 4, 5, 10 };
    int csN = (int)(sizeof(chunkSizes)/sizeof(chunkSizes[0]));
    int bwN = (int)(sizeof(bWays)/sizeof(bWays[0]));

    printf("Records: %d\n", RECORDS_NUM);
    printf("Passes table (rows=chunkSize, cols=bWay):\n\n");

    printf("chunk\\bWay");
    for (int j = 0; j < bwN; j++) printf("\t%d", bWays[j]);
    printf("\n");

    for (int i = 0; i < csN; i++) {
        printf("%d", chunkSizes[i]);
        for (int j = 0; j < bwN; j++) {
            int passes = run_once("records.db", chunkSizes[i], bWays[j]);
            printf("\t%d", passes);
        }
        printf("\n");
    }

    CALL_BF(BF_Close());
    return 0;
}

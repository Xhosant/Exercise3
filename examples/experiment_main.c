#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"
#include "sort.h"
#include "merge.h"

/* how many records to generate for the experiment */
#define RECORDS_NUM 50000

static int create_and_populate(const char *filename);
static void run_experiment(const char *inputFile, int chunkSize, int bWay);

static void make_name(char *buf, size_t n, const char *prefix, int pass) {
    snprintf(buf, n, "%s_pass_%d.db", prefix, pass);
}

/* returns 1 when the file consists of exactly one sorted run of size runSizeBlocks */
static int is_single_run(int file_desc, int runSizeBlocks) {
    int lastBlock = HP_GetIdOfLastBlock(file_desc); /* data blocks are [1..lastBlock] */
    int dataBlocks = lastBlock;                     /* since block 0 is metadata */
    return dataBlocks <= runSizeBlocks;
}

int main(void) {
    BF_Init(LRU);

    unlink("records.db");

    /* create input once (same data for all experiments) */
    create_and_populate("records.db");

    /* required experiments: chunkSize=5, 2-way vs 10-way */
    run_experiment("records.db", 5, 2);
    run_experiment("records.db", 5, 10);

    CALL_BF(BF_Close());
    return 0;
}

static int create_and_populate(const char *filename) {
    int fd;

    HP_CreateFile((char *)filename);
    HP_OpenFile((char *)filename, &fd);

    srand(12569874);
    for (int i = 0; i < RECORDS_NUM; i++) {
        Record r = randomRecord();
        HP_InsertEntry(fd, r);
    }

    HP_CloseFile(fd);
    return 0;
}

static void run_experiment(const char *inputFile, int chunkSize, int bWay) {
    printf("\n--- Experiment ---\n");
    printf("chunkSize = %d blocks, bWay = %d, records = %d\n", chunkSize, bWay, RECORDS_NUM);

    /* We'll work on a fresh copy of the input for each experiment */
    char workInName[64];
    snprintf(workInName, sizeof(workInName), "work_%dway.db", bWay);
    unlink(workInName);

    /* Create working copy by regenerating same data (deterministic seed) */
    create_and_populate(workInName);

    int inFd;
    HP_OpenFile((char *)workInName, &inFd);

    /* PASS 0: sort each chunk in-place */
    sort_FileInChunks(inFd, chunkSize);

    int runSizeBlocks = chunkSize;
    int passes = 0;

    while (!is_single_run(inFd, runSizeBlocks)) {
        char outName[64];
        make_name(outName, sizeof(outName), "tmp_sorted", passes);
        unlink(outName);

        int outFd;
        HP_CreateFile(outName);
        HP_OpenFile(outName, &outFd);

        /* Your merge() returns void in this project */
        merge(inFd, runSizeBlocks, bWay, outFd);

        HP_CloseFile(inFd);
        HP_CloseFile(outFd);

        /* Next pass uses output as input */
        HP_OpenFile(outName, &inFd);

        runSizeBlocks *= bWay;
        passes++;
    }

    printf("Total merge passes: %d\n", passes);
    printf("------------------\n");

    HP_CloseFile(inFd);
}

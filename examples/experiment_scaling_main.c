#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"
#include "sort.h"
#include "merge.h"

static void create_and_populate(const char *filename, int records) {
    int fd;
    HP_CreateFile((char*)filename);
    HP_OpenFile((char*)filename, &fd);

    srand(12569874);
    for (int i = 0; i < records; i++) {
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
    int dataBlocks = lastBlock;
    return dataBlocks <= runSizeBlocks;
}

static int count_passes_for(const char *workName, int chunkSize, int bWay) {
    int inFd;
    HP_OpenFile((char*)workName, &inFd);

    sort_FileInChunks(inFd, chunkSize);

    int runSizeBlocks = chunkSize;
    int passes = 0;

    while (!is_single_run(inFd, runSizeBlocks)) {
        char outName[64];
        make_name(outName, sizeof(outName), "tmp_scale", passes);
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

    int chunkSize = 5;
    int bWay = 2;

    int sizes[] = { 10000, 50000, 100000 };
    int n = (int)(sizeof(sizes)/sizeof(sizes[0]));

    printf("Scaling experiment: chunkSize=%d, bWay=%d\n", chunkSize, bWay);
    printf("records\tpasses\n");

    for (int i = 0; i < n; i++) {
        char workName[64];
        snprintf(workName, sizeof(workName), "work_scale_%d.db", sizes[i]);
        unlink(workName);

        create_and_populate(workName, sizes[i]);

        int passes = count_passes_for(workName, chunkSize, bWay);
        printf("%d\t%d\n", sizes[i], passes);
    }

    CALL_BF(BF_Close());
    return 0;
}

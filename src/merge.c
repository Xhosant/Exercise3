#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "merge.h"
#include "sort.h"
#include "chunk.h"
#include "hp_file.h"

typedef struct {
    Record *top;
    CHUNK_RecordIterator it;
} MergeInlet;

static int findMin(MergeInlet *arr, int n) {
    int idx = -1;

    for (int i = 0; i < n; i++) {
        if (arr[i].top == NULL) continue;
        if (idx == -1 || shouldSwap(arr[idx].top, arr[i].top)) {
            idx = i;
        }
    }
    return idx;
}

void merge(int input_FileDesc, int chunkSize, int bWay, int output_FileDesc) {
    CHUNK_Iterator cIt = CHUNK_CreateIterator(input_FileDesc, chunkSize);
    MergeInlet *inlets = malloc(sizeof(MergeInlet) * bWay);
    CHUNK chunk;

    while (1) {
        int active = 0;

        for (int i = 0; i < bWay; i++) {
            if (CHUNK_GetNext(&cIt, &chunk) == -1) break;

            inlets[i].it = CHUNK_CreateRecordIterator(&chunk);
            inlets[i].top = malloc(sizeof(Record));

            if (CHUNK_GetNextRecord(&inlets[i].it, inlets[i].top) != 0) {
                free(inlets[i].top);
                inlets[i].top = NULL;
            }
            active++;
        }

        if (active == 0) break;

        while (1) {
            int idx = findMin(inlets, active);
            if (idx == -1) break;

            HP_InsertEntry(output_FileDesc, *inlets[idx].top);

            if (CHUNK_GetNextRecord(&inlets[idx].it, inlets[idx].top) != 0) {
                free(inlets[idx].top);
                inlets[idx].top = NULL;
            }
        }
    }

    free(inlets);
}

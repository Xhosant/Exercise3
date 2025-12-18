#include "chunk.h"

/* -------------------------------------------------------------
 * CHUNK ITERATOR
 * ------------------------------------------------------------- */

/* Create a CHUNK iterator */
CHUNK_Iterator CHUNK_CreateIterator(int fileDesc, int blocksInChunk) {
    CHUNK_Iterator it;
    it.file_desc = fileDesc;
    it.blocksInChunk = blocksInChunk;
    it.current = 1;  /* block 0 is usually metadata */
    it.lastBlocksID = HP_GetIdOfLastBlock(fileDesc);
    return it;
}

/* Get next chunk from iterator */
int CHUNK_GetNext(CHUNK_Iterator *iterator, CHUNK *chunk) {
    if (iterator->current > iterator->lastBlocksID) {
        return -1;
    }

    chunk->file_desc = iterator->file_desc;
    chunk->from_BlockId = iterator->current;

    chunk->to_BlockId =
        iterator->current + iterator->blocksInChunk - 1;

    if (chunk->to_BlockId > iterator->lastBlocksID) {
        chunk->to_BlockId = iterator->lastBlocksID;
    }

    chunk->blocksInChunk =
        chunk->to_BlockId - chunk->from_BlockId + 1;

    chunk->recordsInChunk = 0;

    for (int b = chunk->from_BlockId; b <= chunk->to_BlockId; b++) {
        chunk->recordsInChunk +=
            HP_GetRecordCounter(iterator->file_desc, b);
    }

    iterator->current = chunk->to_BlockId + 1;
    return 0;
}

/* -------------------------------------------------------------
 * RECORD ACCESS IN CHUNK
 * ------------------------------------------------------------- */

/* Get ith record in chunk (0-based) */
int CHUNK_GetIthRecordInChunk(CHUNK *chunk, int i, Record *record) {
    int counter = 0;

    for (int b = chunk->from_BlockId; b <= chunk->to_BlockId; b++) {
        int recs = HP_GetRecordCounter(chunk->file_desc, b);

        if (i < counter + recs) {
            int cursor = i - counter;
            if (HP_GetRecord(chunk->file_desc, b, cursor, record) < 0) {
                return -1;
            }
            return 0;
        }
        counter += recs;
    }
    return -1;
}

/* Update ith record in chunk (0-based) */
int CHUNK_UpdateIthRecord(CHUNK *chunk, int i, Record record) {
    int counter = 0;

    for (int b = chunk->from_BlockId; b <= chunk->to_BlockId; b++) {
        int recs = HP_GetRecordCounter(chunk->file_desc, b);

        if (i < counter + recs) {
            int cursor = i - counter;
            if (HP_UpdateRecord(chunk->file_desc, b, cursor, record) < 0) {
                return -1;
            }
            return 0;
        }
        counter += recs;
    }
    return -1;
}

/* -------------------------------------------------------------
 * PRINTING
 * ------------------------------------------------------------- */

void CHUNK_Print(CHUNK chunk) {
    printf("CHUNK [%d -> %d] blocks=%d records=%d\n",
           chunk.from_BlockId,
           chunk.to_BlockId,
           chunk.blocksInChunk,
           chunk.recordsInChunk);

    for (int b = chunk.from_BlockId; b <= chunk.to_BlockId; b++) {
        HP_PrintBlockEntries(chunk.file_desc, b);
    }
}

/* -------------------------------------------------------------
 * RECORD ITERATOR
 * ------------------------------------------------------------- */

CHUNK_RecordIterator CHUNK_CreateRecordIterator(CHUNK *chunk) {
    CHUNK_RecordIterator it;
    it.chunk = *chunk;
    it.currentBlockId = chunk->from_BlockId;
    it.cursor = 0;
    return it;
}

int CHUNK_GetNextRecord(CHUNK_RecordIterator *iterator, Record *record) {
    while (iterator->currentBlockId <= iterator->chunk.to_BlockId) {
        int recs = HP_GetRecordCounter(
            iterator->chunk.file_desc,
            iterator->currentBlockId
        );

        if (iterator->cursor < recs) {
            if (HP_GetRecord(iterator->chunk.file_desc,
                             iterator->currentBlockId,
                             iterator->cursor,
                             record) < 0) {
                return -2;
            }
            iterator->cursor++;
            return 0;
        } else {
            iterator->currentBlockId++;
            iterator->cursor = 0;
        }
    }
    return -1;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "merge.h"
#include "hp_file.h"
#include "bf.h"
#include "record.h"

#define RECORDS_NUM 500 // you can change it if you want
#define FILE_NAME "data.db"
#define OUT_NAME "out"

int createAndPopulateHeapFile(char* filename);
void printHeapFile(int file_desc);

int main() {
  int chunkSize=5;
  int bWay= 4;
  int outputDesc;

  // Initialize buffer manager
  BF_Init(LRU);

  // Remove existing files
  unlink(FILE_NAME);
  unlink(OUT_NAME);

  // Create output file
  HP_CreateFile(OUT_NAME);
  HP_OpenFile(OUT_NAME, &outputDesc);

  // Create and populate input file
  int file_desc = createAndPopulateHeapFile(FILE_NAME);

  // Run merge sort
  merge(file_desc, chunkSize, bWay, outputDesc);

  // Print sorted output
  printf("=== Sorted Records ===\n");
  printHeapFile(outputDesc);

  // Close files
  HP_CloseFile(file_desc);
  HP_CloseFile(outputDesc);

  // Close buffer manager
  CALL_BF(BF_Close());

  return 0;
}

int createAndPopulateHeapFile(char* filename){
  HP_CreateFile(filename);
  
  int file_desc;
  HP_OpenFile(filename, &file_desc);

  Record record;
  srand(12569874);
  for (int id = 0; id < RECORDS_NUM; ++id)
  {
    record = randomRecord();
    HP_InsertEntry(file_desc, record);
  }
  return file_desc;
}

void printHeapFile(int file_desc, int maxPrint = -1) {
  int printed = 0;
  int lastBlock = HP_GetIdOfLastBlock(file_desc);

  for (int blk = 0; blk <= lastBlock && (maxPrint==-1 || printed < maxPrint); blk++) {
    int numRecords = HP_GetRecordCounter(file_desc, blk);
    for (int i = 0; i < numRecords && (maxPrint==-1 || printed < maxPrint); i++) {
      Record rec;
      if (HP_GetRecord(file_desc, blk, i, &rec) == 0) {
        printf("Record: %-10s %-10s %d\n", rec.name, rec.surname, rec.id);
        printed++;
      }
    }
  }

  if (printed == maxPrint) {
    printf("... (output truncated, first %d records shown)\n", maxPrint);
  }
}
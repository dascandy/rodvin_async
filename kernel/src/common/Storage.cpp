#include "device.h"
#include "Storage.h"
#include "mbr.h"
#include "fat.h"
#include <stdio.h>

bool storageIsFat(Storage* s, DiskBlock*);
bool storageIsMbr(Storage* s, DiskBlock*);

void register_storage(Storage* storage) {
  printf("got storage\n");
  storage->readBlock(0, 1).then([storage](future<DiskBlock*>f){
    DiskBlock* firstSector = f.get();
    if (storageIsFat(storage, firstSector)) {
      printf("is fat\n");
      new FatFilesystem(storage, firstSector);
    } else if (storageIsMbr(storage, firstSector)) {
      printf("is mbr\n");
      new MBRPartitioning(storage, firstSector);
    }
    delete firstSector;
  });
}



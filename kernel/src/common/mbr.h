#ifndef RAMDISK_H
#define RAMDISK_H

#include "Storage.h"

struct MBRPartition;

class MBRPartitioning {
public:
  MBRPartitioning(Storage* parent, DiskBlock* firstSector);
private:
  MBRPartition **parts;
};

#endif



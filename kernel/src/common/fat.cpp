#include "fat.h"
#include "Storage.h"
#include <string.h>
#include <stdio.h>
#include <algorithm>

struct fat_bootsect
{
  uint8_t start[11];
  uint8_t bytespersect[2];
  uint8_t sectpercluster;
  uint16_t reserved_sectors;
  uint8_t table_count;
  uint8_t root_entries[2];
  uint8_t sectors_16[2];
  uint8_t mediatype;
  uint16_t table_size;
  uint8_t pad[8];
  uint32_t sectors_32;
  uint32_t table_size2;
  uint8_t pad2[4];
  uint32_t rootDirCluster;
};

bool storageIsFat(Storage* , DiskBlock* firstSector) {
  fat_bootsect* fb = (fat_bootsect*)firstSector->data;
  bool isFat = true;

  if (fb->start[0] != 0xEB) isFat = false;
  if (fb->start[2] != 0x90) isFat = false;
  if (fb->mediatype != 0xF8) isFat = false;
  if (fb->bytespersect[0] != 0) isFat = false;
  if (fb->bytespersect[1] != 2 && fb->bytespersect[1] != 16) isFat = false;
  if (fb->table_count != 1 && fb->table_count != 2) isFat = false;
  if (fb->sectors_32 < 65535 && fb->sectors_32 != 0) isFat = false;
  if (fb->sectors_16[1] < 15 && fb->sectors_16[1] != 0) isFat = false;

  return isFat;
}

FatFilesystem::FatFilesystem(Storage *disk, DiskBlock* firstSector)
: disk(disk)
{
  fat_bootsect* boot = reinterpret_cast<fat_bootsect*>(firstSector->data);
  uint32_t sectors = boot->sectors_16[0] | (boot->sectors_16[1] << 8);
  if (sectors == 0) sectors = boot->sectors_32;
  if (sectors < 65525) {
    fat_version = Fat16;
    rootDirCluster = 1;
    root_entry_count = boot->root_entries[0] | (boot->root_entries[1] << 8);
    sect_per_clus = boot->sectpercluster;
    reserved_sect = boot->reserved_sectors;
    root_directory = reserved_sect + boot->table_count * boot->table_size;
    first_sect = reserved_sect + boot->table_count * boot->table_size + root_entry_count / 16;
  } else if (sectors < 268435445) {
    fat_version = Fat32;
    rootDirCluster = boot->rootDirCluster;
    root_entry_count = 0;
    sect_per_clus = boot->sectpercluster;
    reserved_sect = boot->reserved_sectors;
    first_sect = reserved_sect + boot->table_count * boot->table_size2 + root_entry_count / 16;
  } else {
    fat_version = ExFat;
    rootDirCluster = boot->rootDirCluster;
    root_entry_count = 0;
    sect_per_clus = boot->sectpercluster;
    reserved_sect = boot->reserved_sectors;
    first_sect = reserved_sect + boot->table_count * boot->table_size2 + root_entry_count / 16;
  }
  delete firstSector;

  Vfs::Register(this);
}

future<uint32_t> FatFilesystem::nextCluster(uint32_t cluster) {
  int entriesPerSector = 512 / (fat_version == Fat16 ? 2 : 4);
  uint32_t sector = (cluster / entriesPerSector) + reserved_sect;
  uint32_t offset = cluster % entriesPerSector;
  return disk->readBlock(sector, 1).then([this, offset, entriesPerSector](future<DiskBlock*> fd){
    DiskBlock* block = fd.get();
    uint32_t newCluster;
    if (entriesPerSector == 2)
      newCluster = reinterpret_cast<uint16_t*>(block->data)[offset];
    else
      newCluster = reinterpret_cast<uint32_t*>(block->data)[offset];
    
    if (fat_version == Fat32) newCluster &= 0xFFFFFFF;
    delete block;
    return newCluster;
  });
}

future<DiskBlock*> FatFilesystem::readCluster(uint32_t cluster) {
  if (cluster == 1) {
    return disk->readBlock(root_directory, root_entry_count / 16);
  } else {
    return disk->readBlock(first_sect + (cluster-2) * sect_per_clus, sect_per_clus);
  }
}

FatDir* FatFilesystem::openrootdir() {
  return new FatDir(this, rootDirCluster);
}

FatDir* FatFilesystem::opendir(DirEntry *ent) {
  return new FatDir(this, ent->inodeId);
}

FatFile* FatFilesystem::open(DirEntry *ent) {
  return new FatFile(this, ent->inodeId, ent->size);
}

struct FatDir::fat_dirent {
  char name[11];
  uint8_t attributes;
  uint8_t reserved[8];
  uint16_t high_cluster;
  uint8_t reserved2[4];
  uint16_t low_cluster;
  uint32_t filesize;
};

struct fat_lfn_ent {
  char prefix;
  uint8_t utfdata1[10];
  uint8_t attributes;
  uint8_t enttype;
  uint8_t checksum;
  uint8_t utfdata2[12];
  char pad[2];
  uint8_t utfdata3[4];
};

future<FatDir::fat_dirent*> FatDir::get_next_entry(FatDir::fat_dirent*) {
  // TODO: add large FAT16 dir support back in
  if (!db || index == (fat->sect_per_clus * 512) / sizeof(fat_dirent)) {
    return fat->nextCluster(cluster).then([this](future<uint32_t> nc){
      uint32_t oldCluster = cluster;
      cluster = nc.get();
      index = 0;
      return fat->readCluster(oldCluster).then([this](future<DiskBlock*> fdb){
        db = fdb.get();
        return reinterpret_cast<fat_dirent*>(db->data + index++ * sizeof(fat_dirent));
      });
    });
  } else {
    return make_ready_future<fat_dirent*>(reinterpret_cast<fat_dirent*>(db->data + index++ * sizeof(fat_dirent)));
  }
}

FatDir::FatDir(FatFilesystem* fat, uint64_t cluster)
: fat(fat)
, cluster(cluster)
, index(0)
, db(NULL)
, dirent(NULL)
{}

future<DirEntry*> FatDir::readdir() {
  uint8_t *utf16buf = new uint8_t[520];
  uint8_t **ptr = new uint8_t*(utf16buf + 520);
  return async_for<fat_dirent*>(
    NULL,
    [](fat_dirent* arg){ return arg->name[0] == '\0' || arg->attributes != 0x0F; },
    [this, utf16buf, ptr](fat_dirent* arg){ 
      fat_lfn_ent* l = reinterpret_cast<fat_lfn_ent*>(arg);
      *ptr -= 4;
      memcpy(*ptr, l->utfdata3, 4);
      *ptr -= 12;
      memcpy(*ptr, l->utfdata2, 12);
      *ptr -= 10;
      memcpy(*ptr, l->utfdata1, 10);
      return this->get_next_entry(arg);
    }
  ).then([this, utf16buf, ptr](const future<fat_dirent*>& arg) -> DirEntry* {
    fat_dirent* ent = arg.get();
    if (ent->attributes == 0x0F) {
      return NULL;
    }
    DirEntry* dirent = new DirEntry();
    dirent->fs = fat;
    dirent->inodeId = (ent->high_cluster << 16) | (ent->low_cluster);
    dirent->flags = (ent->attributes & 0x10) != 0;
    dirent->size = ent->filesize;
    dirent->name = rodvin::string((uint16_t*)*ptr);
    delete [] utf16buf;
    delete ptr;
    return dirent;
  });
}

FatDir::~FatDir() {
  delete dirent;
}

FatFile::FatFile(FatFilesystem* fat, size_t cluster, size_t fileSize) 
: File(fileSize)
, fat(fat)
, firstCluster(cluster)
, currentCluster(cluster)
, bytesLeftInCurrentCluster(fat->sect_per_clus * 512)
{
}

size_t FatFile::filesize() {
  return size;
}

struct read_req {
  uint8_t *buffer;
  size_t amount;
  std::vector<future<void>> pendingReads;
  promise<void> readComplete;
};

future<size_t> FatFile::read(uint8_t *, size_t amount) {
  return make_ready_future<size_t>(amount);
/*
future<size_t> FatFile::read(uint8_t *buffer, size_t amount) {
  size_t rv = amount;
  while (amount) {
    size_t readFromThisCluster = std::min(amount, bytesLeftInCurrentCluster);
    req->pendingReads.push_back(fat->readCluster(currentCluster).then([buffer, bytesLeftInCurrentCluster = bytesLeftInCurrentCluster, readFromThisCluster](const future<DiskBlock*> &block) {
      DiskBlock* db = block.get();
      memcpy(buffer, db->data + db->size - bytesLeftInCurrentCluster, readFromThisCluster);
    }));
    buffer += readFromThisCluster;
    amount -= readFromThisCluster;
    bytesLeftInCurrentCluster -= readFromThisCluster;
    if (!bytesLeftInCurrentCluster) {
      currentCluster = fat->nextCluster(currentCluster);
      bytesLeftInCurrentCluster = fat->sect_per_clus * 512;
    }
    delete db;
  }
  return rv;
*/
}

future<size_t> FatFile::seek(int64_t deltaPosition, SeekPosition location) {
  int64_t newPosition;
  switch(location) {
    case FromStartOfFile:
    default:
      newPosition = deltaPosition;
      break;
    case FromCurrentPosition:
      newPosition = position + deltaPosition;
      break;
    case FromEndOfFile:
      newPosition = size - deltaPosition;
      break;
  }
  if (newPosition > (int64_t)size)
    newPosition = (int64_t)size;
  if (newPosition < 0)
    newPosition = 0;

  // TODO: for ExFAT, fix the static cast below.
  size_t sectorsToGo = static_cast<uint32_t>(newPosition) / (fat->sect_per_clus * 512);
  bytesLeftInCurrentCluster = newPosition - (fat->sect_per_clus * 512 * sectorsToGo);
  position = newPosition;
  currentCluster = firstCluster;
  return async_for(firstCluster, [mySectorsToGo = sectorsToGo, this](size_t currentCluster) mutable { this->currentCluster = currentCluster; return --mySectorsToGo > 0; }, [this](size_t currentCluster){
    return fat->nextCluster(currentCluster);
  }).then([this](const future<uint32_t>&){ return position; });
}

future<size_t> FatFile::write(const uint8_t *, size_t ) {
  printf("TODO: add fat file write\n");
  return async([]{ return static_cast<size_t>(0); });
}

FatFile::~FatFile() 
{
}



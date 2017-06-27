#ifndef FAT_H
#define FAT_H

#include "vfs.h"

struct FatFilesystem;

struct FatDir : public Dir {
  FatDir(FatFilesystem* fat, uint64_t cluster);
  ~FatDir() override;
  future<DirEntry *> readdir() override;
private:
  struct fat_dirent;
  future<fat_dirent*> get_next_entry(fat_dirent*);
  FatFilesystem *fat;
  uint64_t cluster;
  size_t index;
  DiskBlock* db;
  DirEntry *dirent;
};

struct FatFile : public File {
  FatFile(FatFilesystem* fat, size_t clusterId, size_t fileSize);
  ~FatFile();
  future<size_t> seek(int64_t position, SeekPosition location) override;
  size_t filesize() override;
  future<size_t> read(uint8_t *buffer, size_t amount) override;
  future<size_t> write(const uint8_t *buffer, size_t amount) override;
private:
  FatFilesystem *fat;
  uint32_t firstCluster;
  uint32_t currentCluster;
  uint32_t bytesLeftInCurrentCluster;
};

struct FatFilesystem : public Filesystem {
  FatFilesystem(Storage *disk, DiskBlock* firstSector);
  FatDir* openrootdir() override;
  FatDir* opendir(DirEntry* entry) override;
  FatFile* open(DirEntry* entry) override;
private:
  future<uint32_t> nextCluster(uint32_t cluster);
  future<DiskBlock*> readCluster(uint32_t cluster);
  Storage *disk;
  uint32_t sect_per_clus;
  uint32_t reserved_sect;
  uint32_t first_sect;
  uint32_t root_entry_count;
  uint32_t root_directory;
  uint32_t rootDirCluster;
  enum {
    Fat16,
    Fat32,
    ExFat,
  } fat_version;
  friend class FatDir;
  friend class FatFile;
};


#endif



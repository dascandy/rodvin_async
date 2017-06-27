#include "vfs.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <path>

future<DirEntry*> Vfs::lookup(const rodvin::path &name) {
  DirEntry *ient = new DirEntry;
  ient->fs = fs;
  ient->inodeId = 0xFFFFFFFF;
  future<DirEntry*> ent = make_ready_future(ient);

  for (const rodvin::string &p : name.string().split('/')) {
    ent = ent.then([p](const future<DirEntry*>& f) -> future<DirEntry*>{
      DirEntry* ent = f.get();
      if (!ent) return make_ready_future<DirEntry*>(nullptr);
      Dir* d = ent->opendir();
      delete ent;
      return d->readdir().then([p,d](const future<DirEntry*> &f) {
        future<DirEntry*> newent = async_for(f.get(), [p,d](DirEntry* e) { 
          bool lastEntry = !e || e->name == p;
          if (!lastEntry) delete e;
          return lastEntry;
        }, [d](DirEntry*){
          return d->readdir();
        });
        delete d;
        return newent;
      });
    });
  }
  return ent;
}

void Vfs::Register(Filesystem* fs) {
  printf("root vfs set\n");
  Vfs::fs = fs;
}

Dir* DirEntry::opendir() {
  if (inodeId == 0xFFFFFFFF) {
    return fs->openrootdir();
  }
  return fs->opendir(this);
}

File* DirEntry::open() {
  return fs->open(this);
}

Filesystem* Vfs::fs;



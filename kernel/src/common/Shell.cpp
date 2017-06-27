#include "Shell.h"
#include "version.h"
#include "Keyboard.h"
#include "device.h"
#include "vfs.h"
#include "interrupts.h"
#include "future.h"

future<Shell*> Shell::Create()
{
  return Console::Create(1280, 720).then([](future<Console*> f){
    return new Shell(f.get());
  });
}

Shell::Shell(Console* con)
: con(con)
, currentPath("/")
{
  functions["echo"] = &Shell::executeEcho;
  functions["ls"] = &Shell::executeLs;
  functions["cd"] = &Shell::executeCd;
  functions["cat"] = &Shell::executeCat;

  con->printtext(getVersion());
  con->printtext("\nu@localhost:");
  con->printtext(currentPath.string());
  con->printtext("$ ");
  async([this]{
    this->run();
  });
}

void Shell::run() {
  getchar().then([this](const future<uint32_t>&f){ this->handleChar(f.get()).then([this](future<void> f){ f.get(); this->run(); }); });
}

future<void> Shell::handleChar(uint32_t c)
{
  switch(c) {
  case '\n':
    con->printtext("\n");
    return execute().then([this](future<void> f){
      f.get();
      con->printtext("u@localhost:");
      con->printtext(currentPath.string());
      con->printtext("$ ");
      currentCmd.clear();
    });
    break;
  case '\b':
    {
      currentCmd.pop_back();
      uint32_t t[2] = { c, 0 };
      con->printtext(rodvin::string(t));
    }
    break;
  default:
    {
      currentCmd.push_back(c);
      uint32_t t[2] = { c, 0 };
      con->printtext(rodvin::string(t));
    }
    break;
  }
  return make_ready_void_future();
}

future<void> Shell::execute() {
  std::vector<rodvin::string> args = currentCmd.split();
  if (args.empty()) return make_ready_void_future();
  auto it = functions.find(args[0]);
  if (it != functions.end()) {
    return (this->*(it->second))(args);
  } else {
    con->printtext("Unknown command ");
    con->printtext(args[0]);
    con->printtext("\n");
    return make_ready_void_future();
  }
}

future<void> Shell::executeEcho(const std::vector<rodvin::string>& args) {
  auto it = args.begin();
  ++it;
  for (; it != args.end(); ++it) {
    con->printtext(*it);
    con->printtext(" ");
  }
  con->printtext("\n");
  return make_ready_void_future();
}

future<void> Shell::executeLs(const std::vector<rodvin::string>& args) {
  rodvin::path p = currentPath;
  if (args.size() > 1) {
    if (*args[1].begin() == '/') {
      p = rodvin::path(args[1]).canonical();
    } else {
      p = (currentPath / args[1]).canonical();
    }
  }
  return Vfs::lookup(p).then([this,p](future<DirEntry*> fent) {
    DirEntry* ent = fent.get();
    if (!ent) {
      con->printtext("Path not found: ");
      con->printtext(p.string());
      con->printtext("\n");
      return make_ready_void_future();
    }
    Dir* d = ent->opendir();
    delete ent;
    return async_for<DirEntry*>(NULL, [](DirEntry*e){ return e != NULL; }, [d,this](DirEntry* e) {
      con->printtext(e->name);
      con->printtext("\n");
      delete e;
      return d->readdir();
    }).then([d](future<DirEntry*> f){ f.get(); delete d; });
  });
}

future<void> Shell::executeCd(const std::vector<rodvin::string>& args) {
  if (args.size() == 1) {
    con->printtext("No home dir yet\n");
    return make_ready_void_future();
  }
  rodvin::path old = currentPath, newPath;
  if (*args[1].begin() == '/') {
    newPath = rodvin::path(args[1]).canonical();
  } else {
    newPath = (currentPath / args[1]).canonical();
  }

  return Vfs::lookup(currentPath).then([newPath, old, this](future<DirEntry*> fent) {
    DirEntry* ent = fent.get();
    if (ent) {
      currentPath = newPath;
      delete ent;
    } else {
      con->printtext("Path not found: ");
      con->printtext(currentPath.string());
      con->printtext("\n");
      currentPath = old;
    }
  });
}

future<void> Shell::executeCat(const std::vector<rodvin::string>& args) {
  rodvin::path p;
  if (*args[1].begin() == '/') {
    p = rodvin::path(args[1]).canonical();
  } else {
    p = (currentPath / args[1]).canonical();
  }
  return Vfs::lookup(p).then([this, p](future<DirEntry*> fent){
    DirEntry* ent = fent.get();
    if (!ent) {
      con->printtext("Path not found: ");
      con->printtext(p.string());
      con->printtext("\n");
      return;
    }
    
    File* f = ent->open();
    char *buf = new char[f->filesize()+1];
    f->read((uint8_t*)buf, f->filesize());
    buf[f->filesize()] = 0;
    con->printtext(buf);
    delete [] buf;
    delete f;
    con->printtext("\n");
  });
}



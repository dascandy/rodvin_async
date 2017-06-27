#ifndef SHELL_H
#define SHELL_H

#include "Console.h"
#include <path>
#include "future.h"

class Dir;

class Shell {
public:
  static future<Shell*> Create();
  Shell(Console* con);
  void run();
  future<void> handleChar(uint32_t f);
  future<void> execute();
  future<void> executeEcho(const std::vector<rodvin::string>& args);
  future<void> executeLs(const std::vector<rodvin::string>& args);
  future<void> executeCd(const std::vector<rodvin::string>& args);
  future<void> executeCat(const std::vector<rodvin::string>& args);

  Console *con;
  rodvin::path currentPath;
  rodvin::string currentCmd;
  vectormap<rodvin::string, future<void> (Shell::*)(const std::vector<rodvin::string>&)> functions;
  vectormap<rodvin::string, rodvin::string> env;
};

#endif



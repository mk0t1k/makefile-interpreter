#include "makefile.h"

int main(int argc, char* argv[])
{
  std::string filename = std::string(argv[1]);
  MakeFile make(filename);
  make.Execute();

  return 0;
}
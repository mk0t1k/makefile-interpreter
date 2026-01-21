#include "makefile.h"
#include "cli.h"
#include "argparser/argparser.h"

#include <iostream>

int main(int argc, char* argv[])
{

  CliOptions options;
  options.makefile_name = GetMakefileName();
  nargparse::ArgumentParser parser = CreateMakeParser(options);
  
  if (!parser.Parse(argc, const_cast<const char**>(argv)))
  {
    std::cerr << "[make]: Can't parse your cli command. Stop building.\n";
    return 1;
  }
  
  CollectCliTargets(parser, options);
  
  if (options.makefile_name.empty()) 
  {
    std::cerr << "[make]: No targets makefile found. Stop building.\n";
    return 1;
  }
  
  MakeFile make(options.makefile_name, options.targets);
  make.Execute(MakeOptions{options.dry_run, options.silent, options.keep_going});

  return 0;
}
#include "makefile.h"
#include "cli.h"
#include "argparser/argparser.h"

#include <iostream>
#include <filesystem>

int main(int argc, char* argv[])
{
  namespace fs = std::filesystem;

  CliOptions options;
  nargparse::ArgumentParser parser = CreateMakeParser(options);
  
  if (!parser.Parse(argc, const_cast<const char**>(argv)))
  {
    std::cerr << "[make]: Can't parse your cli command. Stop building.\n";
    return 1;
  }
  
  CollectCliTargets(parser, options);
  
  
  if (!options.directory.empty())
  {
    if (!fs::exists(options.directory) || !fs::is_directory(options.directory))
    {
      std::cerr << "[make]: Cannot change to directory '" << options.directory << "': No such file or directory\n";
      return 1;
    }
    fs::current_path(options.directory);
  }
  
  if (options.makefile_name.empty())
  {
    options.makefile_name = GetMakefileName();
  }
  
  if (options.makefile_name.empty()) 
  {
    std::cerr << "[make]: No targets makefile found. Stop building.\n";
    return 1;
  }
  
  try
  {
    MakeFile make(options.makefile_name, options.targets);
    make.Execute(MakeOptions{options.dry_run, options.silent, options.keep_going});
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
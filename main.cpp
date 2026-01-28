#include "makefile.h"
#include "cli.h"
#include "argparser/argparser.h"
#include "logger.h"

#include <iostream>
#include <filesystem>

int main(int argc, char* argv[])
{
  namespace fs = std::filesystem;

  CliOptions options;
  nargparse::ArgumentParser parser = CreateMakeParser(options);
  
  if (!parser.Parse(argc, const_cast<const char**>(argv)))
  {
    loging::LogError("Can't parse your cli command. Stop building.");
    return 1;
  }
  
  CollectCliTargets(parser, options);
  
  
  if (!options.directory.empty())
  {
    if (!fs::exists(options.directory) || !fs::is_directory(options.directory))
    {
      loging::LogError("Cannot change to directory '" + options.directory + "': No such file or directory");
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
    loging::LogError("No targets makefile found. Stop building.");
    return 1;
  }
  
  bool need_rebuild = false;
  try
  {
    MakeFile make(options.makefile_name, options.targets);
    need_rebuild = make.Execute(MakeOptions{
      options.dry_run,
      options.silent,
      options.keep_going,
      options.ignore_errors,
      options.always_make,
      options.question
    });

    if (options.question)
      return need_rebuild ? 1 : 0;
    
    return 0;
  }
  catch (const std::exception& e)
  {
    loging::LogError(std::string(e.what()));
    return 1;
  }
}
#include "cli.h"

#include <filesystem>

const std::vector<std::string> standard_names = {"GNUmakefile", "makefile", "Makefile"};

std::string GetMakefileName()
{
  namespace fs = std::filesystem;
  
  for (const auto& name : standard_names)
    if (fs::exists(name) && fs::is_regular_file(name))
      return name;
  
  return "";
}

nargparse::ArgumentParser CreateMakeParser(CliOptions& options)
{
  using namespace nargparse;
  ArgumentParser parser("make", kMaxArgLen);

  parser.AddArgument<std::string>("-f", "--file", &options.makefile_name, "Path to Makefile", 
                                 kNargsOptional, nullptr, "Incorrect filename");

  parser.AddPositional<std::string>("target", "Target names (optional)", kNargsZeroOrMore);

  parser.AddFlag("-n", "--dry-run", &options.dry_run, "Don't actually run any recipe; just print them.");
  parser.AddFlag("-s", "--silent", &options.silent, "Don't echo recipes.");
  parser.AddFlag("-k", "--keep-going", &options.keep_going, "Keep going when some targets can't be made.");

  return parser;
}

void CollectCliTargets(nargparse::ArgumentParser& parser, CliOptions& options)
{
  using namespace nargparse;
  int count = parser.GetRepeatedCount("target");
  options.targets.clear();
  
  for (int i = 0; i < count; i++) 
  {
    std::string target;
    if (parser.GetRepeated<std::string>("target", i, &target))
      options.targets.push_back(target);
  }
}
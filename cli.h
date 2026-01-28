#pragma once

#include <cstddef>
#include <vector>
#include <string>

#include "argparser/argparser.h"

static constexpr std::size_t kMaxArgLen = 512;

struct CliOptions 
{
  std::string makefile_name;
  std::string directory;

  std::vector<std::string> targets;

  bool dry_run = false;
  bool keep_going = false;
  bool silent = false;
  bool always_make = false;
  bool ignore_errors = false;
};

nargparse::ArgumentParser CreateMakeParser(CliOptions& options);
std::string GetMakefileName();
void CollectCliTargets(nargparse::ArgumentParser& parser, CliOptions& options);
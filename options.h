#pragma once

#include <string>
#include <unordered_map>

struct MakeOptions 
{
  bool dry_run = false;
  bool silent = false;
  bool keep_going = false;
  bool ignore_errors = false;
  bool always_make = false;
  bool question_only = false;
  std::unordered_map<std::string, std::string> vars;
};


#pragma once

struct MakeOptions 
{
  bool dry_run = false;
  bool silent = false;
  bool keep_going = false;
  bool ignore_errors = false;
  bool always_make = false;
  bool question_only = false;
};


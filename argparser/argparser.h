#pragma once
#include <cstddef>
#include <vector>
#include <string>

#include "argument.h"

namespace nargparse 
{

class ArgumentParser 
{
  std::string program_name;
  std::string version;
  size_t max_arg_len;
    
  int arg_count;
  int positional_count;
  bool help_enabled;
  bool version_enabled;
  bool show_positionals_in_help = true;
  std::string custom_usage;
  
  std::vector<ArgumentBase*> arguments_;
  std::vector<ArgumentBase*> positional_arguments_;

  ArgumentBase* FindArgumentByOption(const char* token);
  int HandleEqualsSyntax(const char* token, int i);
  int ProcessFlagOption(ArgumentBase* arg);
  int ProcessSingleValueOption(ArgumentBase* arg, const char* argv[], int argc, int i);
  int ProcessMultipleValuesOption(ArgumentBase* arg, const char* argv[], int argc, int i);
  int ProcessValueOption(ArgumentBase* arg, const char* argv[], int argc, int i);
  int ProcessOption(const char* argv[], int argc, int i);
  bool ProcessPositional(std::size_t& current_positional_index, const char* value);
  bool ValidateRequiredArguments();

public:
  ArgumentParser(const std::string& program_name, const std::string& version, size_t max_arg_len);
  ~ArgumentParser();

  void SetShowPositionalsInHelp(bool value) { show_positionals_in_help = value; }
  void SetUsage(const std::string& usage) { custom_usage = usage; }

  template<typename T>
  void AddArgument(Argument<T>* arg);

  template<typename T>
  void AddArgument(const std::string& short_name, const std::string& long_name,
                   T* target, const std::string& help, 
                   NargsType nargs = kNargsOptional,
                   bool (*validator)(const T&) = nullptr,
                   const std::string& validation_error = "");

  // overloading for positional arg
  template<typename T>
  void AddPositional(const std::string& name, const std::string& help,
                   NargsType nargs = kNargsOptional,
                   bool (*validator)(const T&) = nullptr,
                   const std::string& validation_error = "");
  
  void AddFlag(const std::string& short_name, const std::string& long_name,
               bool* target, const std::string& help, bool default_value = false);

  bool Parse(int argc, const char* argv[]);

  void AddHelp();
  void PrintHelp();
  bool ProcessHelp(int argc, const char* argv[]);

  void AddVersion();
  void PrintVersion();
  bool ProcessVersion(int argc, const char* argv[]);

  int GetRepeatedCount(const std::string& name);
  
  template<typename T>
  bool GetRepeated(const std::string& name, int index, T* value);
};

template<typename T>
void ArgumentParser::AddArgument(Argument<T>* arg)
{
  if (!arg) return;

  arguments_.push_back(arg);

  if (arg->is_positional_)
  {
    positional_arguments_.push_back(arg);
    positional_count++;
  }
  else
  {
    arg_count++;
  }
}

template<typename T>
void ArgumentParser::AddArgument(const std::string& short_name, const std::string& long_name,
                                 T* target, const std::string& help, NargsType nargs,
                                 bool (*validator)(const T&),
                                 const std::string& validation_error)
{
  auto* arg = new Argument<T>();

  if (!short_name.empty()) arg->short_name_ = short_name;
  if (!long_name.empty()) arg->long_name_ = long_name;
  if (!help.empty()) arg->name_ = help;
  else if (!arg->long_name_.empty()) arg->name_ = arg->long_name_;
  else if (!arg->short_name_.empty() && arg->short_name_.size() > 1) arg->name_ = arg->short_name_.substr(1);
  else arg->name_ = "arg";

  arg->SetTarget(target);
  arg->SetValidator(validator);
  arg->nargs_ = nargs;
  arg->is_flag_ = false;
  arg->is_positional_ = (arg->short_name_.empty() && arg->long_name_.empty());
  if (!validation_error.empty()) arg->validation_error = validation_error;

  AddArgument(arg);
}

template<typename T>
void ArgumentParser::AddPositional(const std::string& name, const std::string& help,
                                   NargsType nargs,
                                   bool (*validator)(const T&),
                                   const std::string& validation_error)
{
  auto* arg = new Argument<T>();
  arg->short_name_.clear();
  arg->long_name_.clear();
  arg->name_ = name;
  arg->SetTarget(nullptr);
  arg->SetValidator(validator);
  arg->nargs_ = nargs;
  arg->is_flag_ = false;
  arg->is_positional_ = true;
  if (!validation_error.empty())
    arg->validation_error = validation_error;

  AddArgument(arg);
}

template<typename T>
bool ArgumentParser::GetRepeated(const std::string& name, int index, T* value)
{
  if (!value) return false;

  ArgumentBase* base = nullptr;
  for (ArgumentBase* arg : arguments_)
  {
    if (arg && arg->name_ == name)
    {
      base = arg;
      break;
    }
  }

  if (!base) return false;

  auto* typed = dynamic_cast<Argument<T>*>(base);
  if (!typed) return false;

  const auto& values = typed->GetValues();
  if (index < 0 || static_cast<std::size_t>(index) >= values.size())
    return false;

  *value = values[static_cast<std::size_t>(index)];
  return true;
}

} // namespace nargparse

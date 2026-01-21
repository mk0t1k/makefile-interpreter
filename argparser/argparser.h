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
  size_t max_arg_len;
    
  int arg_count;
  int positional_count;
  bool help_enabled;
  
  std::vector<ArgumentBase*> arguments_;
  std::vector<ArgumentBase*> positional_arguments_;

  // Вспомогательные методы для парсинга
  ArgumentBase* FindArgumentByOption(const char* token);
  bool ProcessHelp(int argc, const char* argv[]);
  int HandleEqualsSyntax(const char* token, int i);
  int ProcessFlagOption(ArgumentBase* arg);
  int ProcessSingleValueOption(ArgumentBase* arg, const char* argv[], int argc, int i);
  int ProcessMultipleValuesOption(ArgumentBase* arg, const char* argv[], int argc, int i);
  int ProcessValueOption(ArgumentBase* arg, const char* argv[], int argc, int i);
  int ProcessOption(const char* argv[], int argc, int i);
  bool ProcessPositional(std::size_t& current_positional_index, const char* value);
  bool ValidateRequiredArguments();

public:
  ArgumentParser(std::string program_name, size_t max_arg_len);
  ~ArgumentParser();
  
  template<typename T>
  void AddArgument(Argument<T>* arg)
  {
    if (!arg) return;
  
    arguments_.push_back(arg);
  
    if (arg->is_positional_) 
    {
      positional_arguments_.push_back(arg);
      positional_count++;
    } 
    else
      arg_count++;
  }
  
  template<typename T>
  void AddArgument(std::string short_name, std::string long_name,
                   T* target, std::string help, 
                   NargsType nargs = kNargsOptional,
                   bool (*validator)(const T&) = nullptr,
                   std::string validation_error = "")
  {                                 
    auto* arg = new Argument<T>();
  
    if (!short_name.empty()) arg->short_name_ = short_name;
    if (!long_name.empty()) arg->long_name_ = long_name;
    if (!help.empty()) arg->name_ = help;
    else if (!long_name.empty()) arg->name_ = long_name;
    else if (!short_name.empty() && short_name.size() > 1) arg->name_ = short_name.substr(1);
    else arg->name_ = "arg";
  
    arg->SetTarget(target);
    arg->SetValidator(validator);
    arg->nargs_ = nargs;
    arg->is_flag_ = false;
    arg->is_positional_ = (short_name.empty() && long_name.empty());
    if (!validation_error.empty()) arg->validation_error = validation_error;
  
    AddArgument(arg);
  }

  // overloading for positional arg
  template<typename T>
  void AddPositional(std::string name, std::string help,
                   NargsType nargs = kNargsOptional,
                   bool (*validator)(const T&) = nullptr,
                   std::string validation_error = "")
  {
    auto* arg = new Argument<T>();
    arg->short_name_ = "";
    arg->long_name_ = "";
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
  
  void AddFlag(std::string short_name, std::string long_name, bool* target, std::string help,
               bool default_value = false);
  bool Parse(int argc, const char* argv[]);
  void AddHelp();
  void PrintHelp();
  int GetRepeatedCount(std::string name);
  
  template<typename T>
  bool GetRepeated(std::string name, int index, T* value)
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
};

} // namespace nargparse

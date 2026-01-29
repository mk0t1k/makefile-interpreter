#include "argparser.h"

#include <cstring>
#include <iostream>

namespace nargparse {

ArgumentParser::ArgumentParser(const std::string& program_name, const std::string& version, size_t max_arg_len)
  : program_name(program_name)
  , version(version)
  , max_arg_len(max_arg_len)
  , arg_count(0)
  , positional_count(0)
  , help_enabled(false)
{}

ArgumentParser::~ArgumentParser()
{
  for (ArgumentBase* arg : arguments_)
    delete arg;
}

void ArgumentParser::AddFlag(const std::string& short_name, const std::string& long_name, bool* target, 
                            const std::string& help, bool default_value)
{
  auto* arg = new Argument<bool>();
  
  arg->short_name_ = short_name;
  arg->long_name_ = long_name;
  
  if (!help.empty())
    arg->name_ = help;
  else if (!arg->long_name_.empty())
    arg->name_ = arg->long_name_;
  else if (!arg->short_name_.empty() && arg->short_name_.size() > 1)
    arg->name_ = arg->short_name_.substr(1);
  else
    arg->name_ = "flag";
  
  arg->SetTarget(target);
  arg->nargs_ = kNargsOptional;
  arg->is_flag_ = true;
  arg->is_positional_ = false;
  
  if (target)
    *target = default_value;
  
  AddArgument(arg);
}

void ArgumentParser::AddHelp()
{
  help_enabled = true;
  static bool help_flag = false;
  AddFlag("-h", "--help", &help_flag, "Show help", false);
}

void ArgumentParser::PrintHelp()
{
  if (!custom_usage.empty())
  {
    std::cout << "Usage: " << custom_usage << "\n\nOptions:\n";
  }
  else
  {
    std::cout << "Usage: "
              << (program_name.empty() ? "program" : program_name)
              << " [options]\n\nOptions:\n";
  }
  
  for (ArgumentBase* arg : arguments_)
  {
    if (!arg) continue;

    if (!show_positionals_in_help && arg->is_positional_)
      continue;

    std::cout << "  ";
    bool has_option = false;

    if (!arg->short_name_.empty() || !arg->long_name_.empty())
    {
      if (!arg->short_name_.empty())
      {
        std::cout << arg->short_name_;
        has_option = true;
      }
      if (!arg->long_name_.empty())
      {
        if (has_option) std::cout << ", ";
        std::cout << arg->long_name_;
        has_option = true;
      }
    }
    else if (arg->is_positional_ && !arg->name_.empty())
    {
      std::cout << arg->name_;
      has_option = true;
    }
    
    if (!arg->name_.empty())
    {
      if (has_option)
        std::cout << "  ";
      std::cout << arg->name_;
    }

    if (!arg->validation_error.empty())
    {
      if (!arg->name_.empty())
        std::cout << " ";
      std::cout << "(" << arg->validation_error << ")";
    }

    if (has_option || !arg->name_.empty() || !arg->validation_error.empty())
      std::cout << "\n";
  }
  
  std::cout << "\n";
}

void ArgumentParser::AddVersion()
{
  version_enabled = true;
  static bool version_flag = false;
  AddFlag("-v", "--version", &version_flag, "Show version", false);
}

void ArgumentParser::PrintVersion()
{
  std::cout << program_name << " version: " << version << '\n';
}

ArgumentBase* ArgumentParser::FindArgumentByOption(const char* token)
{
  if (!token) return nullptr;
  
  for (ArgumentBase* arg : arguments_)
  {
    if (!arg) continue;
    if (!arg->short_name_.empty() && arg->short_name_ == token)
      return arg;
    if (!arg->long_name_.empty() && arg->long_name_ == token)
      return arg;
  }
  return nullptr;
}

bool ArgumentParser::ProcessHelp(int argc, const char* argv[])
{
  if (!help_enabled)
    return false;
  
  for (int i = 1; i < argc; ++i)
  {
    if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0)
    {
      PrintHelp();
      return true;
    }
  }
  return false;
}

bool ArgumentParser::ProcessVersion(int argc, const char* argv[])
{
  if (!version_enabled) return false;

  for (int i = 1; i < argc; ++i)
  {
    if (std::strcmp(argv[i], "-v") == 0 || std::strcmp(argv[i], "--version") == 0)
    {
      PrintVersion();
      return true;
    }
  }
  return false;
}

int ArgumentParser::HandleEqualsSyntax(const char* token, int /*i*/)
{
  const char* equals_ptr = std::strchr(token, '=');
  if (!equals_ptr)
    return 0;
  
  std::string option(token, equals_ptr - token);
  ArgumentBase* arg = FindArgumentByOption(option.c_str());
  
  if (!arg || arg->is_flag_)
    return -1;
  
  if (arg->nargs_ == kNargsOptional && arg->values_count_ > 0)
    return -1;
  
  const char* value = equals_ptr + 1;
  if (!arg->ParseValue(value, max_arg_len))
    return -1;
  
  return 1;
}

int ArgumentParser::ProcessFlagOption(ArgumentBase* arg)
{
  arg->ParseValue("", 0);
  return 1;
}

int ArgumentParser::ProcessSingleValueOption(ArgumentBase* arg,
                                               const char* argv[], int argc, int i)
{
  if (i + 1 >= argc)
  {
    return (arg->nargs_ == kNargsRequired) ? -1 : 1;
  }
  
  if (!arg->ParseValue(argv[i + 1], max_arg_len))
    return -1;
  
  return 2;
}

int ArgumentParser::ProcessMultipleValuesOption(ArgumentBase* arg,
                                                 const char* argv[], int argc, int i)
{
  int values_taken = 0;
  int current_i = i;
  
  while (current_i + 1 < argc)
  {
    const char* next_token = argv[current_i + 1];
    
    if (next_token[0] == '-' && std::strlen(next_token) > 1)
      break;
    
    if (!arg->ParseValue(next_token, max_arg_len))
      return -1;
    
    values_taken++;
    current_i++;
  }
  
  if (arg->nargs_ == kNargsOneOrMore && values_taken == 0)
    return -1;
  
  return 1 + values_taken;
}

int ArgumentParser::ProcessValueOption(ArgumentBase* arg,
                                       const char* argv[], int argc, int i)
{
  switch (arg->nargs_)
  {
    case kNargsRequired:
    case kNargsOptional:
      return ProcessSingleValueOption(arg, argv, argc, i);
    case kNargsZeroOrMore:
    case kNargsOneOrMore:
      return ProcessMultipleValuesOption(arg, argv, argc, i);
    default:
      return -1;
  }
}

int ArgumentParser::ProcessOption(const char* argv[], int argc, int i)
{
  const char* token = argv[i];
  
  int equals_result = HandleEqualsSyntax(token, i);
  if (equals_result != 0)
    return equals_result;
  
  ArgumentBase* arg = FindArgumentByOption(token);
  if (!arg)
    return -1;
  
  if (!arg->is_flag_ && arg->nargs_ == kNargsOptional && arg->values_count_ > 0)
    return -1;
  
  if (arg->is_flag_)
    return ProcessFlagOption(arg);
  else
    return ProcessValueOption(arg, argv, argc, i);
}

bool ArgumentParser::ProcessPositional(std::size_t& current_positional_index,
                                       const char* value)
{
  if (current_positional_index >= positional_arguments_.size())
    return false;
  
  ArgumentBase* current = positional_arguments_[current_positional_index];
  if (!current)
    return false;
  
  if (!current->ParseValue(value, max_arg_len))
    return false;
  
  if (current->nargs_ == kNargsRequired || current->nargs_ == kNargsOptional)
  {
    current_positional_index++;
  }
  
  return true;
}

bool ArgumentParser::ValidateRequiredArguments()
{
  for (ArgumentBase* arg : arguments_)
  {
    if (!arg || arg->is_flag_) continue;
    
    if ((arg->nargs_ == kNargsRequired || arg->nargs_ == kNargsOneOrMore) &&
        arg->values_count_ == 0)
      return false;
  }
  
  for (ArgumentBase* arg : positional_arguments_)
  {
    if (!arg) continue;
    
    if ((arg->nargs_ == kNargsRequired || arg->nargs_ == kNargsOneOrMore) &&
        arg->values_count_ == 0)
      return false;
  }
  
  return true;
}

bool ArgumentParser::Parse(int argc, const char* argv[])
{
  if (ProcessHelp(argc, argv))
    return true;

  if (ProcessVersion(argc, argv))
    return true;
  
  std::size_t current_positional_index = 0;
  
  for (int i = 1; i < argc; )
  {
    if (argv[i][0] == '-' && std::strlen(argv[i]) > 1)
    {
      int processed = ProcessOption(argv, argc, i);
      if (processed < 0)
        return false;
      i += processed;
    }
    else
    {
      if (current_positional_index >= positional_arguments_.size())
      {
        return false;
      }
      
      if (!ProcessPositional(current_positional_index, argv[i]))
        return false;
      ++i;
    }
  }
  
  return ValidateRequiredArguments();
}

int ArgumentParser::GetRepeatedCount(const std::string& name)
{
  for (ArgumentBase* arg : arguments_)
  {
    if (arg && arg->name_ == name)
      return arg->values_count_;
  }
  return 0;
}


}  // namespace nargparse
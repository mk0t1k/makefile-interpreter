#include <cstdlib>
#include <set>
#include <unordered_set>

#include "rule.h"
#include "options.h"
#include "logger.h"

namespace
{
  static const std::unordered_set<std::string> kSpecialVars = {"@F", "@D", "<F", "<D", "^F", "^D"};

  bool FindVariable(const std::string& str, size_t pos,
    size_t* dollar, std::string* var_name, size_t* end)
  {
    while (pos < str.size())
    {
      size_t dollar_pos = str.find('$', pos);
      if (dollar_pos == std::string::npos || dollar_pos + 1 >= str.size())
        return false;

      size_t var_start = 0;
      size_t var_end = 0;

      if (str[dollar_pos + 1] == '(')
      {
        var_start = dollar_pos + 2;
        var_end = str.find(')', var_start);
      }
      else if (str[dollar_pos + 1] == '{')
      {
        var_start = dollar_pos + 2;
        var_end = str.find('}', var_start);
      }
      else
      {
        pos = dollar_pos + 1;
        continue;
      }

      if (var_end == std::string::npos)
      {
        pos = dollar_pos + 1;
        continue;
      }

      *var_name = str.substr(var_start, var_end - var_start);
      if (kSpecialVars.count(*var_name))
      {
        pos = dollar_pos + 1;
        continue;
      }

      *dollar = dollar_pos;
      *end = var_end + 1;
      return true;
    }
    return false;
  }

  std::string ExpandVariables(std::string str,
    const std::unordered_map<std::string, std::string>& vars,
    std::set<std::string>* in_progress = nullptr)
  {
    std::set<std::string> local;
    std::set<std::string>* ip = in_progress ? in_progress : &local;

    size_t pos = 0;
    size_t dollar, end_pos;
    std::string var_name;

    while (FindVariable(str, pos, &dollar, &var_name, &end_pos))
    {
      std::string replacement;
      if (ip->count(var_name))
        replacement = "";
      else
      {
        ip->insert(var_name);
        auto it = vars.find(var_name);
        if (it != vars.end())
          replacement = ExpandVariables(it->second, vars, ip);
        else
          replacement = "";
        ip->erase(var_name);
      }

      str.replace(dollar, end_pos - dollar, replacement);
      pos = dollar + replacement.size();
    }
    return str;
  }
}

bool Rule::IsNeedRebuild(const MakeOptions& options) const
{
	if (options.always_make) return true;

	if (!fs::exists(target_)) return true;

	if (is_phony_) return true;

	auto target_time = fs::last_write_time(target_);

	for (const fs::path& dependence : dependencies_)
		if (!fs::exists(dependence) || target_time < fs::last_write_time(dependence))
			return true;
	return false;
}

bool Rule::Run(const MakeOptions& options)
{
	for (const std::string& com : commands_)
	{
		std::string command = PrepareCommand(com, options);
		if (!options.silent || options.dry_run)
			loging::LogInfo(command);
		
		if (options.dry_run)
			continue;
		
		int status = system(command.c_str());
		
		if (status != 0)
		{
			std::string error_msg = "Command failed: " + command;
			if (options.ignore_errors)
			{
				loging::LogError(error_msg + " (ignored)");
				continue;
			}
			throw loging::MakeException(error_msg);
		}
	}
	return true;
}

std::string Rule::PrepareCommand(std::string command, const MakeOptions& options)
{
  command = ExpandVariables(command, options.vars);

  std::string target_str = target_.string();
  std::string stem_str = stem_;
    
  std::string first_dep = dependencies_.empty() ? "" : dependencies_[0].string();
  std::string first_dep_filename = dependencies_.empty() ? "" : dependencies_[0].filename().string();
  std::string first_dep_dir = dependencies_.empty() ? "" : dependencies_[0].parent_path().string();
    
  std::string deps;  // $+
  for (const auto& dep : dependencies_)
    deps += (deps.empty() ? "" : " ") + dep.string();
    
  std::string unique_deps;  // $^
  for (const auto& dep : std::set(dependencies_.begin(), dependencies_.end()))
    unique_deps += (unique_deps.empty() ? "" : " ") + dep.string();
    
  std::string new_deps;  // $?
  if (!fs::exists(target_)) 
	{
  	for (const fs::path& dep : dependencies_)
      new_deps += (new_deps.empty() ? "" : " ") + dep.string();
  } 
	else 
	{
    auto target_time = fs::last_write_time(target_);
      for (const fs::path& dep : dependencies_)
        if (fs::exists(dep) && target_time < fs::last_write_time(dep))
          new_deps += (new_deps.empty() ? "" : " ") + dep.string();
  }
    
  std::string deps_filenames, deps_dirs;
  for (const auto& dep : dependencies_) 
	{
    if (!deps_filenames.empty()) deps_filenames += " ";
    	deps_filenames += dep.filename().string();
    if (!deps_dirs.empty()) deps_dirs += " ";
      deps_dirs += dep.parent_path().string();
  }
    
  std::string target_filename = target_.filename().string();
  std::string target_dir = target_.parent_path().string();
    
  std::vector<std::pair<std::string_view, std::string>> replacements = {
        {"$(@F)", target_filename},
        {"$(@D)", target_dir},
        {"$(<F)", first_dep_filename},
        {"$(<D)", first_dep_dir},
        {"$(^F)", deps_filenames},
        {"$(^D)", deps_dirs},
        {"$@", target_str},
        {"$<", first_dep},
        {"$+", deps},
        {"$^", unique_deps},
        {"$?", new_deps},
        {"$*", stem_str},
    };
    
  for (const auto& [pattern, replacement] : replacements) 
	{
  	size_t pos = 0;
    while ((pos = command.find(pattern, pos)) != std::string::npos) 
		{
      command.replace(pos, pattern.length(), replacement);
      pos += replacement.length();
    }
  }
  return command;
}

Rule::Rule(fs::path target,
       std::vector<fs::path> dependencies, 
       std::vector<fs::path> prereqs, 
       std::vector<std::string> commands,
			 std::string stem)
			 : target_(target)
			 , dependencies_(dependencies)
       , order_only_prerequisites_(prereqs)
			 , commands_(commands)
			 , stem_(stem)
{}
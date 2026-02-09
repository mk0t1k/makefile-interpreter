#include <cstdlib>
#include <set>

#include "rule.h"
#include "options.h"
#include "logger.h"

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
		std::string command = PrepareCommand(com);
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

std::string Rule::PrepareCommand(std::string command)
{
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
       std::vector<std::string> commands,
			 std::string stem)
			 : target_(target)
			 , dependencies_(dependencies)
			 , commands_(commands)
			 , stem_(stem)
{}
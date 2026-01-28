#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "rule.h"
#include "options.h"

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
	for (const std::string& command : commands_)
	{
		if (!options.silent || options.dry_run)
			std::cout << command << std::endl;
		
		if (options.dry_run)
			continue;
		
		int status = system(command.c_str());
		
		if (status != 0)
		{
			std::string error_msg = "Command failed: " + command;
			if (options.ignore_errors)
			{
				std::cerr << "[make]: " << error_msg << " (ignored)" << std::endl;
				continue;
			}
			throw std::runtime_error(error_msg);
		}
	}
	return true;
}

Rule::Rule(fs::path target, 
       std::vector<fs::path> dependencies, 
       std::vector<std::string> commands)
			 : target_(target)
			 , dependencies_(dependencies)
			 , commands_(commands)
{}
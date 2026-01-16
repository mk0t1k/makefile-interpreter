#include <cstdlib>

#include "rule.h"

bool Rule::IsNeedRebuild() const
{
	if (!fs::exists(target_)) return true;

	if (is_phony_) return true;

	auto target_time = fs::last_write_time(target_);

	for (const fs::path& dependence : dependencies_)
		if (!fs::exists(dependence) || target_time < fs::last_write_time(dependence))
			return true;
	return false;
}

void Rule::Run()
{
	for (std::string& command : commands_)
		int status = system(command.c_str());
}

Rule::Rule(fs::path target, 
       std::vector<fs::path> dependencies, 
       std::vector<std::string> commands)
			 : target_(target)
			 , dependencies_(dependencies)
			 , commands_(commands)
{}
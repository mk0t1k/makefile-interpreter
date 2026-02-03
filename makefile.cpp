#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>

#include "makefile.h"
#include "rule.h"
#include "logger.h"

namespace parsing
{
  std::string LTrim(const std::string& str) 
  {
  	size_t start = 0;
  	while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
   		start++;
			    
    return str.substr(start, str.size() - start);
  }

  std::string RTrim(const std::string& str) 
  {
    if (str.empty()) return str;
    size_t end = str.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(str[end - 1])))
      end--;
      	  	   					    
    return str.substr(0, end);
  }

  std::vector<std::string> ParseCommands(std::ifstream& file)
  {
    std::vector<std::string> commands;
  	std::string line;

    auto current_pos = file.tellg();

  	while(std::getline(file, line))
    {
      std::string trimmed = LTrim(line);
      if (line.empty() || (!trimmed.empty() && trimmed[0] == '#')) continue;

      if (line[0] == '\t')
        commands.push_back(trimmed);
      else
      {
        file.seekg(current_pos);
        break;
      }
    }

    return commands;
  }

  Rule ParseRule(std::string& line, std::ifstream& file)
  {
  	size_t delimetr_pos = line.find(':');
  	if (delimetr_pos == std::string::npos) return Rule();

  	fs::path target = RTrim(line.substr(0, delimetr_pos));

  	std::string deps_str = LTrim(line.substr(delimetr_pos + 1, line.size()));
  	std::istringstream iss(deps_str);
  	fs::path dependence;
  	std::vector<fs::path> dependences;

  	while (iss >> dependence) dependences.push_back(dependence);

    std::vector<std::string> commands = ParseCommands(file);

  	return Rule(target, dependences, commands);
  }

  std::vector<std::string> ParsePhonyTargets(std::string line)
  {
    std::vector<std::string> result;
    std::string targets = LTrim(line.substr(6, line.size()));
    if (targets.empty()) return result;

    std::istringstream iss(targets);
    std::string target;
    while (iss >> target) result.push_back(target);

    return result;
  }
};

void MakeFile::Parse()
{
  std::unordered_set<std::string> phony_targets_temp;
  std::string line;
  bool first_rule = true;
  bool use_default_target = executed_targets_.empty();

  while(std::getline(make_file_stream_, line))
  {
    while (line.ends_with('\\'))
    {
      line.pop_back();
      std::string next_line;
      if (!std::getline(make_file_stream_, next_line))
        break;
      line += next_line;
    }
    std::string trimmed = parsing::LTrim(line);

    if (trimmed.starts_with(".PHONY:"))
    {
      std::vector<std::string> p = parsing::ParsePhonyTargets(line);
      phony_targets_temp.insert(p.begin(), p.end());
      continue;
    }

    if (!line.empty() && (trimmed.empty() || trimmed[0] != '#'))
    {
      Rule rule = parsing::ParseRule(line, make_file_stream_);
      if (!rule.GetTarget().empty())
      {
        std::string target_str = rule.GetTarget().string();
        rules_[target_str] = rule;
        if (use_default_target && first_rule)
        {
          executed_targets_.push_back(target_str);
          first_rule = false;
        }
      }
    }
  }

  for (const auto& phony_target : phony_targets_temp) 
  {
    auto it = rules_.find(phony_target);
    if (it != rules_.end())

      it->second.SetPhony();
  }
}

MakeFile::MakeFile(const std::string& filename, std::vector<std::string> targets)
  : executed_targets_(targets)
{
  make_file_stream_.open(filename);
  if (!make_file_stream_.is_open())
    throw loging::MakeException("Cannot open file: " + filename);
    
  Parse(); 
}

bool MakeFile::PreBuildRec(Rule& rule, const MakeOptions& options)
{
  bool need_rebuild = false;

  for (const fs::path& dependence : rule.GetDependencies())
  {
    auto it = rules_.find(dependence.string());
    if (it != rules_.end())
    {
      bool dep_needs = PreBuildRec(it->second, options);
      if (dep_needs)
        need_rebuild = true;
    }
  }

  bool this_rule_needs = rule.IsNeedRebuild(options);
  if (this_rule_needs)
    need_rebuild = true;

  if (!options.question_only && this_rule_needs)
  {
    rule.Run(options);
  }

  return need_rebuild;
}

bool MakeFile::Execute(const MakeOptions& options)
{
  bool any_need_rebuild = false;

  if (executed_targets_.empty())
    throw loging::MakeException("No target rule found");
  
  for (const auto& executed_target : executed_targets_)
  {
    if (rules_.find(executed_target) == rules_.end())
    {
      std::string error = "Can't find " + executed_target;
      if (options.keep_going)
      {
        loging::LogError(error);
        continue;
      }
      throw loging::MakeException(error);
    }
    
    if (options.keep_going)
    {
      try
      {
        Rule& target_rule = rules_[executed_target];
        bool need = PreBuildRec(target_rule, options);
        if (need)
          any_need_rebuild = true;
      }
      catch (const std::exception& e)
      {
        loging::LogError("Error building target '" + executed_target + "': " + e.what());
      }
    }
    else
    {
      Rule& target_rule = rules_[executed_target];
      bool need = PreBuildRec(target_rule, options);
      if (need)
        any_need_rebuild = true;
    }
  }

  return any_need_rebuild;
}
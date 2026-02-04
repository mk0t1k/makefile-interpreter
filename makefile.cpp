#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <optional>

#include "makefile.h"
#include "rule.h"
#include "pattern_rule.h"
#include "logger.h"

namespace
{
  std::optional<std::string> MatchPattern(const std::string& pattern, const std::string& target)
  {
    size_t pct = pattern.find('%');
    if (pct == std::string::npos) return std::nullopt;

    std::string prefix = pattern.substr(0, pct);
    std::string suffix = pattern.substr(pct + 1);

    if (target.size() < prefix.size() + suffix.size()) return std::nullopt;
    if (!target.starts_with(prefix) || !target.ends_with(suffix)) return std::nullopt;

    size_t stem_len = target.size() - prefix.size() - suffix.size();
    return target.substr(prefix.size(), stem_len);
  }

  std::string SubstituteStem(const std::string& str, const std::string& stem)
  {
    std::string result;
    result.reserve(str.size() + stem.size());
    for (size_t i = 0; i < str.size(); ++i)
    {
      if (str[i] == '%')
        result += stem;
      else
        result += str[i];
    }
    return result;
  }
}

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

  PatternRule ParsePatternRule(std::string& line, std::ifstream& file)
  {
    size_t delim_pos = line.find(':');
    if (delim_pos == std::string::npos) return PatternRule();

    std::string target_pattern = RTrim(line.substr(0, delim_pos));
    if (target_pattern.find('%') == std::string::npos) return PatternRule();

    std::string deps_str = LTrim(line.substr(delim_pos + 1, line.size()));
    std::istringstream iss(deps_str);
    std::vector<std::string> deps;
    std::string dep;
    while (iss >> dep) deps.push_back(dep);

    std::vector<std::string> commands = ParseCommands(file);
    return PatternRule(target_pattern, deps, commands);
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
      size_t colon_pos = line.find(':');
      std::string target_part = (colon_pos != std::string::npos)
        ? parsing::RTrim(line.substr(0, colon_pos)) : std::string();

      if (!target_part.empty() && target_part.find('%') != std::string::npos)
      {
        PatternRule pattern_rule = parsing::ParsePatternRule(line, make_file_stream_);
        if (!pattern_rule.target_pattern.empty())
          pattern_rules_.push_back(pattern_rule);
      }
      else
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

Rule* MakeFile::GetRuleForTarget(const std::string& target)
{
  auto it = rules_.find(target);
  if (it != rules_.end())
    return &it->second;

  auto impl_it = implicit_rules_.find(target);
  if (impl_it != implicit_rules_.end())
    return &impl_it->second;

  for (const PatternRule& pr : pattern_rules_)
  {
    std::optional<std::string> stem = MatchPattern(pr.target_pattern, target);
    if (!stem) continue;

    std::vector<fs::path> resolved_deps;
    for (const std::string& dp : pr.deps)
      resolved_deps.push_back(fs::path(SubstituteStem(dp, *stem)));

    std::vector<std::string> substituted_commands;
    for (const std::string& cmd : pr.commands)
      substituted_commands.push_back(SubstituteStem(cmd, *stem));

    Rule rule(fs::path(target), resolved_deps, substituted_commands);
    implicit_rules_[target] = rule;
    return &implicit_rules_[target];
  }
  return nullptr;
}

bool MakeFile::PreBuildRec(Rule& rule, const MakeOptions& options)
{
  bool need_rebuild = false;

  for (const fs::path& dependence : rule.GetDependencies())
  {
    Rule* dep_rule = GetRuleForTarget(dependence.string());
    if (dep_rule)
    {
      bool dep_needs = PreBuildRec(*dep_rule, options);
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
    Rule* target_rule = GetRuleForTarget(executed_target);
    if (!target_rule)
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
        bool need = PreBuildRec(*target_rule, options);
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
      bool need = PreBuildRec(*target_rule, options);
      if (need)
        any_need_rebuild = true;
    }
  }

  return any_need_rebuild;
}
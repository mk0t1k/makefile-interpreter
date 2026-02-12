#include <sstream>
#include <string>
#include <optional>

#include "makefile.h"
#include "parser.h"
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

MakeFile::MakeFile(const std::string& filename, std::vector<std::string> targets)
  : executed_targets_(targets)
{
  try
  {
    MakefileParser parser(filename);
    MakefileParseResult result = parser.Parse();

    rules_ = result.rules;
    pattern_rules_ = result.pattern_rules;
    vars_ = result.vars;

    for (const auto& phony_target : result.phony_targets)
    {
      auto it = rules_.find(phony_target);
      if (it != rules_.end())
        it->second.SetPhony();
    }

    if (executed_targets_.empty() && !result.default_target.empty())
      executed_targets_.push_back(result.default_target);
  }
  catch (const std::exception& e)
  {
    throw loging::MakeException(e.what());
  }
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

    std::vector<fs::path> resolved_order_only;
    for (const std::string& dp : pr.order_only_deps)
      resolved_order_only.push_back(fs::path(SubstituteStem(dp, *stem)));

    std::vector<std::string> substituted_commands;
    for (const std::string& cmd : pr.commands)
      substituted_commands.push_back(SubstituteStem(cmd, *stem));

    Rule rule(fs::path(target), resolved_deps, resolved_order_only, substituted_commands, stem.value());
    implicit_rules_[target] = rule;
    return &implicit_rules_[target];
  }
  return nullptr;
}

bool MakeFile::PreBuildRec(Rule& rule, const MakeOptions& options)
{
  bool need_rebuild = false;

  for (const fs::path& prereq : rule.GetOrderOnlyPrerequisites())
  {
    Rule* prereq_rule = GetRuleForTarget(prereq.string());
    if (prereq_rule)
      PreBuildRec(*prereq_rule, options);
  }

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
    rule.Run(options);

  return need_rebuild;
}

bool MakeFile::Execute(const MakeOptions& options)
{
  MakeOptions run_opts = options;
  run_opts.vars = vars_;

  bool any_need_rebuild = false;

  if (executed_targets_.empty())
    throw loging::MakeException("No target rule found");
  
  for (const auto& executed_target : executed_targets_)
  {
    Rule* target_rule = GetRuleForTarget(executed_target);
    if (!target_rule)
    {
      std::string error = "Can't find " + executed_target;
      if (run_opts.keep_going)
      {
        loging::LogError(error);
        continue;
      }
      throw loging::MakeException(error);
    }
    
    if (run_opts.keep_going)
    {
      try
      {
        bool need = PreBuildRec(*target_rule, run_opts);
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
      bool need = PreBuildRec(*target_rule, run_opts);
      if (need)
        any_need_rebuild = true;
    }
  }

  return any_need_rebuild;
}
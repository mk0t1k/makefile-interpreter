#pragma once

#include <vector>
#include <string>

struct PatternRule
{
  std::string target_pattern;
  std::vector<std::string> deps;
  std::vector<std::string> order_only_deps;
  std::vector<std::string> commands;

  PatternRule() = default;
  PatternRule(std::string target_pattern,
             std::vector<std::string> deps,
             std::vector<std::string> order_only_deps,
             std::vector<std::string> commands)
    : target_pattern(target_pattern)
    , deps(deps)
    , order_only_deps(order_only_deps)
    , commands(commands)
  {}
};
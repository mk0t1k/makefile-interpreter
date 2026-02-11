#include "parser.h"

#include <cctype>
#include <set>
#include <sstream>
#include <utility>

namespace
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

    while (std::getline(file, line))
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

  std::pair<std::string, std::string> ParseLazyVar(const std::string& line)
  {
    size_t eq_pos = line.find('=');
    if (eq_pos == std::string::npos)
      return {};
    
    std::string name = RTrim(line.substr(0, eq_pos));
    std::string value = LTrim(line.substr(eq_pos + 1));
    return {name, value};
  }

  std::pair<std::string, std::string> ParseImVar(const std::string& line)
  {
    size_t delim_pos = line.find(":=");
    if (delim_pos == std::string::npos)
      return {};
      
    std::string name = RTrim(line.substr(0, delim_pos));
    std::string value = LTrim(line.substr(delim_pos + 2));
    return {name, value};
  }

  std::pair<std::string, std::string> ParseConditionalVar(const std::string& line)
  {
    size_t delim_pos = line.find("?=");
    if (delim_pos == std::string::npos)
      return {};
    
    std::string name = RTrim(line.substr(0, delim_pos));
    std::string value = LTrim(line.substr(delim_pos + 2));
    return {name, value};
  }

  bool FindVariable(const std::string& str, size_t pos,
                        size_t* dollar, std::string* var_name,
                        size_t* end)
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

      *dollar = dollar_pos;
      *var_name = str.substr(var_start, var_end - var_start);
      *end = var_end + 1;
      return true;
    }
    return false;
  }

}

MakefileParser::MakefileParser(const std::string& filename)
{
  file_.open(filename);
  if (!file_.is_open())
    throw std::runtime_error("Cannot open file: " + filename);
}

std::string MakefileParser::ExpandVariables(std::string str, std::set<std::string>* in_progress)
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
    {
      replacement = "";
    }
    else
    {
      ip->insert(var_name);
      auto im_it = im_var_.find(var_name);
      auto lazy_it = lazy_vars_.find(var_name);
      if (im_it != im_var_.end())
        replacement = im_it->second;
      else if (lazy_it != lazy_vars_.end())
        replacement = ExpandVariables(lazy_it->second, ip);
      else
        replacement = "";
      ip->erase(var_name);
    }

    str.replace(dollar, end_pos - dollar, replacement);
    pos = dollar + replacement.size();
  }
  return str;
}

MakefileParseResult MakefileParser::Parse()
{
  MakefileParseResult result;
  std::string line;
  bool first_rule = true;

  while (std::getline(file_, line))
  {
    while (line.ends_with('\\'))
    {
      line.pop_back();
      std::string next_line;
      if (!std::getline(file_, next_line))
        break;
      line += next_line;
    }

    std::string trimmed = LTrim(line);

    if (trimmed.starts_with(".PHONY:"))
    {
      line = ExpandVariables(line);
      std::vector<std::string> p = ParsePhonyTargets(line);
      result.phony_targets.insert(p.begin(), p.end());
      continue;
    }

    if (line[0] != '\t' && !trimmed.empty() && trimmed[0] != '#')
    {
      if (trimmed.find(":=") != std::string::npos)
      {
        auto [name, value] = ParseImVar(trimmed);
        if (!name.empty())
          im_var_[name] = ExpandVariables(value);
        continue;
      }
      if (trimmed.find("?=") != std::string::npos)
      {
        auto [name, value] = ParseConditionalVar(trimmed);
        auto it_im = im_var_.find(name);
        auto it_lazy = lazy_vars_.find(name);

        if (it_im == im_var_.end() && it_lazy == lazy_vars_.end())
          lazy_vars_[name] = value;
        continue;
      }
      if (trimmed.find('=') != std::string::npos)
      {
        auto [name, value] = ParseLazyVar(trimmed);
        if (!name.empty())
          lazy_vars_[name] = value;
        continue;
      }
    }

    if (!line.empty() && (trimmed.empty() || trimmed[0] != '#'))
    {
      size_t colon_pos = line.find(':');
      std::string target_part = (colon_pos != std::string::npos)
        ? RTrim(line.substr(0, colon_pos)) : std::string();

      if (!target_part.empty())
      {
        line = ExpandVariables(line);
        colon_pos = line.find(':');
        target_part = (colon_pos != std::string::npos)
          ? RTrim(line.substr(0, colon_pos)) : std::string();
      }

      if (!target_part.empty() && target_part.find('%') != std::string::npos)
      {
        PatternRule pattern_rule = ParsePatternRule(line, file_);
        if (!pattern_rule.target_pattern.empty())
          result.pattern_rules.push_back(pattern_rule);
      }
      else if (!target_part.empty())
      {
        Rule rule = ParseRule(line, file_);
        if (!rule.GetTarget().empty())
        {
          std::string target_str = rule.GetTarget().string();
          result.rules[target_str] = rule;
          if (first_rule)
          {
            result.default_target = target_str;
            first_rule = false;
          }
        }
      }
    }
  }

  result.vars = im_var_;
  for (const auto& [k, v] : lazy_vars_)
    result.vars[k] = v;
  return result;
}
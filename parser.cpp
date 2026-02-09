#include "parser.h"

#include <cctype>
#include <sstream>

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
}

MakefileParser::MakefileParser(const std::string& filename)
{
  file_.open(filename);
  if (!file_.is_open())
    throw std::runtime_error("Cannot open file: " + filename);
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
      std::vector<std::string> p = ParsePhonyTargets(line);
      result.phony_targets.insert(p.begin(), p.end());
      continue;
    }

    if (!line.empty() && (trimmed.empty() || trimmed[0] != '#'))
    {
      size_t colon_pos = line.find(':');
      std::string target_part = (colon_pos != std::string::npos)
        ? RTrim(line.substr(0, colon_pos)) : std::string();

      if (!target_part.empty() && target_part.find('%') != std::string::npos)
      {
        PatternRule pattern_rule = ParsePatternRule(line, file_);
        if (!pattern_rule.target_pattern.empty())
          result.pattern_rules.push_back(pattern_rule);
      }
      else
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

  return result;
}
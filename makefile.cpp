#include <fstream>
#include <sstream>
#include <string>
#include <stdexcept>

#include "makefile.h"
#include "rule.h"

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
};

void MakeFile::Parse()
{
  std::string line;
  bool first_rule = true;

    while(std::getline(make_file_stream_, line))
    {
      std::string trimmed = parsing::LTrim(line);
      if (!line.empty() && (trimmed.empty() || trimmed[0] != '#'))
      {
        Rule rule = parsing::ParseRule(line, make_file_stream_);
        if (!rule.GetTarget().empty())
        {
          std::string target_str = rule.GetTarget().string();
          rules_[target_str] = rule;
          if (first_rule)
          {
            first_target_ = target_str;
            first_rule = false;
          }
        }
      }
    }

}

MakeFile::MakeFile(const std::string& filename)
{
  make_file_stream_.open(filename);
  if (!make_file_stream_.is_open())
    throw std::runtime_error("Cannot open file: " + filename);
    
  Parse(); 
}

void MakeFile::PreBuildRec(Rule& rule)
{
  for (const fs::path& dependence : rule.GetDependencies())
  {
    auto it = rules_.find(dependence.string());
    if (it != rules_.end())
    {
      PreBuildRec(it->second);
      if (it->second.IsNeedRebuild())
        it->second.Run();
    }
  }
  
  if (rule.IsNeedRebuild())
    rule.Run();
}

void MakeFile::Execute()
{
  if (first_target_.empty() || rules_.find(first_target_) == rules_.end())
    throw std::runtime_error("No target rule found");
    
  Rule& target_rule = rules_[first_target_];
  PreBuildRec(target_rule);
}
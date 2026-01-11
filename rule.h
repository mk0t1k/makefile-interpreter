#pragma once
#include <filesystem>
#include <vector>
#include <string>

namespace fs = std::filesystem;

class Rule
{
  fs::path target_;
  std::vector<fs::path> dependencies_;
  std::vector<std::string> commands_;

public:
  Rule(fs::path target, 
       std::vector<fs::path> dependencies, 
       std::vector<std::string> commands);
  
  Rule() = default;

  fs::path GetTarget() const {return target_;}
  std::vector<fs::path> GetDependencies() const {return dependencies_;}

  bool IsNeedRebuild() const;
  void Run();
};

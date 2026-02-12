#pragma once
#include <filesystem>
#include <vector>
#include <string>

#include "options.h"

namespace fs = std::filesystem;

class Rule
{
  fs::path target_;
  std::vector<fs::path> dependencies_;
  std::vector<std::string> commands_;
  bool is_phony_ = false;
  std::string stem_;
  std::vector<fs::path> order_only_prerequisites_;

  std::string PrepareCommand(std::string command, const MakeOptions& options);

public:
  Rule(fs::path target,
       std::vector<fs::path> dependencies,
       std::vector<fs::path> prereqs, 
       std::vector<std::string> commands,
       std::string stem = "");
  
  Rule() = default;

  fs::path GetTarget() const {return target_;}
  std::vector<fs::path> GetDependencies() const {return dependencies_;}
  const std::vector<fs::path>& GetOrderOnlyPrerequisites() const {return order_only_prerequisites_;}

  void SetPhony() {is_phony_ = true;}

  bool CheckOrderOnlyPrerequisites() const;
  bool IsNeedRebuild(const MakeOptions& options) const;
  bool Run(const MakeOptions& options);
};

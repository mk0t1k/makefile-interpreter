#pragma once
#include <unordered_map>
#include <fstream>

#include "rule.h"

struct MakeOptions 
{
  bool dry_run = false;
  bool silent = false;
  bool keep_going = false;
};

class MakeFile
{
	std::unordered_map<std::string, Rule> rules_;
	std::ifstream make_file_stream_;
	std::vector<std::string> executed_targets_;

	void Parse();
	void PreBuildRec(Rule& rule, const MakeOptions& options);

public:
	MakeFile(const std::string& filename, std::vector<std::string> targets);
	~MakeFile() = default;

	void Execute(const MakeOptions& options = {});
};
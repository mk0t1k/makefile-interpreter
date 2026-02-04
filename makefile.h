#pragma once
#include <unordered_map>
#include <fstream>
#include <optional>

#include "rule.h"
#include "pattern_rule.h"
#include "options.h"

class MakeFile
{
	std::unordered_map<std::string, Rule> rules_;
	std::vector<PatternRule> pattern_rules_;
	std::unordered_map<std::string, Rule> implicit_rules_;
	std::ifstream make_file_stream_;
	std::vector<std::string> executed_targets_;

	void Parse();
	bool PreBuildRec(Rule& rule, const MakeOptions& options);
	Rule* GetRuleForTarget(const std::string& target);

public:
	MakeFile(const std::string& filename, std::vector<std::string> targets);
	~MakeFile() = default;

	bool Execute(const MakeOptions& options = {});
};
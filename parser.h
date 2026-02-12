#pragma once

#include <fstream>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "rule.h"
#include "pattern_rule.h"

struct MakefileParseResult
{
	std::unordered_map<std::string, Rule> rules;
	std::vector<PatternRule> pattern_rules;
	std::unordered_set<std::string> phony_targets;
	std::string default_target;

	std::unordered_map<std::string, std::string> vars;
};

class MakefileParser
{
public:
	explicit MakefileParser(const std::string& filename);

	MakefileParseResult Parse();

private:
	std::ifstream file_;

	std::string ExpandVariables(std::string str, std::set<std::string>* in_progress = nullptr);
	void LoadEnvVars();

	std::unordered_map<std::string, std::string> lazy_vars_;
	std::unordered_map<std::string, std::string> im_var_;
};

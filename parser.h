#pragma once

#include <fstream>
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
};

class MakefileParser
{
public:
	explicit MakefileParser(const std::string& filename);

	MakefileParseResult Parse();

private:
	std::ifstream file_;
};

#pragma once
#include <unordered_map>
#include <fstream>

#include "rule.h"

class MakeFile
{
	std::unordered_map<std::string, Rule> rules_;
	std::ifstream make_file_stream_;
	std::string first_target_;

	void Parse();
	void PreBuildRec(Rule& rule);

public:
	MakeFile(const std::string& filename);
	~MakeFile() = default;

	void Execute();
};
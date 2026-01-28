#pragma once

#include <string>
#include <stdexcept>
#include <iostream>

namespace loging 
{

inline std::string MakeMessage(const std::string& message)
{
  return "[make]: " + message;
}

class MakeException : public std::runtime_error
{
public:
  explicit MakeException(const std::string& message)
    : std::runtime_error(MakeMessage(message))
  {}
};

inline void LogError(const std::string& message)
{
  std::cerr << MakeMessage(message) << '\n';
}

inline void LogInfo(const std::string& message)
{
  std::cout << MakeMessage(message) << '\n';
}

} // namespace loging
#include "argument.h"

#include <cerrno>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>

namespace nargparse 
{

namespace 
{

bool ParseInt(const char* s, int* out) 
{
  if (!s || !*s) return false;
  errno = 0;
  char* end = nullptr;
  long v = std::strtol(s, &end, 10);
  if (end == s || *end != '\0') return false;
  if (errno == ERANGE || v < INT_MIN || v > INT_MAX) return false;
  *out = static_cast<int>(v);
  return true;
}

bool ParseFloat(const char* s, float* out) 
{
  if (!s || !*s) return false;
  errno = 0;
  char* end = nullptr;
  float v = std::strtof(s, &end);
  if (end == s || *end != '\0') return false;
  if (errno == ERANGE || std::isinf(v)) return false;
  *out = v;
  return true;
}

} // namespace 

template<>
bool Argument<int>::ParseValue(const char* value, size_t max_arg_len) 
{
  if (!value) return false;
  
  int v = 0;
  if (!ParseInt(value, &v)) return false;
  
  if (!ValidateValue(&v)) return false;
  
  repeating_argument_values_.push_back(v);
  values_count_++;
  
  if (GetTarget() && values_count_ == 1)
    *GetTarget() = v;
  
  return true;
}

template<>
bool Argument<float>::ParseValue(const char* value, size_t max_arg_len) 
{
  if (!value) return false;
  
  float v = 0.0f;
  if (!ParseFloat(value, &v)) return false;
  
  if (!ValidateValue(&v)) return false;
  
  repeating_argument_values_.push_back(v);
  values_count_++;
  
  if (GetTarget() && values_count_ == 1) 
    *GetTarget() = v;
  
  return true;
}

template<>
bool Argument<std::string>::ParseValue(const char* value, size_t max_arg_len) 
{
  if (!value) return false;
  
  if (std::strlen(value) >= max_arg_len) return false;
  
  std::string v(value);
  if (!ValidateValue(&v)) return false;
  
  repeating_argument_values_.push_back(v);
  values_count_++;
  
  if (GetTarget() && values_count_ == 1)
    *GetTarget() = v;
  
  return true;
}

template<>
bool Argument<bool>::ParseValue(const char* value, size_t max_arg_len) 
{
  if (GetTarget())
    *GetTarget() = true;

  repeating_argument_values_.push_back(true);
  values_count_++;
  return true;
}

} // namespace nargparse



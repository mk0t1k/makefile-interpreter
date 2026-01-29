#pragma once
#include <cstddef>
#include <vector>
#include <string>

namespace nargparse 
{

enum NargsType 
{
  kNargsRequired = 0,
  kNargsOptional,
  kNargsZeroOrMore,
  kNargsOneOrMore
};

class ArgumentBase 
{
public:
  std::string short_name_;
  std::string long_name_;
  std::string name_;  
    
  NargsType nargs_ = kNargsOptional;
  bool is_flag_ = false;
  bool is_positional_ = false;
  std::string validation_error;
  
  int values_count_ = 0;

  ArgumentBase() = default;
  virtual ~ArgumentBase() = default;
  
  virtual bool ParseValue(const char* value, size_t max_arg_len) = 0;
  virtual bool ValidateValue(const void* value) = 0;
  virtual void AssignFirstValue() = 0;
};

template<typename T>
class Argument : public ArgumentBase
{
  T* target_ = nullptr;
  bool (*validator_)(const T&) = nullptr;
  std::vector<T> repeating_argument_values_;

public:
  Argument() = default;
  ~Argument() override = default;
  
  void SetTarget(T* target) { target_ = target; }
  void SetValidator(bool (*validator)(const T&)) { validator_ = validator; }
  
  bool ParseValue(const char* value, size_t max_arg_len) override;
  bool ValidateValue(const void* value) override;
  void AssignFirstValue() override;
  
  const std::vector<T>& GetValues() const { return repeating_argument_values_; }
  T* GetTarget() const { return target_; }
};

template<typename T>
bool Argument<T>::ValidateValue(const void* value) 
{
  if (!validator_ || !value) return true;
  return validator_(*static_cast<const T*>(value));
}

template<typename T>
void Argument<T>::AssignFirstValue() 
{
  if (target_ && !repeating_argument_values_.empty())
    *target_ = repeating_argument_values_.front();
}

template<>
bool Argument<int>::ParseValue(const char* value, size_t max_arg_len);

template<>
bool Argument<float>::ParseValue(const char* value, size_t max_arg_len);

template<>
bool Argument<std::string>::ParseValue(const char* value, size_t max_arg_len);

template<>
bool Argument<bool>::ParseValue(const char* value, size_t max_arg_len);

} // namespace nargparse
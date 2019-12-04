// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_JSVALUEREADER
#define MICROSOFT_REACTNATIVE_JSVALUEREADER

#include "JSValue.h"
#include "JSValueTreeReader.h"
#include "winrt/Microsoft.ReactNative.Bridge.h"

namespace winrt::Microsoft::ReactNative::Bridge {

// A value can be read from IJSValueReader in one of three ways:
// 1. Using a ReadValue standalone function with IJSValueReader& as a first argument.
// 2. Using a ReadValue standalone function with const JSValue& as a first argument.
// 3. We can auto-generate the read method for the type in some cases.

// Forward declarations
template <class T>
T ReadValue(IJSValueReader &reader) noexcept;
template <class T>
void ReadValue(IJSValueReader &reader, /*out*/ T &value) noexcept;

void ReadValue(IJSValueReader &reader, /*out*/ std::string &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ std::wstring &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ bool &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ int64_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ int8_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ int16_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ int32_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ int64_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ uint8_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ uint16_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ uint32_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ uint64_t &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ float &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ double &value) noexcept;
template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 1>
void ReadValue(IJSValueReader &reader, /*out*/ T &value) noexcept;
template <class T>
void ReadValue(IJSValueReader &reader, /*out*/ std::optional<T> &value) noexcept;
template <class T, class TCompare = std::less<>, class TAlloc = std::allocator<pair<const std::string, T>>>
void ReadValue(IJSValueReader &reader, /*out*/ std::map<std::string, T, TCompare, TAlloc> &value) noexcept;
template <class T, class TCompare = std::less<>, class TAlloc = std::allocator<pair<const std::string, T>>>
void ReadValue(IJSValueReader &reader, /*out*/ std::map<std::wstring, T, TCompare, TAlloc> &value) noexcept;
template <class T, class TAlloc = allocator<T>>
void ReadValue(IJSValueReader &reader, /*out*/ std::vector<T, TAlloc> &value) noexcept;
template <class... Ts>
void ReadValue(IJSValueReader &reader, /*out*/ std::tuple<Ts...> &value) noexcept;

void ReadValue(IJSValueReader &reader, /*out*/ JSValue &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ JSValueObject &value) noexcept;
void ReadValue(IJSValueReader &reader, /*out*/ JSValueArray &value) noexcept;

template <class T>
void ReadValue(const JSValue &jsValue, /*out*/ T &value) noexcept;
template <class T>
T ReadValue(const JSValue &jsValue) noexcept;

bool SkipArrayToEnd(IJSValueReader &reader) noexcept;
template <class... TArgs>
void ReadArgs(IJSValueReader &reader, /*out*/ TArgs &... args) noexcept;

//===========================================================================
// IJSValueReader extensions implementation
//===========================================================================

template <class T>
inline T ReadValue(IJSValueReader &reader) noexcept {
  T result;
  ReadValue(reader, /*out*/ result);
  return result;
}

template <class T>
inline void ReadValue(IJSValueReader &reader, /*out*/ T &value) noexcept {
  // TODO: add call to ReadValue with JSValue first parameter
  // TODO: add support for attributed structs
  static_assert(sizeof(std::decay_t<T>) == 0, "Implement ReadValue for the T type");
}

inline void ReadValue(IJSValueReader &reader, /*out*/ std::string &value) noexcept {
  switch (reader.ValueType()) {
    case JSValueType::String:
      value = to_string(reader.GetString());
      break;
    case JSValueType::Boolean:
      value = reader.GetBoolean() ? "true" : "false";
      break;
    case JSValueType::Int64:
      value = std::to_string(reader.GetInt64());
      break;
    case JSValueType::Double:
      value = std::to_string(reader.GetDouble());
      break;
    default:
      value = "";
      break;
  }
}

inline void ReadValue(IJSValueReader &reader, /*out*/ std::wstring &value) noexcept {
  switch (reader.ValueType()) {
    case JSValueType::String:
      value = reader.GetString();
      break;
    case JSValueType::Boolean:
      value = reader.GetBoolean() ? L"true" : L"false";
      break;
    case JSValueType::Int64:
      value = std::to_wstring(reader.GetInt64());
      break;
    case JSValueType::Double:
      value = std::to_wstring(reader.GetDouble());
      break;
    default:
      value = L"";
      break;
  }
}

inline void ReadValue(IJSValueReader &reader, /*out*/ bool &value) noexcept {
  switch (reader.ValueType()) {
    case JSValueType::String:
      value = !to_string(reader.GetString()).empty();
      break;
    case JSValueType::Boolean:
      value = reader.GetBoolean();
      break;
    case JSValueType::Int64:
      value = reader.GetInt64() != 0;
      break;
    case JSValueType::Double:
      value = reader.GetDouble() != 0;
      break;
    default:
      value = false;
      break;
  }
}

inline void ReadValue(IJSValueReader &reader, /*out*/ int8_t &value) noexcept {
  int64_t val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<int8_t>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ int16_t &value) noexcept {
  int64_t val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<int16_t>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ int32_t &value) noexcept {
  int64_t val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<int32_t>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ int64_t &value) noexcept {
  switch (reader.ValueType()) {
    case JSValueType::String: {
      hstring str = reader.GetString();
      wchar_t *end = nullptr;
      auto iValue = _wcstoi64(str.data(), &end, 10 /*base*/);
      if (end == str.data() + str.size()) {
        value = iValue;
      } else {
        value = 0;
      }
      break;
    }
    case JSValueType::Boolean:
      value = reader.GetBoolean() ? 1 : 0;
      break;
    case JSValueType::Int64:
      value = reader.GetInt64();
      break;
    case JSValueType::Double:
      value = static_cast<int64_t>(reader.GetDouble());
      break;
    default:
      value = 0;
      break;
  }
}

inline void ReadValue(IJSValueReader &reader, /*out*/ uint8_t &value) noexcept {
  int64_t val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<uint8_t>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ uint16_t &value) noexcept {
  int64_t val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<uint16_t>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ uint32_t &value) noexcept {
  int64_t val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<uint32_t>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ uint64_t &value) noexcept {
  int64_t val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<uint64_t>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ float &value) noexcept {
  double val;
  ReadValue(reader, /*out*/ val);
  value = static_cast<float>(val);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ double &value) noexcept {
  switch (reader.ValueType()) {
    case JSValueType::String: {
      hstring str = reader.GetString();
      wchar_t *end = nullptr;
      auto dvalue = wcstod(str.data(), &end);
      if (end == str.data() + str.size()) {
        value = dvalue;
      } else {
        value = 0;
      }
      break;
    }
    case JSValueType::Boolean:
      value = reader.GetBoolean() ? 1 : 0;
      break;
    case JSValueType::Int64:
      value = static_cast<double>(reader.GetInt64());
      break;
    case JSValueType::Double:
      value = reader.GetDouble();
      break;
    default:
      value = 0;
      break;
  }
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int>>
inline void ReadValue(IJSValueReader &reader, /*out*/ T &value) noexcept {
  int32_t intValue;
  ReadValue(reader, intValue);
  value = static_cast<T>(intValue);
}

template <class T>
inline void ReadValue(IJSValueReader &reader, /*out*/ std::optional<T> &value) noexcept {
  if (reader.ValueType != JSValueType.Null) {
    value = ReadValue<T>(reader);
  } else {
    value = std::nullopt;
  }
}

// Note that we use std::less<> as a default comparer instead of standard std::less<Key>.
// This is to enable use of string-like objects such as std::string_view as key to retrieve values.
// While std::less<> is better, the standard cannot have a breaking change to switch to it.
template <class T, class TCompare, class TAlloc>
inline void ReadValue(IJSValueReader &reader, /*out*/ std::map<std::string, T, TCompare, TAlloc> &value) noexcept {
  if (reader.ValueType == JSValueType.Object) {
    hstring propertyName;
    while (reader.GetNextObjectProperty(/*out*/ propertyName)) {
      value.emplace(to_string(propertyName), ReadValue<T>(reader));
    }
  }
}

template <class T, class TCompare, class TAlloc>
inline void ReadValue(IJSValueReader &reader, /*out*/ std::map<std::wstring, T, TCompare, TAlloc> &value) noexcept {
  if (reader.ValueType == JSValueType.Object) {
    hstring propertyName;
    while (reader.GetNextObjectProperty(/*out*/ propertyName)) {
      value.emplace(propertyName, ReadValue<T>(reader));
    }
  }
}

template <class T, class TAlloc>
inline void ReadValue(IJSValueReader &reader, /*out*/ std::vector<T, TAlloc> &value) noexcept {
  if (reader.ValueType == JSValueType.Array) {
    while (reader.GetNextArrayItem()) {
      value.push_back(ReadValue<T>(reader));
    }
  }
}

inline void ReadValue(IJSValueReader &reader, /*out*/ JSValue &value) noexcept {
  value = JSValue::ReadFrom(reader);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ JSValueObject &value) noexcept {
  value = JSValue::ReadObjectFrom(reader);
}

inline void ReadValue(IJSValueReader &reader, /*out*/ JSValueArray &value) noexcept {
  value = JSValue::ReadArrayFrom(reader);
}

template <class... Ts>
inline void ReadValue(IJSValueReader &reader, /*out*/ std::tuple<Ts...> &value) noexcept {
  Ts... args;
  ReadArgs(reader, /*out*/ args...);
  value = std::tuple<Ts...>(args...);
}

template <class T>
inline void ReadValue(const JSValue &jsValue, /*out*/ T &value) noexcept {
  ReadValue(make<JSValueTreeReader>(jsValue), /*out*/ value);
}

template <class T>
inline T ReadValue(const JSValue &jsValue) noexcept {
  T value;
  ReadValue(make<JSValueTreeReader>(jsValue), /*out*/ value);
  return value;
}

// It helps to read arguments from an array if there are more items than expected.
inline bool SkipArrayToEnd(IJSValueReader &reader) noexcept {
  while (reader.GetNextArrayItem()) {
    ReadValue<JSValue>(reader); // Read and ignore the value
  }

  return true;
}

template <class... TArgs>
inline void ReadArgs(IJSValueReader &reader, /*out*/ TArgs &... args) noexcept {
  // Read as many arguments as we can or return default values.
  bool success = reader.ValueType == JSValueType.Array;

  // To read variadic template arguments in natural order we must use them in an initializer list.
  // TODO: can we fold expression instead?
  [[maybe_unused]] int dummy[] = {
      (success = success && reader.GetNextArrayItem(), args = success ? ReadValue<TArgs>(reader) : TArgs{}, 0)...};
  success = success && SkipArrayToEnd(reader);
}

} // namespace winrt::Microsoft::ReactNative::Bridge

#endif // MICROSOFT_REACTNATIVE_JSVALUEREADER

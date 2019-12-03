// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_JSVALUEREADER
#define MICROSOFT_REACTNATIVE_JSVALUEREADER

#include "winrt/Microsoft.ReactNative.Bridge.h"

namespace winrt::Microsoft::ReactNative::Bridge {

// A value can be read from IJSValueReader in one of three ways:
// 1. Using a ReadValue standalone function with IJSValueReader& as a first argument.
// 2. Using a ReadValue standalone function with const JSValue& as a first argument.
// 3. We can auto-generate the read method for the type in some cases.

template <class T>
T ReadValue(IJSValueReader &reader) noexcept;
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

template <class T>
inline T ReadValue(IJSValueReader &reader) noexcept {
  T result;
  ReadValue(reader, /*out*/ result);
  return result;
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

template <class T>
inline void ReadValue(IJSValueReader &reader, /*out*/T &value) noexcept {
  static_assert(sizeof(std::decay_t<T>) == 0, "Implement ReadValue for the T type");
}


////template <class... TArgs>
////inline void ReadArgs(winrt::Microsoft::ReactNative::Bridge::IJSValueReader const &reader, TArgs &... args) noexcept
///{ /  if constexpr (sizeof...(args) > 0) { /    using winrt::Microsoft::ReactNative::Bridge::JSValueReaderState; /
/// JSValueReaderState state = reader.ReadNext(); /    if (state == JSValueReaderState::ArrayBegin) { /      // To read
/// variadic template arguments in natural order we must use them /      // in an initializer list. / [[maybe_unused]]
/// int dummy[] = { /          (state = reader.ReadNext(), state != JSValueReaderState::ArrayEnd && ReadValue(reader,
/// args), 0)...}; /    } /  }
////}

} // namespace winrt::Microsoft::ReactNative::Bridge

#endif // MICROSOFT_REACTNATIVE_JSVALUEREADER

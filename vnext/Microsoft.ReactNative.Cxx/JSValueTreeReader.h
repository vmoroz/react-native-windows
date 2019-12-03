// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_JSVALUETREEREADER
#define MICROSOFT_REACTNATIVE_JSVALUETREEREADER

#include "IJSValueTreeReader.h"

namespace winrt::Microsoft::ReactNative::Bridge {

struct JSValueTreeReader : implements<JSValueTreeReader, IJSValueReader, IJSValueTreeReader> {
  JSValueTreeReader(const JSValue &value) noexcept;
  JSValueTreeReader(JSValue &&value) noexcept;

 public: // IJSValueReader
  JSValueType ValueType() noexcept;
  bool GetNextObjectProperty(hstring &propertyName) noexcept;
  bool GetNextArrayItem() noexcept;
  hstring GetString() noexcept;
  bool GetBoolean() noexcept;
  int64_t GetInt64() noexcept;
  double GetDouble() noexcept;

 public: // IJSValueTreeReader
  const JSValue &Current() noexcept;
  const JSValue &Root() noexcept;

 private:
  struct StackEntry {
    StackEntry(const JSValue &value, const JSValueObject::const_iterator &property) noexcept;
    StackEntry(const JSValue &value, const JSValueArray::const_iterator &item) noexcept;

    const JSValue& Value;
    JSValueArray::const_iterator Item;
    JSValueObject::const_iterator Property;
  };

 private:
  void SetCurrentValue(const JSValue &value) noexcept;

 private:
  const JSValue m_ownedValue;
  const JSValue &m_root;
  const JSValue *m_current;
  bool m_isInContainer{false};
  std::vector<StackEntry> m_stack;
};

} // namespace winrt::Microsoft::ReactNative::Bridge

#endif // MICROSOFT_REACTNATIVE_JSVALUETREEREADER

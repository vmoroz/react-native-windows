// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <unknwn.h>
#include "JSValue.h"
#include "winrt/Microsoft.ReactNative.Bridge.h"

namespace winrt::Microsoft::ReactNative::Bridge {

struct __declspec(uuid("2abb7c60-88d7-45e7-bb72-863c117a0c00")) IJSValueTreeReader
  : ::IUnknown {
  virtual JSValue Current() noexcept = 0;
  virtual JSValue Root() noexcept = 0;
};

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
  JSValue Current() noexcept;
  JSValue Root() noexcept;

 private:
  struct StackEntry {
    static StackEntry ObjectProperty(
        const JSValue *value,
        const JSValueObject::const_iterator &property) noexcept;
    static StackEntry ArrayItem(const JSValue *value, const JSValueArray::const_iterator &item) noexcept;

    const JSValue *Value{nullptr};
    JSValueArray::const_iterator Item;
    JSValueObject::const_iterator Property;
  };

 private:
  void SetCurrentValue(const JSValue *value) noexcept;

 private:
  const JSValue m_ownedValue;
  const JSValue& m_root;
  const JSValue *m_current{nullptr};
  bool m_isInContainer{false};
  std::vector<StackEntry> m_stack;
};

} // namespace winrt::Microsoft::ReactNative::Bridge

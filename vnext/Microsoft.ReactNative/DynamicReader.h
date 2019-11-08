// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "folly/dynamic.h"
#include "winrt/Microsoft.ReactNative.Bridge.h"

namespace winrt::Microsoft::ReactNative::Bridge {

struct DynamicReader : implements<DynamicReader, IJSValueReader> {
  DynamicReader(const folly::dynamic &root) noexcept;

 public: // IJSValueReader
  JSValueReaderState ReadNext() noexcept;
  hstring GetPropertyName() noexcept;
  hstring GetString() noexcept;
  bool GetBoolean() noexcept;
  int64_t GetInt64() noexcept;
  double GetDouble() noexcept;

 private:
  struct StackEntry {
    static StackEntry ObjectProperty(
        const folly::dynamic *value,
        const folly::dynamic::const_item_iterator &property) noexcept;
    static StackEntry ArrayItem(const folly::dynamic *value, const folly::dynamic::const_iterator &item) noexcept;

    const folly::dynamic *Value{nullptr};
    folly::dynamic::const_iterator Item;
    std::optional<folly::dynamic::const_item_iterator> Property;
  };

 private:
  JSValueReaderState ReadValue(const folly::dynamic *value) noexcept;
  JSValueReaderState ReadObject() noexcept;
  JSValueReaderState ReadArray() noexcept;
  JSValueReaderState ReadNextValue() noexcept;
  JSValueReaderState ReadNextObjectProperty() noexcept;
  JSValueReaderState ReadNextArrayItem() noexcept;

 private:
  // Special initial internal state that we never return.
  static constexpr JSValueReaderState StartState = static_cast<JSValueReaderState>(-1);

 private:
  const folly::dynamic *m_root{nullptr};
  const folly::dynamic *m_current{nullptr};
  JSValueReaderState m_state{StartState};
  std::vector<StackEntry> m_stack;
};

} // namespace winrt::Microsoft::ReactNative::Bridge

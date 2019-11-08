// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "DynamicReader.h"
#include "Crash.h"

namespace winrt::Microsoft::ReactNative::Bridge {

//===========================================================================
// DynamicReader implementation
//===========================================================================

/*static*/ DynamicReader::StackEntry DynamicReader::StackEntry::ObjectProperty(
    const folly::dynamic *value,
    const folly::dynamic::const_item_iterator &property) noexcept {
  StackEntry entry;
  entry.Value = value;
  entry.Property = property;
  return entry;
}

/*static*/ DynamicReader::StackEntry DynamicReader::StackEntry::ArrayItem(
    const folly::dynamic *value,
    const folly::dynamic::const_iterator &item) noexcept {
  StackEntry entry;
  entry.Value = value;
  entry.Item = item;
  return entry;
}

DynamicReader::DynamicReader(const folly::dynamic &root) noexcept : m_root{&root} {}

JSValueReaderState DynamicReader::ReadNext() noexcept {
  if (m_state == StartState) {
    return ReadValue(m_root);
  }

  switch (m_state) {
    case JSValueReaderState::ObjectBegin:
      return ReadObject();
    case JSValueReaderState::ArrayBegin:
      return ReadArray();
    case JSValueReaderState::NullValue:
    case JSValueReaderState::ObjectEnd:
    case JSValueReaderState::ArrayEnd:
    case JSValueReaderState::StringValue:
    case JSValueReaderState::BooleanValue:
    case JSValueReaderState::Int64Value:
    case JSValueReaderState::DoubleValue:
      return ReadNextValue();
    default:
      return m_state = JSValueReaderState::Error;
  }
}

JSValueReaderState DynamicReader::ReadValue(const folly::dynamic *value) noexcept {
  if (!value) {
    return m_state = JSValueReaderState::Error;
  }

  m_current = value;
  switch (value->type()) {
    case folly::dynamic::Type::NULLT:
      return m_state = JSValueReaderState::NullValue;
    case folly::dynamic::Type::BOOL:
      return m_state = JSValueReaderState::BooleanValue;
    case folly::dynamic::Type::INT64:
      return m_state = JSValueReaderState::Int64Value;
    case folly::dynamic::Type::DOUBLE:
      return m_state = JSValueReaderState::DoubleValue;
    case folly::dynamic::Type::STRING:
      return m_state = JSValueReaderState::StringValue;
    case folly::dynamic::Type::OBJECT:
      return m_state = JSValueReaderState::ObjectBegin;
    case folly::dynamic::Type::ARRAY:
      return m_state = JSValueReaderState::ArrayBegin;
    default:
      return m_state = JSValueReaderState::Error;
  }
}

JSValueReaderState DynamicReader::ReadObject() noexcept {
  const auto &properties = m_current->items();
  const auto &property = properties.begin();
  if (property != properties.end()) {
    m_stack.push_back(StackEntry::ObjectProperty(m_current, property));
    return ReadValue(&(property->second));
  } else {
    return m_state = JSValueReaderState::ObjectEnd;
  }
}

JSValueReaderState DynamicReader::ReadArray() noexcept {
  const auto &item = m_current->begin();
  if (item != m_current->end()) {
    m_stack.push_back(StackEntry::ArrayItem(m_current, item));
    return ReadValue(&*item);
  } else {
    return m_state = JSValueReaderState::ArrayEnd;
  }
}

JSValueReaderState DynamicReader::ReadNextValue() noexcept {
  if (m_stack.empty()) {
    return m_state = JSValueReaderState::Error;
  }

  switch (m_stack.back().Value->type()) {
    case folly::dynamic::OBJECT:
      return ReadNextObjectProperty();
    case folly::dynamic::ARRAY:
      return ReadNextArrayItem();
    default:
      return m_state = JSValueReaderState::Error;
  }
}

JSValueReaderState DynamicReader::ReadNextObjectProperty() noexcept {
  auto &entry = m_stack.back();
  auto &property = entry.Property.value();
  if (++property != entry.Value->items().end()) {
    return ReadValue(&(property->second));
  } else {
    m_current = entry.Value;
    m_stack.pop_back();
    return m_state = JSValueReaderState::ObjectEnd;
  }
}

JSValueReaderState DynamicReader::ReadNextArrayItem() noexcept {
  auto &entry = m_stack.back();
  if (++entry.Item != entry.Value->end()) {
    return ReadValue(&*entry.Item);
  } else {
    m_current = entry.Value;
    m_stack.pop_back();
    return m_state = JSValueReaderState::ArrayEnd;
  }
}

hstring DynamicReader::GetPropertyName() noexcept {
  if (!m_stack.empty()) {
    auto &entry = m_stack.back();
    if (entry.Value->type() == folly::dynamic::OBJECT) {
      return to_hstring(entry.Property.value()->first.asString());
    }
  }

  return to_hstring(L"");
}

hstring DynamicReader::GetString() noexcept {
  return to_hstring((m_state == JSValueReaderState::BooleanValue) ? m_current->getString() : "");
}

bool DynamicReader::GetBoolean() noexcept {
  return (m_state == JSValueReaderState::BooleanValue) ? m_current->getBool() : false;
}

int64_t DynamicReader::GetInt64() noexcept {
  return (m_state == JSValueReaderState::Int64Value) ? m_current->getInt() : 0;
}

double DynamicReader::GetDouble() noexcept {
  return (m_state == JSValueReaderState::DoubleValue) ? m_current->getDouble() : 0;
}

} // namespace winrt::Microsoft::ReactNative::Bridge

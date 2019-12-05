// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "Crash.h"
#include "JSValueTreeWriter.h"

namespace winrt::Microsoft::ReactNative::Bridge {

//===========================================================================
// DynamicWriter implementation
//===========================================================================

JSValue JSValueTreeWriter::TakeValue() noexcept {
  return std::move(m_result);
}

void JSValueTreeWriter::WriteNull() noexcept {
  WriteValue(JSValue{});
}

void JSValueTreeWriter::WriteBoolean(bool value) noexcept {
  WriteValue(JSValue{value});
}

void JSValueTreeWriter::WriteInt64(int64_t value) noexcept {
  WriteValue(JSValue{value});
}

void JSValueTreeWriter::WriteDouble(double value) noexcept {
  WriteValue(JSValue{value});
}

void JSValueTreeWriter::WriteString(const hstring &value) noexcept {
  WriteValue(JSValue{to_string(value)});
}

void JSValueTreeWriter::WriteObjectBegin() noexcept {
  //if (m_state == State::PropertyValue) {
  //  m_stack.emplace_back(m_state, std::move(m_object), std::move(m_propertyName));
  //} else if (m_state == State::Array) {
  //  m_stack.emplace_back(m_state, std::move(m_array));
  //} else if (m_state != State::Start) {
  //  VerifyElseCrash(false);
  //}

  //m_object = JSValueObject();
  //m_state = State::PropertyName;
}

void JSValueTreeWriter::WritePropertyName(const winrt::hstring &name) noexcept {
  if (m_state == State::PropertyName) {
    m_propertyName = winrt::to_string(name);
    m_state = State::PropertyValue;
  } else {
    VerifyElseCrash(false);
  }
}

void JSValueTreeWriter::WriteObjectEnd() noexcept {
  //if (m_state == State::PropertyName) {
  //  auto obj = JSValue(std::move(m_object));
  //  if (m_stack.empty()) {
  //    m_result = std::move(obj);
  //    m_state = State::Finish;
  //    return;
  //  } else {
  //    StackEntry &entry = m_stack.back();
  //    if (entry.State == State::PropertyValue) {
  //      entry.Object.emplace(std::move(entry.PropertyName), std::move(obj));
  //      m_object = std::move(entry.Object);
  //      m_state = State::PropertyName;
  //      m_stack.pop_back();
  //      return;
  //    } else if (entry.State == State::Array) {
  //      entry.Array.push_back(std::move(obj));
  //      m_array = std::move(entry.Array);
  //      m_state = State::Array;
  //      m_stack.pop_back();
  //      return;
  //    }
  //  }
  //}

  VerifyElseCrash(false);
}

void JSValueTreeWriter::WriteArrayBegin() noexcept {
  if (m_state == State::PropertyValue) {
    //m_stack.push_back(StackEntry{m_state, std::move(m_object), std::move(m_propertyName)});
  } else if (m_state == State::Array) {
    //m_stack.emplace_back(m_state, JSValueObject{}, std::move(m_array), std::string{});
  } else if (m_state != State::Start) {
    VerifyElseCrash(false);
  }

  m_array.clear();
  m_state = State::Array;
}

void JSValueTreeWriter::WriteArrayEnd() noexcept {
  //if (m_state == State::Array) {
  //  auto arr = JSValue(std::move(m_array));
  //  if (m_stack.empty()) {
  //    m_result = std::move(arr);
  //    m_state = State::Finish;
  //    return;
  //  } else {
  //    StackEntry &entry = m_stack.back();
  //    if (entry.State == State::PropertyValue) {
  //      entry.Object.emplace(std::move(entry.PropertyName), std::move(arr));
  //      m_object = std::move(entry.Object);
  //      m_state = State::PropertyName;
  //      m_stack.pop_back();
  //      return;
  //    } else if (entry.State == State::Array) {
  //      entry.Array.push_back(std::move(arr));
  //      m_array = std::move(entry.Array);
  //      m_state = State::Array;
  //      m_stack.pop_back();
  //      return;
  //    }
  //  }
  //}

  VerifyElseCrash(false);
}

void JSValueTreeWriter::WriteValue(JSValue &&/*value*/) noexcept {
  //if (m_state == State::PropertyValue) {
  //  m_object.emplace(std::move(m_propertyName), std::move(value));
  //  m_state = State::PropertyName;
  //} else if (m_state == State::Array) {
  //  m_array.push_back(std::move(value));
  //}
}

} // namespace winrt::Microsoft::ReactNative::Bridge

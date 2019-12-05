// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_JSVALUETREEWRITER
#define MICROSOFT_REACTNATIVE_JSVALUETREEWRITER

#include "JSValue.h"

namespace winrt::Microsoft::ReactNative::Bridge {

struct JSValueTreeWriter : implements<JSValueTreeWriter, IJSValueWriter> {
  JSValue TakeValue() noexcept;

 public: // IJSValueWriter
  void WriteNull() noexcept;
  void WriteBoolean(bool value) noexcept;
  void WriteInt64(int64_t value) noexcept;
  void WriteDouble(double value) noexcept;
  void WriteString(const winrt::hstring &value) noexcept;
  void WriteObjectBegin() noexcept;
  void WritePropertyName(const winrt::hstring &name) noexcept;
  void WriteObjectEnd() noexcept;
  void WriteArrayBegin() noexcept;
  void WriteArrayEnd() noexcept;

 private:
  enum struct State { Start, PropertyName, PropertyValue, Array, Finish };

  struct StackEntry {
    //StackEntry(State state, JSValueObject &&object, std::string &&propertyName) noexcept
    //    : State{state}, Object{std::move(object)}, PropertyName{std::move(propertyName)} {}

    //StackEntry(State state, JSValueArray &&array) noexcept : State{state}, Array(std::move(array)) {}

    State State;
    JSValueObject Object;
    JSValueArray Array;
    std::string PropertyName;
  };

 private:
  void WriteValue(JSValue &&value) noexcept;

 private:
  State m_state{State::Start};
  std::vector<StackEntry> m_stack;
  JSValueObject m_object;
  JSValueArray m_array;
  std::string m_propertyName;
  JSValue m_result;
};

} // namespace winrt::Microsoft::ReactNative::Bridge

#endif // MICROSOFT_REACTNATIVE_JSVALUETREEWRITER

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_JSVALUEWRITER
#define MICROSOFT_REACTNATIVE_JSVALUEWRITER

#include "winrt/Microsoft.ReactNative.Bridge.h"

namespace winrt::Microsoft::ReactNative::Bridge {

//==============================================================================
// IJSValueWriter extensions
//==============================================================================

// Forward declarations
template <class T, class TJSValueWriter, std::enable_if_t<std::is_same_v<TJSValueWriter, IJSValueWriter>, int> = 1>
void WriteValue(TJSValueWriter const &writer, T const &value) noexcept;

template <class T, class TJSValue, std::enable_if_t<std::is_same_v<TJSValue, JSValue>, int> = 1>
void WriteValue(/*out*/ TJSValue &jsValue, T const &value) noexcept;

void WriteValue(IJSValueWriter const &writer, std::string const &value) noexcept;
void WriteValue(IJSValueWriter const &writer, std::wstring const &value) noexcept;
void WriteValue(IJSValueWriter const &writer, bool value) noexcept;
void WriteValue(IJSValueWriter const &writer, int8_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, int16_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, int32_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, int64_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, uint8_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, uint16_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, uint32_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, uint64_t value) noexcept;
void WriteValue(IJSValueWriter const &writer, float value) noexcept;
void WriteValue(IJSValueWriter const &writer, double value) noexcept;
template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 1>
void WriteValue(IJSValueWriter const &writer, T const &value) noexcept;
template <class T>
void WriteValue(IJSValueWriter const &writer, std::optional<T> const &value) noexcept;
template <class T, class TCompare = std::less<>, class TAlloc = std::allocator<pair<const std::string, T>>>
void WriteValue(IJSValueWriter const &writer, std::map<std::string, T, TCompare, TAlloc> const &value) noexcept;
template <class T, class TCompare = std::less<>, class TAlloc = std::allocator<pair<const std::string, T>>>
void WriteValue(IJSValueWriter const &writer, std::map<std::wstring, T, TCompare, TAlloc> const &value) noexcept;
template <class T, class TAlloc = allocator<T>>
void WriteValue(IJSValueWriter const &writer, std::vector<T, TAlloc> const &value) noexcept;
template <class... Ts>
void WriteValue(IJSValueWriter const &writer, std::tuple<Ts...> const &value) noexcept;

void WriteValue(IJSValueWriter const &writer, JSValue const &value) noexcept;
void WriteValue(IJSValueWriter const &writer, JSValueObject const &value) noexcept;
void WriteValue(IJSValueWriter const &writer, JSValueArray const &value) noexcept;

template <class T, std::enable_if_t<!std::is_void_v<decltype(GetStructInfo(static_cast<T *>(nullptr)))>, int> = 1>
void WriteValue(IJSValueWriter const &writer, T const &value) noexcept;

template <class T>
bool WriteProperty(IJSValueWriter const &writer, std::string_view propertyName, T const &value) noexcept;
template <class T>
bool WriteProperty(IJSValueWriter const &writer, std::wstring_view propertyName, T const &value) noexcept;

template <class... TArgs>
void WriteArgs(IJSValueWriter const &writer, TArgs const &... args) noexcept;

IJSValueWriter MakeJSValueTreeWriter() noexcept;

//==============================================================================
// IJSValueWriter extensions implementation
//==============================================================================

// Try to call WriteValue for JSValue unless it is already called us with a TypeWrapper parameter.
template <class T, class TJSValueWriter, std::enable_if_t<std::is_same_v<TJSValueWriter, IJSValueWriter>, int>>
inline void WriteValue(TJSValueWriter const &writer, T const &value) noexcept {
  TypeWrapper<JSValue> wrappedJSValue;
  WriteValue(/*out*/ wrappedJSValue, value);
  wrappedJSValue.Value.WriteTo(writer);
}

// Try to call ReadValue for IJSValueWriter unless it is already called us with TypeWrapper parameter.
template <class T, class TJSValue, std::enable_if_t<std::is_same_v<TJSValue, JSValue>, int>>
inline void WriteValue(/*out*/ TJSValue &jsValue, T const &value) noexcept {
  TypeWrapper<IJSValueWriter> wrappedWriter = {MakeJSValueTreeWriter()};
  WriteValue(wrappedWriter, value);
  jsValue = std::move(wrappedWriter.Value);
}

inline void WriteValue(IJSValueWriter const &writer, std::string_view value) noexcept {
  writer.WriteString(to_hstring(value));
}

inline void WriteValue(IJSValueWriter const &writer, std::wstring_view value) noexcept {
  writer.WriteString(value);
}

inline void WriteValue(IJSValueWriter const &writer, bool value) noexcept {
  writer.WriteBoolean(value);
}

inline void WriteValue(IJSValueWriter const &writer, int8_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, int16_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, int32_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, int64_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, uint8_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, uint16_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, uint32_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, uint64_t value) noexcept {
  writer.WriteInt64(value);
}

inline void WriteValue(IJSValueWriter const &writer, float value) noexcept {
  writer.WriteDouble(value);
}

inline void WriteValue(IJSValueWriter const &writer, double value) noexcept {
  writer.WriteDouble(value);
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int>>
inline void WriteValue(IJSValueWriter const &writer, T const &value) noexcept {
  WriteValue(writer, static_cast<int32_t>(value));
}

template <class T>
inline void WriteValue(IJSValueWriter const &writer, std::optional<T> const &value) noexcept {
  if (value.has_value()) {
    WriteValue(writer, *value);
  } else {
    writer.WriteNull();
  }
}

template <class T, class TCompare, class TAlloc>
inline void WriteValue(IJSValueWriter const &writer, std::map<std::string, T, TCompare, TAlloc> const &value) noexcept {
  writer.WriteObjectBegin();
  for (const auto &entry : value) {
    WriteProperty(writer, entry.first, entry.second);
  }
  writer.WriteObjectEnd();
}

template <class T, class TCompare, class TAlloc>
inline void WriteValue(
    IJSValueWriter const &writer,
    std::map<std::wstring, T, TCompare, TAlloc> const &value) noexcept {
  writer.WriteObjectBegin();
  for (const auto &entry : value) {
    WriteProperty(writer, entry.first, entry.second);
  }
  writer.WriteObjectEnd();
}

template <class T, class TAlloc>
inline void WriteValue(IJSValueWriter const &writer, std::vector<T, TAlloc> const &value) noexcept {
  writer.WriteArrayBegin();
  for (const auto &item : value) {
    WriteValue(writer, item);
  }
  writer.WriteArrayEnd();
}

template <class T, size_t... I>
inline void WriteTuple(IJSValueWriter const &writer, T const &tuple, std::index_sequence<I...>) noexcept {
  WriteArgs(writer, std::get<I>(tuple)...);
}

template <class... Ts>
inline void WriteValue(IJSValueWriter const &writer, std::tuple<Ts...> const &value) noexcept {
  WriteTuple(writer, value, std::make_index_sequence<sizeof...(Ts)>{});
}

inline void WriteValue(IJSValueWriter const &writer, JSValue const &value) noexcept {
  value.WriteTo(writer);
}

inline void WriteValue(IJSValueWriter const &writer, JSValueObject const &value) noexcept {
  JSValue::WriteObjectTo(writer, value);
}

inline void WriteValue(IJSValueWriter const &writer, JSValueArray const &value) noexcept {
  JSValue::WriteArrayTo(writer, value);
}

template <class T, std::enable_if_t<!std::is_void_v<decltype(GetStructInfo(static_cast<T *>(nullptr)))>, int>>
inline void WriteValue(IJSValueWriter const &writer, T const &value) noexcept {
  writer.WriteObjectBegin();
  for (const auto &fieldEntry : StructInfo<T>::FieldMap) {
    writer.WritePropertyName(fieldEntry.first);
    fieldEntry.second.WriteField(writer, &value);
  }
  writer.WriteObjectEnd();
}

template <class T>
inline bool WriteProperty(IJSValueWriter const &writer, std::string_view propertyName, T const &value) noexcept {
  writer.WritePropertyName(propertyName);
  WriteValue(writer, value);
}

template <class T>
inline bool WriteProperty(IJSValueWriter const &writer, std::wstring_view propertyName, T const &value) noexcept {
  writer.WritePropertyName(propertyName);
  WriteValue(writer, value);
}

template <class... TArgs>
inline void WriteArgs(IJSValueWriter const &writer, TArgs const &... args) noexcept {
  writer.WriteArrayBegin();
  if constexpr (sizeof...(args) > 0) {
    // To write variadic template arguments in natural order we must use them in
    // an initializer list.
    [[maybe_unused]] int dummy[] = {(WriteValue(writer, args), 0)...};
  }
  writer.WriteArrayEnd();
}

} // namespace winrt::Microsoft::ReactNative::Bridge

#endif // MICROSOFT_REACTNATIVE_JSVALUEWRITER

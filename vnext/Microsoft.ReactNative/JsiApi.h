// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "JsiPointer.g.h"
#include "winrt/Microsoft.ReactNative.h"

namespace facebook::jsi {
class Runtime;
class Pointer;
} // namespace facebook::jsi

namespace winrt::Microsoft::ReactNative::implementation {

struct JsiRuntime : implements<JsiRuntime, IJsiRuntime> {
  JsiRuntime(facebook::jsi::Runtime &runtime) noexcept;

 public: // IJsiRuntime
  Microsoft::ReactNative::JsiValueData EvaluateJavaScript(
      Microsoft::ReactNative::IJsiBuffer const &buffer,
      hstring const &sourceUrl,
      Microsoft::ReactNative::JsiPointer &ptrResult);
  Windows::Foundation::IInspectable PrepareJavaScript(
      Microsoft::ReactNative::IJsiBuffer const &buffer,
      hstring const &sourceUrl);
  Microsoft::ReactNative::JsiValueData EvaluatePreparedJavaScript(Windows::Foundation::IInspectable const &js);
  Microsoft::ReactNative::JsiPointer Global() noexcept;
  hstring Description();
  bool IsInspectable();
  Microsoft::ReactNative::JsiPointer CloneSymbol(Microsoft::ReactNative::JsiPointer const &symbol);
  Microsoft::ReactNative::JsiPointer CloneString(Microsoft::ReactNative::JsiPointer const &str);
  Microsoft::ReactNative::JsiPointer CloneObject(Microsoft::ReactNative::JsiPointer const &obj);
  Microsoft::ReactNative::JsiPointer ClonePropertyNameId(Microsoft::ReactNative::JsiPointer const &propertyNameId);
  Microsoft::ReactNative::JsiPointer CreatePropertyNameIdFromAscii(array_view<uint8_t const> ascii);
  Microsoft::ReactNative::JsiPointer CreatePropertyNameIdFromUtf8(array_view<uint8_t const> utf8);
  Microsoft::ReactNative::JsiPointer CreatePropertyNameIdFromString(Microsoft::ReactNative::JsiPointer const &str);
  void PropertyNameIdToUtf8(
      Microsoft::ReactNative::JsiPointer const &propertyNameId,
      Microsoft::ReactNative::JsiDataHandler const &utf8Handler);
  bool ComparePropertyNameIds(
      Microsoft::ReactNative::JsiPointer const &left,
      Microsoft::ReactNative::JsiPointer const &right);
  void SymbolToUtf8(
      Microsoft::ReactNative::JsiPointer const &symbol,
      Microsoft::ReactNative::JsiDataHandler const &utf8Handler);
  Microsoft::ReactNative::JsiPointer CreateStringFromAscii(array_view<uint8_t const> ascii);
  Microsoft::ReactNative::JsiPointer CreateStringFromUtf8(array_view<uint8_t const> utf8);
  void StringToUtf8(
      Microsoft::ReactNative::JsiPointer const &str,
      Microsoft::ReactNative::JsiDataHandler const &utf8Handler);
  Microsoft::ReactNative::JsiValueData CreateValueFromJsonUtf8(
      array_view<uint8_t const> json,
      Microsoft::ReactNative::JsiPointer &ptrResult);
  Microsoft::ReactNative::JsiPointer CreateObject();
  Microsoft::ReactNative::JsiPointer CreateObjectWithHostObject(
      Microsoft::ReactNative::IJsiHostObject const &hostObject);
  Microsoft::ReactNative::IJsiHostObject GetHostObject(Microsoft::ReactNative::JsiPointer const &obj);
  Microsoft::ReactNative::JsiHostFunction GetHostFunction(Microsoft::ReactNative::JsiPointer const &func);
  Microsoft::ReactNative::JsiValueData GetProperty(
      Microsoft::ReactNative::JsiPointer const &obj,
      Microsoft::ReactNative::JsiPointer const &propertyNameId,
      Microsoft::ReactNative::JsiPointer &ptrResult);
  Microsoft::ReactNative::JsiValueData GetPropertyWithString(
      Microsoft::ReactNative::JsiPointer const &obj,
      Microsoft::ReactNative::JsiPointer const &nameStr,
      Microsoft::ReactNative::JsiPointer &ptrResult);
  bool HasProperty(
      Microsoft::ReactNative::JsiPointer const &obj,
      Microsoft::ReactNative::JsiPointer const &propertyNameId);
  bool HasPropertyWithString(
      Microsoft::ReactNative::JsiPointer const &obj,
      Microsoft::ReactNative::JsiPointer const &nameStr);
  void SetProperty(
      Microsoft::ReactNative::JsiPointer const &obj,
      Microsoft::ReactNative::JsiPointer const &propertyNameId,
      Microsoft::ReactNative::JsiValueData const &value,
      Microsoft::ReactNative::JsiPointer const &ptrValue);
  void SetPropertyWithString(
      Microsoft::ReactNative::JsiPointer const &obj,
      Microsoft::ReactNative::JsiPointer const &nameStr,
      Microsoft::ReactNative::JsiValueData const &value,
      Microsoft::ReactNative::JsiPointer const &ptrValue);
  bool IsArray(Microsoft::ReactNative::JsiPointer const &obj);
  bool IsArrayBuffer(Microsoft::ReactNative::JsiPointer const &obj);
  bool IsFunction(Microsoft::ReactNative::JsiPointer const &obj);
  bool IsHostObject(Microsoft::ReactNative::JsiPointer const &obj);
  bool IsHostFunction(Microsoft::ReactNative::JsiPointer const &obj);
  Microsoft::ReactNative::JsiPointer GetPropertyNameArray(Microsoft::ReactNative::JsiPointer const &obj);
  Microsoft::ReactNative::JsiPointer CreateWeakObject(Microsoft::ReactNative::JsiPointer const &obj);
  Microsoft::ReactNative::JsiValueData LockWeakObject(
      Microsoft::ReactNative::JsiPointer const &weakObject,
      Microsoft::ReactNative::JsiPointer &obj);
  Microsoft::ReactNative::JsiPointer CreateArray(uint32_t size);
  uint32_t GetArraySize(Microsoft::ReactNative::JsiPointer const &arr);
  uint32_t GetArrayBufferSize(Microsoft::ReactNative::JsiPointer const &arrayBuffer);
  void ArrayBufferToUtf8(
      Microsoft::ReactNative::JsiPointer const &arrayBuffer,
      Microsoft::ReactNative::JsiDataHandler const &utf8Handler);
  Microsoft::ReactNative::JsiValueData GetValueAtIndex(
      Microsoft::ReactNative::JsiPointer const &arr,
      uint32_t index,
      Microsoft::ReactNative::JsiPointer &ptrResult);
  void SetValueAtIndex(
      Microsoft::ReactNative::JsiPointer const &arr,
      uint32_t index,
      Microsoft::ReactNative::JsiValueData const &value,
      Microsoft::ReactNative::JsiPointer const &ptrValue);
  Microsoft::ReactNative::JsiPointer CreateFunctionFromHostFunction(
      Microsoft::ReactNative::JsiPointer const &propNameId,
      uint32_t paramCount,
      Microsoft::ReactNative::JsiHostFunction const &hostFunc);
  Microsoft::ReactNative::JsiValueData Call(
      Microsoft::ReactNative::JsiPointer const &func,
      Microsoft::ReactNative::JsiValueData const &thisData,
      Microsoft::ReactNative::JsiPointer const &thisPtr,
      array_view<Microsoft::ReactNative::JsiValueData const> args,
      array_view<Microsoft::ReactNative::JsiPointer const> ptrArgs,
      Microsoft::ReactNative::JsiPointer &ptrResult);
  Microsoft::ReactNative::JsiValueData CallAsConstructor(
      Microsoft::ReactNative::JsiPointer const &func,
      array_view<Microsoft::ReactNative::JsiValueData const> args,
      array_view<Microsoft::ReactNative::JsiPointer const> ptrArgs,
      Microsoft::ReactNative::JsiPointer &ptrResult);
  uint64_t PushScope();
  void PopScope(uint64_t scopeState);
  bool SymbolStrictEquals(
      Microsoft::ReactNative::JsiPointer const &left,
      Microsoft::ReactNative::JsiPointer const &right);
  bool StringStrictEquals(
      Microsoft::ReactNative::JsiPointer const &left,
      Microsoft::ReactNative::JsiPointer const &right);
  bool ObjectStrictEquals(
      Microsoft::ReactNative::JsiPointer const &left,
      Microsoft::ReactNative::JsiPointer const &right);
  bool InstanceOf(Microsoft::ReactNative::JsiPointer const &obj, Microsoft::ReactNative::JsiPointer const &constructor);

 private:
  facebook::jsi::Runtime &m_runtime;
};

} // namespace winrt::Microsoft::ReactNative::implementation

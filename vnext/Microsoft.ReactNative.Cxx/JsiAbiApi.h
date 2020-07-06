// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_JSIABIAPI
#define MICROSOFT_REACTNATIVE_JSIABIAPI

#include "Crash.h"
#include "jsi/jsi.h"
#include "winrt/Microsoft.ReactNative.h"
#include "winrt/base.h"

namespace winrt::Microsoft::ReactNative {

struct JsiBuffer : implements<JsiBuffer, IJsiBuffer> {
  JsiBuffer(std::shared_ptr<const facebook::jsi::Buffer> const &buffer) : m_buffer{buffer} {}
  uint32_t Size() {
    return static_cast<uint32_t>(m_buffer->size());
  }

  void GetData(JsiDataHandler const &handler) {
    handler(winrt::array_view<uint8_t const>{m_buffer->data(), m_buffer->data() + m_buffer->size()});
  }

 private:
  std::shared_ptr<const facebook::jsi::Buffer> m_buffer;
};

struct JsiAbiRuntime : facebook::jsi::Runtime {
  PointerValue *ToPointerValue(JsiPointer const &pointer) {
    return static_cast<PointerValue *>(reinterpret_cast<void *>(pointer.QueryPointerValue()));
  }

  facebook::jsi::Object ToObject(JsiPointer const &pointer) {
    return make<facebook::jsi::Object>(ToPointerValue(pointer));
  }

  facebook::jsi::Value evaluateJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      const std::string &sourceURL) override {
    JsiPointer resultPtr{nullptr};
    JsiValueData data = m_runtime.EvaluateJavaScript(winrt::make<JsiBuffer>(buffer), to_hstring(sourceURL), resultPtr);
  }
#if 0
  std::shared_ptr<const PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const Buffer> &buffer,
      std::string sourceURL) override{}
  Value evaluatePreparedJavaScript(const std::shared_ptr<const PreparedJavaScript> &js) override{}
#endif

  facebook::jsi::Object global() override {
    return ToObject(m_runtime.Global());
  }

  std::string description() override {
    return to_string(m_runtime.Description());
  }

  bool isInspectable() override {
    return m_runtime.IsInspectable();
  }

  facebook::jsi::Instrumentation &instrumentation() {
    VerifyElseCrash(false);
  }
  #if 0
  PointerValue *cloneSymbol(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime->CloneSymbol(pv));
  }

  PointerValue *cloneString(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime->CloneString(pv));
  }

  PointerValue *cloneObject(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime->CloneObject(pv));
  }

  PointerValue *clonePropNameID(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime->ClonePropertyNameId(pv));
  }

  facebook::jsi::PropNameID createPropNameIDFromAscii(const char *str, size_t length) override {
    return make<facebook::jsi::PropNameID>(m_runtime->CreatePropertyNameIdFromAscii(length, str));
  }

  facebook::jsi::PropNameID createPropNameIDFromUtf8(const uint8_t *utf8, size_t length) override {
    return make<facebook::jsi::PropNameID>(m_runtime->CreatePropertyNameIdFromUtf8(length, utf8));
  }

  facebook::jsi::PropNameID createPropNameIDFromString(const facebook::jsi::String &str) override {
    return make<facebook::jsi::PropNameID>(m_runtime->CreatePropertyNameIdFromString(getPointerValue(str)));
  }

  std::string utf8(const facebook::jsi::PropNameID &propertyNameId) override {
    std::string result;
    m_runtime->PropertyNameIdToUtf8(
        [&result](size_t length, const char *utf8) noexcept { result.assign(utf8, length); });
    return result;
  }
#endif
#if 0
  bool compare(const PropNameID &, const PropNameID &) override {}
  std::string symbolToString(const Symbol &) override{}
  String createStringFromAscii(const char *str, size_t length) override{}
  String createStringFromUtf8(const uint8_t *utf8, size_t length) override{}
  std::string utf8(const String &) override{}
  Value createValueFromJsonUtf8(const uint8_t *json, size_t length){}
  Object createObject() override{}
  Object createObject(std::shared_ptr<HostObject> ho) override{}
  std::shared_ptr<HostObject> getHostObject(const jsi::Object &) override{}
  HostFunctionType &getHostFunction(const jsi::Function &) override{}
  Value getProperty(const Object &, const PropNameID &name) override{}
  Value getProperty(const Object &, const String &name) override{}
  bool hasProperty(const Object &, const PropNameID &name) override{}
  bool hasProperty(const Object &, const String &name) override{}
  void setPropertyValue(Object &, const PropNameID &name, const Value &value) override{}
  void setPropertyValue(Object &, const String &name, const Value &value) override{}
  bool isArray(const Object &) const override{}
  bool isArrayBuffer(const Object &) const override{}
  bool isFunction(const Object &) const override{}
  bool isHostObject(const jsi::Object &) const override{}
  bool isHostFunction(const jsi::Function &) const override{}
  Array getPropertyNames(const Object &) override{}
  WeakObject createWeakObject(const Object &) override{}
  Value lockWeakObject(const WeakObject &) override{}
  Array createArray(size_t length) override{}
  size_t size(const Array &) override{}
  size_t size(const ArrayBuffer &) override{}
  uint8_t *data(const ArrayBuffer &) override{}
  Value getValueAtIndex(const Array &, size_t i) override{}
  void setValueAtIndexImpl(Array &, size_t i, const Value &value) override{}
  Function
  createFunctionFromHostFunction(const PropNameID &name, unsigned int paramCount, HostFunctionType func) override{}
  Value call(const Function &, const Value &jsThis, const Value *args, size_t count) override{}
  Value callAsConstructor(const Function &, const Value *args, size_t count) override{}
  ScopeState *pushScope(){}
  void popScope(ScopeState *){}
  bool strictEquals(const Symbol &a, const Symbol &b) const override{}
  bool strictEquals(const String &a, const String &b) const override{}
  bool strictEquals(const Object &a, const Object &b) const override{}
  bool instanceOf(const Object &o, const Function &f) override{}
#endif
 private:
  IJsiRuntime m_runtime;
};

} // namespace winrt::Microsoft::ReactNative

#endif // MICROSOFT_REACTNATIVE_JSIABIAPI

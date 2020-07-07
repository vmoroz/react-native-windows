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

using JsiPointerHandle = uint64_t;

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

struct JsiPreparedJavaScriptWrapper : facebook::jsi::PreparedJavaScript {
  JsiPreparedJavaScriptWrapper(JsiPreparedJavaScript const &preparedScript) : m_preparedScript{preparedScript} {}

  JsiPreparedJavaScript const &Get() const noexcept {
    return m_preparedScript;
  }

 private:
  JsiPreparedJavaScript m_preparedScript;
};

struct JsiAbiRuntime;

static JsiValueData &&ToJsiValueData(facebook::jsi::Value &&value) noexcept {
  return reinterpret_cast<JsiValueData &&>(value);
}

static JsiValueData const &ToJsiValueData(facebook::jsi::Value const &value) noexcept {
  return reinterpret_cast<JsiValueData const &>(value);
}

struct JsiHostObject : implements<JsiHostObject, IJsiHostObject> {
  JsiHostObject(std::shared_ptr<facebook::jsi::HostObject> &&hostObject) : m_hostObject(std::move(hostObject)) {}

  JsiValueData GetProperty(
      IJsiRuntime const & /*runtime*/,
      JsiPointerHandle name) {
    // TODO: Associate Runtime with ABI runtime
    facebook::jsi::Runtime *rt{nullptr};
    return ToJsiValueData(m_hostObject->get(*rt, *reinterpret_cast<facebook::jsi::PropNameID *>(name)));
  }

  void SetProperty(IJsiRuntime const & /*runtime*/, JsiPointerHandle name, JsiValueData const &value) {
    // TODO: Associate Runtime with ABI runtime
    facebook::jsi::Runtime *rt{nullptr};
    m_hostObject->set(
        *rt,
        *reinterpret_cast<facebook::jsi::PropNameID *>(name),
        *reinterpret_cast<facebook::jsi::Value const *>(&value));
  }

  Windows::Foundation::Collections::IVectorView<JsiPointerHandle> GetPropertyNames(
      IJsiRuntime const & /*runtime*/) noexcept {
    // TODO: Associate Runtime with ABI runtime
    facebook::jsi::Runtime *rt{nullptr};
    auto names = m_hostObject->getPropertyNames(*rt);
    std::vector<JsiPointerHandle> result;
    result.reserve(names.size());
    for (auto &name : names) {
      result.push_back(*reinterpret_cast<JsiPointerHandle *>(&name));
    }

    return winrt::single_threaded_vector<JsiPointerHandle>(std::move(result)).GetView();
  }

  std::shared_ptr<facebook::jsi::HostObject> const &Get() const noexcept {
    return m_hostObject;
  }

 private:
  std::shared_ptr<facebook::jsi::HostObject> m_hostObject;
};

struct JsiAbiRuntime : facebook::jsi::Runtime {
  JsiAbiRuntime(IJsiRuntime const &runtime) : m_runtime{runtime} {}

  static facebook::jsi::String *AsString(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::String *>(pointer);
  }

  static facebook::jsi::PropNameID *AsPropNameID(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::PropNameID *>(pointer);
  }

  static facebook::jsi::Symbol *AsSymbol(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::Symbol *>(pointer);
  }

  static facebook::jsi::Object *AsObject(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::Object *>(pointer);
  }

  static facebook::jsi::WeakObject *AsWeakObject(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::WeakObject *>(pointer);
  }

  static facebook::jsi::Function *AsFunction(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::Function *>(pointer);
  }

  static facebook::jsi::Array *AsArray(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::Array *>(pointer);
  }

  static facebook::jsi::ArrayBuffer *AsArrayBuffer(JsiPointerHandle pointer) noexcept {
    return reinterpret_cast<facebook::jsi::ArrayBuffer *>(pointer);
  }

  static JsiPointerHandle ToJsiPointerHandle(facebook::jsi::Runtime::PointerValue const *pointerValue) noexcept {
    return reinterpret_cast<JsiPointerHandle>(pointerValue);
  }

  static JsiPointerHandle ToJsiPointerHandle(facebook::jsi::Pointer const &pointer) noexcept {
    return ToJsiPointerHandle(facebook::jsi::Runtime::getPointerValue(pointer));
  }

  static facebook::jsi::Runtime::PointerValue *ToPointerValue(JsiPointerHandle pointerHandle) {
    return reinterpret_cast<facebook::jsi::Runtime::PointerValue *>(pointerHandle);
  }

  static facebook::jsi::Value &&ToValue(JsiValueData &&valueData) noexcept {
    return reinterpret_cast<facebook::jsi::Value &&>(valueData);
  }

  static facebook::jsi::Value const &ToValue(JsiValueData const &valueData) noexcept {
    return reinterpret_cast<facebook::jsi::Value const &>(valueData);
  }

  facebook::jsi::Value evaluateJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      const std::string &sourceURL) override {
    return ToValue(m_runtime.EvaluateJavaScript(winrt::make<JsiBuffer>(buffer), to_hstring(sourceURL)));
  }

  std::shared_ptr<const facebook::jsi::PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      std::string sourceURL) override {
    return std::make_shared<JsiPreparedJavaScriptWrapper>(
        m_runtime.PrepareJavaScript(winrt::make<JsiBuffer>(std::move(buffer)), to_hstring(sourceURL)));
  }

  facebook::jsi::Value evaluatePreparedJavaScript(
      const std::shared_ptr<const facebook::jsi::PreparedJavaScript> &js) override {
    return ToValue(
        m_runtime.EvaluatePreparedJavaScript(std::static_pointer_cast<JsiPreparedJavaScriptWrapper const>(js)->Get()));
  }

  facebook::jsi::Object global() override {
    return std::move(*AsObject(m_runtime.Global()));
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

  PointerValue *cloneSymbol(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime.CloneSymbol(ToJsiPointerHandle(pv)));
  }

  PointerValue *cloneString(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime.CloneString(ToJsiPointerHandle(pv)));
  }

  PointerValue *cloneObject(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime.CloneObject(ToJsiPointerHandle(pv)));
  }

  PointerValue *clonePropNameID(const Runtime::PointerValue *pv) override {
    return ToPointerValue(m_runtime.ClonePropertyNameId(ToJsiPointerHandle(pv)));
  }

  facebook::jsi::PropNameID createPropNameIDFromAscii(const char *str, size_t length) override {
    auto data = reinterpret_cast<uint8_t const *>(str);
    return std::move(*AsPropNameID(m_runtime.CreatePropertyNameIdFromAscii({data, data + length})));
  }

  facebook::jsi::PropNameID createPropNameIDFromUtf8(const uint8_t *utf8, size_t length) override {
    return std::move(*AsPropNameID(m_runtime.CreatePropertyNameIdFromUtf8({utf8, utf8 + length})));
  }

  facebook::jsi::PropNameID createPropNameIDFromString(const facebook::jsi::String &str) override {
    return std::move(*AsPropNameID(m_runtime.CreatePropertyNameIdFromString(ToJsiPointerHandle(str))));
  }

  std::string utf8(const facebook::jsi::PropNameID &propertyNameId) override {
    std::string result;
    m_runtime.PropertyNameIdToUtf8(ToJsiPointerHandle(propertyNameId), [&result](array_view<uint8_t const> utf8) {
      result.assign(reinterpret_cast<char const *>(utf8.data()), utf8.size());
    });
    return result;
  }

  bool compare(const facebook::jsi::PropNameID &left, const facebook::jsi::PropNameID &right) override {
    return m_runtime.ComparePropertyNameIds(ToJsiPointerHandle(left), ToJsiPointerHandle(right));
  }

  std::string symbolToString(const facebook::jsi::Symbol &symbol) override {
    std::string result;
    m_runtime.SymbolToUtf8(ToJsiPointerHandle(symbol), [&result](array_view<uint8_t const> utf8) {
      result.assign(reinterpret_cast<char const *>(utf8.data()), utf8.size());
    });
    return result;
  }

  facebook::jsi::String createStringFromAscii(const char *str, size_t length) override {
    auto ascii = reinterpret_cast<uint8_t const *>(str);
    return std::move(*AsString(m_runtime.CreateStringFromAscii({ascii, ascii + length})));
  }

  facebook::jsi::String createStringFromUtf8(const uint8_t *utf8, size_t length) override {
    return std::move(*AsString(m_runtime.CreateStringFromAscii({utf8, utf8 + length})));
  }

  std::string utf8(const facebook::jsi::String &str) override {
    std::string result;
    m_runtime.StringToUtf8(ToJsiPointerHandle(str), [&result](array_view<uint8_t const> utf8) {
      result.assign(reinterpret_cast<char const *>(utf8.data()), utf8.size());
    });
    return result;
  }

  facebook::jsi::Value createValueFromJsonUtf8(const uint8_t *json, size_t length) {
    return ToValue(m_runtime.CreateValueFromJsonUtf8({json, json + length}));
  }

  facebook::jsi::Object createObject() override {
    return std::move(*AsObject(m_runtime.CreateObject()));
  }

  facebook::jsi::Object createObject(std::shared_ptr<facebook::jsi::HostObject> ho) override {
    return std::move(*AsObject(m_runtime.CreateObjectWithHostObject(winrt::make<JsiHostObject>(std::move(ho)))));
  }

  std::shared_ptr<facebook::jsi::HostObject> getHostObject(const facebook::jsi::Object &obj) override {
    auto hostObject = m_runtime.GetHostObject(ToJsiPointerHandle(obj));
    return get_self<JsiHostObject>(hostObject)->Get();
  }

  facebook::jsi::HostFunctionType &getHostFunction(const facebook::jsi::Function &) override {
    // TODO: implement mapping
    static auto result = std::function<facebook::jsi::Value(
        facebook::jsi::Runtime & rt,
        const facebook::jsi::Value &thisVal,
        const facebook::jsi::Value *args,
        size_t count)>();
    return result;
  }

  facebook::jsi::Value getProperty(const facebook::jsi::Object &obj, const facebook::jsi::PropNameID &name) override {
    return ToValue(m_runtime.GetProperty(ToJsiPointerHandle(obj), ToJsiPointerHandle(name)));
  }

  facebook::jsi::Value getProperty(const facebook::jsi::Object &obj, const facebook::jsi::String &name) override {
    return ToValue(m_runtime.GetPropertyWithString(ToJsiPointerHandle(obj), ToJsiPointerHandle(name)));
  }

  bool hasProperty(const facebook::jsi::Object &obj, const facebook::jsi::PropNameID &name) override {
    return m_runtime.HasProperty(ToJsiPointerHandle(obj), ToJsiPointerHandle(name));
  }

  bool hasProperty(const facebook::jsi::Object &obj, const facebook::jsi::String &name) override {
    return m_runtime.HasPropertyWithString(ToJsiPointerHandle(obj), ToJsiPointerHandle(name));
  }

  void setPropertyValue(
      facebook::jsi::Object &obj,
      const facebook::jsi::PropNameID &name,
      const facebook::jsi::Value &value) override {
    m_runtime.SetProperty(ToJsiPointerHandle(obj), ToJsiPointerHandle(name), ToJsiValueData(value));
  }

  void setPropertyValue(
      facebook::jsi::Object &obj,
      const facebook::jsi::String &name,
      const facebook::jsi::Value &value) override {
    m_runtime.SetPropertyWithString(ToJsiPointerHandle(obj), ToJsiPointerHandle(name), ToJsiValueData(value));
  }

  bool isArray(const facebook::jsi::Object &obj) const override {
    return m_runtime.IsArray(ToJsiPointerHandle(obj));
  }

  bool isArrayBuffer(const facebook::jsi::Object &obj) const override {
    return m_runtime.IsArrayBuffer(ToJsiPointerHandle(obj));
  }

  bool isFunction(const facebook::jsi::Object &obj) const override {
    return m_runtime.IsFunction(ToJsiPointerHandle(obj));
  }

  bool isHostObject(const facebook::jsi::Object &obj) const override {
    return m_runtime.IsHostObject(ToJsiPointerHandle(obj));
  }

  bool isHostFunction(const facebook::jsi::Function &func) const override {
    return m_runtime.IsHostFunction(ToJsiPointerHandle(func));
  }

  facebook::jsi::Array getPropertyNames(const facebook::jsi::Object &obj) override {
    return std::move(*AsArray(m_runtime.GetPropertyNameArray(ToJsiPointerHandle(obj))));
  }

  facebook::jsi::WeakObject createWeakObject(const facebook::jsi::Object &obj) override {
    return std::move(*AsWeakObject(m_runtime.CreateWeakObject(ToJsiPointerHandle(obj))));
  }

  facebook::jsi::Value lockWeakObject(const facebook::jsi::WeakObject &weakObj) override {
    return ToValue(m_runtime.LockWeakObject(ToJsiPointerHandle(weakObj)));
  }

  facebook::jsi::Array createArray(size_t length) override {
    return std::move(*AsArray(m_runtime.CreateArray(static_cast<uint32_t>(length))));
  }

  size_t size(const facebook::jsi::Array &arr) override {
    return m_runtime.GetArraySize(ToJsiPointerHandle(arr));
  }

  size_t size(const facebook::jsi::ArrayBuffer &arrayBuffer) override {
    return m_runtime.GetArrayBufferSize(ToJsiPointerHandle(arrayBuffer));
  }

  uint8_t *data(const facebook::jsi::ArrayBuffer &arrayBuffer) override {
    uint8_t *result{};
    m_runtime.ArrayBufferToUtf8(ToJsiPointerHandle(arrayBuffer), [&result](array_view<uint8_t const> dataView) {
      result = const_cast<uint8_t *>(dataView.data());
    });
    return result;
  }

  facebook::jsi::Value getValueAtIndex(const facebook::jsi::Array &arr, size_t i) override {
    return ToValue(m_runtime.GetValueAtIndex(ToJsiPointerHandle(arr), static_cast<uint32_t>(i)));
  }

  void setValueAtIndexImpl(facebook::jsi::Array &arr, size_t i, const facebook::jsi::Value &value) override {
    m_runtime.SetValueAtIndex(ToJsiPointerHandle(arr), static_cast<uint32_t>(i), ToJsiValueData(value));
  }

  facebook::jsi::Function createFunctionFromHostFunction(
      const facebook::jsi::PropNameID &name,
      unsigned int paramCount,
      facebook::jsi::HostFunctionType func) override {
    // TODO: implement mapping
    return std::move(*AsFunction(m_runtime.CreateFunctionFromHostFunction(
        ToJsiPointerHandle(name),
        paramCount,
        [func = std::move(func)](
            IJsiRuntime const &runtime, JsiValueData const &thisValue, array_view<JsiValueData const> args)
            -> JsiValueData {
          auto rt{JsiAbiRuntime{runtime}};
          return ToJsiValueData(
              func(rt, ToValue(thisValue), reinterpret_cast<facebook::jsi::Value const *>(args.data()), args.size()));
        })));
  }

  facebook::jsi::Value call(
      const facebook::jsi::Function &func,
      const facebook::jsi::Value &jsThis,
      const facebook::jsi::Value *args,
      size_t count) override {
    JsiValueData const *argsData = reinterpret_cast<JsiValueData const *>(args);
    return ToValue(m_runtime.Call(ToJsiPointerHandle(func), ToJsiValueData(jsThis), {argsData, argsData + count}));
  }

  facebook::jsi::Value
  callAsConstructor(const facebook::jsi::Function &func, const facebook::jsi::Value *args, size_t count) override {
    JsiValueData const *argsData = reinterpret_cast<JsiValueData const *>(args);
    return ToValue(m_runtime.CallAsConstructor(ToJsiPointerHandle(func), {argsData, argsData + count}));
  }

  ScopeState *pushScope() {
    return reinterpret_cast<ScopeState *>(m_runtime.PushScope());
  }

  void popScope(ScopeState *scope) {
    m_runtime.PopScope(reinterpret_cast<uint64_t>(scope));
  }

  bool strictEquals(const facebook::jsi::Symbol &a, const facebook::jsi::Symbol &b) const override {
    return m_runtime.SymbolStrictEquals(ToJsiPointerHandle(a), ToJsiPointerHandle(b));
  }

  bool strictEquals(const facebook::jsi::String &a, const facebook::jsi::String &b) const override {
    return m_runtime.StringStrictEquals(ToJsiPointerHandle(a), ToJsiPointerHandle(b));
  }

  bool strictEquals(const facebook::jsi::Object &a, const facebook::jsi::Object &b) const override {
    return m_runtime.ObjectStrictEquals(ToJsiPointerHandle(a), ToJsiPointerHandle(b));
  }

  bool instanceOf(const facebook::jsi::Object &o, const facebook::jsi::Function &f) override {
    return m_runtime.InstanceOf(ToJsiPointerHandle(o), ToJsiPointerHandle(f));
  }

 private:
  IJsiRuntime m_runtime;
};

} // namespace winrt::Microsoft::ReactNative

#endif // MICROSOFT_REACTNATIVE_JSIABIAPI

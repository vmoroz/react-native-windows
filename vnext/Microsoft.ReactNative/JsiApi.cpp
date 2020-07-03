// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "JsiApi.h"
#include "JsiPointer.g.cpp"
#include <crash/verifyElseCrash.h>

// facebook::jsi::Runtime hides all methods that we need to call.
// We "open" them up here by redeclaring the private and protected keywords.
#define private public
#define protected public
#include "jsi/jsi.h"

namespace winrt::Microsoft::ReactNative::implementation {

// Wraps up the facebook::jsi::Pointer
struct JsiPointer : JsiPointerT<JsiPointer> {
  JsiPointer() = default;
  JsiPointer(facebook::jsi::Pointer &&pointer) noexcept;
  JsiPointer(facebook::jsi::Runtime::PointerValue *ptr) noexcept;

 private:
  facebook::jsi::Pointer m_pointer{nullptr};
};

// A special JsiPointer holder that avoids deletion of inner facebook::jsi::Pointer
struct JsiPointerRef {
  JsiPointerRef(Microsoft::ReactNative::JsiPointer &&jsiPointer);
  ~JsiPointerRef();

 private:
  Microsoft::ReactNative::JsiPointer m_jsiPointer;
};

// Wraps up the IJsiHostObject
struct HostObjectWrapper : facebook::jsi::HostObject {
  HostObjectWrapper(Microsoft::ReactNative::IJsiHostObject const &hostObject) noexcept;

  facebook::jsi::Value get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &name) override;
  void set(facebook::jsi::Runtime &, const facebook::jsi::PropNameID &name, const facebook::jsi::Value &value) override;
  std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime &runtime) noexcept override;

 private:
  Microsoft::ReactNative::IJsiHostObject m_hostObject;
};

struct JsiBufferWrapper : facebook::jsi::Buffer {
  JsiBufferWrapper(array_view<const uint8_t> bytes) : m_bytes{bytes} {}

  size_t size() const override {
    return m_bytes.size();
  }

  const uint8_t *data() const override {
    return m_bytes.data();
  }

 private:
  array_view<const uint8_t> m_bytes;
};

//===========================================================================
// Helper methods
//===========================================================================

static facebook::jsi::Runtime::PointerValue *GetPointerValue(
    Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return get_self<JsiPointer>(pointer)->m_pointer.ptr_;
}

static facebook::jsi::String const &AsString(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::String const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static facebook::jsi::PropNameID const &AsPropNameID(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::PropNameID const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static facebook::jsi::Symbol const &AsSymbol(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::Symbol const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static facebook::jsi::Object const &AsObject(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::Object const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static facebook::jsi::WeakObject const &AsWeakObject(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::WeakObject const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static facebook::jsi::Function const &AsFunction(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::Function const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static facebook::jsi::Array const &AsArray(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::Array const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static facebook::jsi::ArrayBuffer const &AsArrayBuffer(Microsoft::ReactNative::JsiPointer const &pointer) noexcept {
  return reinterpret_cast<facebook::jsi::ArrayBuffer const &>(get_self<JsiPointer>(pointer)->m_pointer);
}

static Microsoft::ReactNative::JsiPointer MakeJsiPointer(facebook::jsi::Runtime::PointerValue *pointerValue) noexcept {
  return make<JsiPointer>(pointerValue);
}

static Microsoft::ReactNative::JsiPointer MakeJsiPointer(facebook::jsi::Pointer &&pointer) noexcept {
  return make<JsiPointer>(std::exchange(pointer.ptr_, nullptr));
}

static JsiPointerRef MakeJsiPointerRef(facebook::jsi::Pointer const &pointer) noexcept {
  return JsiPointerRef{Microsoft::ReactNative::JsiPointer{make<JsiPointer>(pointer.ptr_)}};
}

static facebook::jsi::Value MakePointerValue(
    facebook::jsi::Value::ValueKind valueKind,
    Microsoft::ReactNative::JsiPointer &&pointer) {
  facebook::jsi::Value value{valueKind};
  new (&value.data_.pointer) facebook::jsi::Pointer(std::move(get_self<JsiPointer>(pointer)->m_pointer));
  return value;
}

static Microsoft::ReactNative::JsiValueData ReturnValue(
    facebook::jsi::Value &&value,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  switch (value.kind_) {
    case facebook::jsi::Value::UndefinedKind:
      ptrResult = nullptr;
      return {JsiValueKind::Undefined, 0};
    case facebook::jsi::Value::NullKind:
      ptrResult = nullptr;
      return {JsiValueKind::Null, 0};
    case facebook::jsi::Value::BooleanKind:
      ptrResult = nullptr;
      return {JsiValueKind::Boolean, value.data_.boolean ? 1.0 : 0};
    case facebook::jsi::Value::NumberKind:
      ptrResult = nullptr;
      return {JsiValueKind::Number, value.data_.number};
    case facebook::jsi::Value::SymbolKind:
      ptrResult = MakeJsiPointer(std::move(value.data_.pointer));
      return {JsiValueKind::Symbol, 0};
    case facebook::jsi::Value::StringKind:
      ptrResult = MakeJsiPointer(std::move(value.data_.pointer));
      return {JsiValueKind::String, 0};
    case facebook::jsi::Value::ObjectKind:
      ptrResult = MakeJsiPointer(std::move(value.data_.pointer));
      return {JsiValueKind::Object, 0};
    default:
      VerifyElseCrashSz(false, "Unexpected JSI value type");
  }
}

static Microsoft::ReactNative::JsiValueData ReturnValue(
    facebook::jsi::Runtime &runtime,
    facebook::jsi::Value const &value,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  switch (value.kind_) {
    case facebook::jsi::Value::UndefinedKind:
      ptrResult = nullptr;
      return {JsiValueKind::Undefined, 0};
    case facebook::jsi::Value::NullKind:
      ptrResult = nullptr;
      return {JsiValueKind::Null, 0};
    case facebook::jsi::Value::BooleanKind:
      ptrResult = nullptr;
      return {JsiValueKind::Boolean, value.data_.boolean ? 1.0 : 0};
    case facebook::jsi::Value::NumberKind:
      ptrResult = nullptr;
      return {JsiValueKind::Number, value.data_.number};
    case facebook::jsi::Value::SymbolKind:
      ptrResult = MakeJsiPointer(runtime.cloneSymbol(value.data_.pointer.ptr_));
      return {JsiValueKind::Symbol, 0};
    case facebook::jsi::Value::StringKind:
      ptrResult = MakeJsiPointer(runtime.cloneString(value.data_.pointer.ptr_));
      return {JsiValueKind::String, 0};
    case facebook::jsi::Value::ObjectKind:
      ptrResult = MakeJsiPointer(runtime.cloneObject(value.data_.pointer.ptr_));
      return {JsiValueKind::Object, 0};
    default:
      VerifyElseCrashSz(false, "Unexpected JSI value type");
  }
}

static facebook::jsi::Value ReturnJsiValue(
    Microsoft::ReactNative::JsiValueData const &data,
    Microsoft::ReactNative::JsiPointer &&pointer) {
  switch (data.Kind) {
    case Microsoft::ReactNative::JsiValueKind::Undefined:
      return facebook::jsi::Value{};
    case Microsoft::ReactNative::JsiValueKind::Null:
      return facebook::jsi::Value{nullptr};
    case Microsoft::ReactNative::JsiValueKind::Boolean:
      return facebook::jsi::Value{data.NumberValue != 0};
    case Microsoft::ReactNative::JsiValueKind::Number:
      return facebook::jsi::Value{data.NumberValue};
    case Microsoft::ReactNative::JsiValueKind::Symbol:
      return MakePointerValue(facebook::jsi::Value::SymbolKind, std::move(pointer));
    case Microsoft::ReactNative::JsiValueKind::String:
      return MakePointerValue(facebook::jsi::Value::StringKind, std::move(pointer));
    case Microsoft::ReactNative::JsiValueKind::Object:
      return MakePointerValue(facebook::jsi::Value::ObjectKind, std::move(pointer));
    default:
      VerifyElseCrashSz(false, "Unexpected JSI value type");
  }
}

struct ValueRef {
  ValueRef() = default;
  ValueRef(ValueRef &&) = default;
  ValueRef &operator=(ValueRef &&) = default;
  ValueRef(facebook::jsi::Value &&value) : m_value{std::move(value)} {}
  ~ValueRef() {
    m_value.data_.pointer.ptr_ = nullptr;
  }

 private:
  facebook::jsi::Value m_value{};
};

static ValueRef MakePointerValueRef(
    facebook::jsi::Value::ValueKind valueKind,
    Microsoft::ReactNative::JsiPointer const &pointer) {
  facebook::jsi::Value value{valueKind};
  new (&value.data_.pointer) facebook::jsi::Pointer(get_self<JsiPointer>(pointer)->m_pointer.ptr_);
  return ValueRef{std::move(value)};
}

static ValueRef ReturnJsiValueRef(
    Microsoft::ReactNative::JsiValueData const &data,
    Microsoft::ReactNative::JsiPointer const &pointer) {
  switch (data.Kind) {
    case Microsoft::ReactNative::JsiValueKind::Undefined:
      return ValueRef{facebook::jsi::Value{}};
    case Microsoft::ReactNative::JsiValueKind::Null:
      return ValueRef{facebook::jsi::Value{nullptr}};
    case Microsoft::ReactNative::JsiValueKind::Boolean:
      return ValueRef{facebook::jsi::Value{data.NumberValue != 0}};
    case Microsoft::ReactNative::JsiValueKind::Number:
      return ValueRef{facebook::jsi::Value{data.NumberValue}};
    case Microsoft::ReactNative::JsiValueKind::Symbol:
      return MakePointerValueRef(facebook::jsi::Value::SymbolKind, pointer);
    case Microsoft::ReactNative::JsiValueKind::String:
      return MakePointerValueRef(facebook::jsi::Value::StringKind, pointer);
    case Microsoft::ReactNative::JsiValueKind::Object:
      return MakePointerValueRef(facebook::jsi::Value::ObjectKind, pointer);
    default:
      VerifyElseCrashSz(false, "Unexpected JSI value type");
  }
}

static std::shared_ptr<facebook::jsi::HostObject> MakeHostObject(
    Microsoft::ReactNative::IJsiHostObject const &hostObject) {
  return std::make_shared<HostObjectWrapper>(hostObject);
}

struct JsiValueParts {
  Microsoft::ReactNative::JsiValueData data;
  Microsoft::ReactNative::JsiPointer pointer;
};

static JsiValueParts GetJsiValueParts(facebook::jsi::Runtime &runtime, facebook::jsi::Value const &value) {
  Microsoft::ReactNative::JsiPointer resultPtr{nullptr};
  Microsoft::ReactNative::JsiValueData resultData = ReturnValue(runtime, value, resultPtr);
  return JsiValueParts{resultData, std::move(resultPtr)};
}

static facebook::jsi::HostFunctionType MakeHostFunction(Microsoft::ReactNative::JsiHostFunction const &hostFunc) {
  return [hostFunc](
             facebook::jsi::Runtime &runtime,
             facebook::jsi::Value const &thisVal,
             facebook::jsi::Value const *args,
             size_t count) -> facebook::jsi::Value {
    auto [thisData, thisPtr] = GetJsiValueParts(runtime, thisVal);
    std::vector<Microsoft::ReactNative::JsiValueData> dataArgs;
    std::vector<Microsoft::ReactNative::JsiPointer> ptrArgs;
    dataArgs.reserve(count);
    ptrArgs.reserve(count);
    for (size_t i = 0; i < count; ++i) {
      facebook::jsi::Value const *arg = args + i;
      auto [dataArg, ptrArg] = GetJsiValueParts(runtime, *arg);
      dataArgs.push_back(dataArg);
      ptrArgs.push_back(std::move(ptrArg));
    }
    Microsoft::ReactNative::JsiPointer ptrResult{nullptr};
    Microsoft::ReactNative::JsiValueData dataResult = hostFunc(
        make<JsiRuntime>(runtime),
        thisData,
        thisPtr,
        dataArgs,
        ptrArgs,
        /*out*/ ptrResult);
    return ReturnJsiValue(dataResult, std::move(ptrResult));
  };
}

//===========================================================================
// JsiPointer implementation
//===========================================================================

JsiPointer::JsiPointer(facebook::jsi::Pointer &&pointer) noexcept : m_pointer{std::move(pointer)} {}

JsiPointer::JsiPointer(facebook::jsi::Runtime::PointerValue *ptr) noexcept : m_pointer{ptr} {}

//===========================================================================
// JsiPointerRef implementation
//===========================================================================

JsiPointerRef::JsiPointerRef(Microsoft::ReactNative::JsiPointer &&jsiPointer) : m_jsiPointer{std::move(jsiPointer)} {}

JsiPointerRef::~JsiPointerRef() {
  get_self<JsiPointer>(m_jsiPointer)->m_pointer.ptr_ = nullptr;
}

//===========================================================================
// HostObjectWrapper implementation
//===========================================================================

HostObjectWrapper::HostObjectWrapper(Microsoft::ReactNative::IJsiHostObject const &hostObject) noexcept
    : m_hostObject{hostObject} {}

facebook::jsi::Value HostObjectWrapper::get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &name) {
  Microsoft::ReactNative::JsiPointer ptrResult{nullptr};
  auto valueData = m_hostObject.GetProperty(make<JsiRuntime>(runtime), MakeJsiPointerRef(name).m_jsiPointer, ptrResult);
  return ReturnJsiValue(valueData, std::move(ptrResult));
}

void HostObjectWrapper::set(
    facebook::jsi::Runtime &runtime,
    const facebook::jsi::PropNameID &name,
    const facebook::jsi::Value &value) {
  Microsoft::ReactNative::JsiPointer ptrValue{nullptr};
  Microsoft::ReactNative::JsiValueData data = ReturnValue(runtime, value, ptrValue);
  m_hostObject.SetProperty(
      make<JsiRuntime>(runtime), MakeJsiPointer(facebook::jsi::PropNameID{runtime, name}), data, ptrValue);
}

std::vector<facebook::jsi::PropNameID> HostObjectWrapper::getPropertyNames(facebook::jsi::Runtime &runtime) noexcept {
  std::vector<facebook::jsi::PropNameID> result;
  auto vectorView = m_hostObject.GetPropertyNames(make<JsiRuntime>(runtime));
  result.reserve(vectorView.Size());
  for (auto const &item : vectorView) {
    result.emplace_back(runtime, AsPropNameID(item));
  }

  return result;
}

//===========================================================================
// JsiRuntime implementation
//===========================================================================

JsiRuntime::JsiRuntime(facebook::jsi::Runtime &runtime) noexcept : m_runtime{runtime} {}

Microsoft::ReactNative::JsiValueData JsiRuntime::EvaluateJavaScript(
    Microsoft::ReactNative::IJsiBuffer const &buffer,
    hstring const &sourceUrl,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  facebook::jsi::Value result;
  buffer.GetData([this, &result, &sourceUrl](array_view<uint8_t const> bytes) {
    result = m_runtime.evaluateJavaScript(std::make_shared<JsiBufferWrapper>(bytes), to_string(sourceUrl));
  });
  return ReturnValue(std::move(result), ptrResult);
}

Windows::Foundation::IInspectable JsiRuntime::PrepareJavaScript(
    Microsoft::ReactNative::IJsiBuffer const &buffer,
    hstring const &sourceUrl) {}

Microsoft::ReactNative::JsiValueData JsiRuntime::EvaluatePreparedJavaScript(
    Windows::Foundation::IInspectable const &js) {}

Microsoft::ReactNative::JsiPointer JsiRuntime::Global() noexcept {
  return MakeJsiPointer(m_runtime.global());
}

hstring JsiRuntime::Description() {
  return to_hstring(m_runtime.description());
}

bool JsiRuntime::IsInspectable() {
  return m_runtime.isInspectable();
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CloneSymbol(Microsoft::ReactNative::JsiPointer const &symbol) {
  return MakeJsiPointer(m_runtime.cloneSymbol(GetPointerValue(symbol)));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CloneString(Microsoft::ReactNative::JsiPointer const &str) {
  return MakeJsiPointer(m_runtime.cloneString(GetPointerValue(str)));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CloneObject(Microsoft::ReactNative::JsiPointer const &obj) {
  return MakeJsiPointer(m_runtime.cloneObject(GetPointerValue(obj)));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::ClonePropertyNameId(
    Microsoft::ReactNative::JsiPointer const &propertyNameId) {
  return MakeJsiPointer(m_runtime.clonePropNameID(GetPointerValue(propertyNameId)));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreatePropertyNameIdFromAscii(array_view<uint8_t const> ascii) {
  return MakeJsiPointer(
      m_runtime.createPropNameIDFromAscii(reinterpret_cast<char const *>(ascii.data()), ascii.size()));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreatePropertyNameIdFromUtf8(array_view<uint8_t const> utf8) {
  return MakeJsiPointer(m_runtime.createPropNameIDFromUtf8(utf8.data(), utf8.size()));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreatePropertyNameIdFromString(
    Microsoft::ReactNative::JsiPointer const &str) {
  return MakeJsiPointer(m_runtime.createPropNameIDFromString(AsString(str)));
}

void JsiRuntime::PropertyNameIdToUtf8(
    Microsoft::ReactNative::JsiPointer const &propertyNameId,
    Microsoft::ReactNative::JsiDataHandler const &utf8Handler) {
  std::string utf8 = m_runtime.utf8(AsPropNameID(propertyNameId));
  uint8_t const *data = reinterpret_cast<uint8_t const *>(utf8.data());
  utf8Handler({data, data + utf8.size()});
}

bool JsiRuntime::ComparePropertyNameIds(
    Microsoft::ReactNative::JsiPointer const &left,
    Microsoft::ReactNative::JsiPointer const &right) {
  return m_runtime.compare(AsPropNameID(left), AsPropNameID(right));
}

void JsiRuntime::SymbolToUtf8(
    Microsoft::ReactNative::JsiPointer const &symbol,
    Microsoft::ReactNative::JsiDataHandler const &utf8Handler) {
  std::string utf8 = m_runtime.symbolToString(AsSymbol(symbol));
  uint8_t const *data = reinterpret_cast<uint8_t const *>(utf8.data());
  utf8Handler({data, data + utf8.size()});
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreateStringFromAscii(array_view<uint8_t const> ascii) {
  return MakeJsiPointer(m_runtime.createStringFromAscii(reinterpret_cast<char const *>(ascii.data()), ascii.size()));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreateStringFromUtf8(array_view<uint8_t const> utf8) {
  return MakeJsiPointer(m_runtime.createStringFromUtf8(utf8.data(), utf8.size()));
}

void JsiRuntime::StringToUtf8(
    Microsoft::ReactNative::JsiPointer const &str,
    Microsoft::ReactNative::JsiDataHandler const &utf8Handler) {
  std::string utf8 = m_runtime.utf8(AsString(str));
  uint8_t const *data = reinterpret_cast<uint8_t const *>(utf8.data());
  utf8Handler({data, data + utf8.size()});
}

Microsoft::ReactNative::JsiValueData JsiRuntime::CreateValueFromJsonUtf8(
    array_view<uint8_t const> json,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  facebook::jsi::Value value = m_runtime.createValueFromJsonUtf8(json.data(), json.size());
  return ReturnValue(std::move(value), ptrResult);
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreateObject() {
  return MakeJsiPointer(m_runtime.createObject());
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreateObjectWithHostObject(
    Microsoft::ReactNative::IJsiHostObject const &hostObject) {
  return MakeJsiPointer(m_runtime.createObject(MakeHostObject(hostObject)));
}

Microsoft::ReactNative::IJsiHostObject JsiRuntime::GetHostObject(Microsoft::ReactNative::JsiPointer const &obj) {
  auto hostObject = m_runtime.getHostObject(AsObject(obj));
  // TODO: using static_pointer_cast is unsafe here. How to use shared_ptr without RTTI?
  auto wrapper = std::static_pointer_cast<HostObjectWrapper>(hostObject);
  return wrapper ? wrapper->m_hostObject : nullptr;
}

Microsoft::ReactNative::JsiHostFunction JsiRuntime::GetHostFunction(
    Microsoft::ReactNative::JsiPointer const & /*func*/) {
  // auto hostFunction = m_runtime.getHostFunction(AsFunction(func));
  // TODO: implement mapping
  return nullptr;
}

Microsoft::ReactNative::JsiValueData JsiRuntime::GetProperty(
    Microsoft::ReactNative::JsiPointer const &obj,
    Microsoft::ReactNative::JsiPointer const &propertyNameId,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  auto value = m_runtime.getProperty(AsObject(obj), AsPropNameID(propertyNameId));
  return ReturnValue(std::move(value), ptrResult);
}

Microsoft::ReactNative::JsiValueData JsiRuntime::GetPropertyWithString(
    Microsoft::ReactNative::JsiPointer const &obj,
    Microsoft::ReactNative::JsiPointer const &nameStr,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  auto value = m_runtime.getProperty(AsObject(obj), AsString(nameStr));
  return ReturnValue(std::move(value), ptrResult);
}

bool JsiRuntime::HasProperty(
    Microsoft::ReactNative::JsiPointer const &obj,
    Microsoft::ReactNative::JsiPointer const &propertyNameId) {
  return m_runtime.hasProperty(AsObject(obj), AsPropNameID(propertyNameId));
}

bool JsiRuntime::HasPropertyWithString(
    Microsoft::ReactNative::JsiPointer const &obj,
    Microsoft::ReactNative::JsiPointer const &nameStr) {
  return m_runtime.hasProperty(AsObject(obj), AsString(nameStr));
}

void JsiRuntime::SetProperty(
    Microsoft::ReactNative::JsiPointer const &obj,
    Microsoft::ReactNative::JsiPointer const &propertyNameId,
    Microsoft::ReactNative::JsiValueData const &value,
    Microsoft::ReactNative::JsiPointer const &ptrValue) {
  auto valueRef = ReturnJsiValueRef(value, ptrValue);
  m_runtime.setPropertyValue(
      const_cast<facebook::jsi::Object &>(AsObject(obj)), AsPropNameID(propertyNameId), valueRef.m_value);
}

void JsiRuntime::SetPropertyWithString(
    Microsoft::ReactNative::JsiPointer const &obj,
    Microsoft::ReactNative::JsiPointer const &nameStr,
    Microsoft::ReactNative::JsiValueData const &value,
    Microsoft::ReactNative::JsiPointer const &ptrValue) {
  auto valueRef = ReturnJsiValueRef(value, ptrValue);
  m_runtime.setPropertyValue(const_cast<facebook::jsi::Object &>(AsObject(obj)), AsString(nameStr), valueRef.m_value);
}

bool JsiRuntime::IsArray(Microsoft::ReactNative::JsiPointer const &obj) {
  return m_runtime.isArray(AsObject(obj));
}

bool JsiRuntime::IsArrayBuffer(Microsoft::ReactNative::JsiPointer const &obj) {
  return m_runtime.isArrayBuffer(AsObject(obj));
}

bool JsiRuntime::IsFunction(Microsoft::ReactNative::JsiPointer const &obj) {
  return m_runtime.isFunction(AsObject(obj));
}

bool JsiRuntime::IsHostObject(Microsoft::ReactNative::JsiPointer const &obj) {
  return m_runtime.isHostObject(AsObject(obj));
}

bool JsiRuntime::IsHostFunction(Microsoft::ReactNative::JsiPointer const &obj) {
  return m_runtime.isHostFunction(AsFunction(obj));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::GetPropertyNameArray(Microsoft::ReactNative::JsiPointer const &obj) {
  return MakeJsiPointer(m_runtime.getPropertyNames(AsObject(obj)));
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreateWeakObject(Microsoft::ReactNative::JsiPointer const &obj) {
  return MakeJsiPointer(m_runtime.createWeakObject(AsObject(obj)));
}

Microsoft::ReactNative::JsiValueData JsiRuntime::LockWeakObject(
    Microsoft::ReactNative::JsiPointer const &weakObject,
    Microsoft::ReactNative::JsiPointer &obj) {
  auto value = m_runtime.lockWeakObject(AsWeakObject(weakObject));
  return ReturnValue(std::move(value), obj);
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreateArray(uint32_t size) {
  return MakeJsiPointer(m_runtime.createArray(size));
}

uint32_t JsiRuntime::GetArraySize(Microsoft::ReactNative::JsiPointer const &arr) {
  return static_cast<uint32_t>(m_runtime.size(AsArray(arr)));
}

uint32_t JsiRuntime::GetArrayBufferSize(Microsoft::ReactNative::JsiPointer const &arrayBuffer) {
  return static_cast<uint32_t>(m_runtime.size(AsArrayBuffer(arrayBuffer)));
}

void JsiRuntime::ArrayBufferToUtf8(
    Microsoft::ReactNative::JsiPointer const &arrayBuffer,
    Microsoft::ReactNative::JsiDataHandler const &utf8Handler) {
  auto data = m_runtime.data(AsArrayBuffer(arrayBuffer));
  return utf8Handler({data, data + m_runtime.size(AsArrayBuffer(arrayBuffer))});
}

Microsoft::ReactNative::JsiValueData JsiRuntime::GetValueAtIndex(
    Microsoft::ReactNative::JsiPointer const &arr,
    uint32_t index,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  auto value = AsArray(arr).getValueAtIndex(m_runtime, index);
  return ReturnValue(std::move(value), ptrResult);
}

void JsiRuntime::SetValueAtIndex(
    Microsoft::ReactNative::JsiPointer const &arr,
    uint32_t index,
    Microsoft::ReactNative::JsiValueData const &value,
    Microsoft::ReactNative::JsiPointer const &ptrValue) {
  auto valueRef = ReturnJsiValueRef(value, ptrValue);
  m_runtime.setValueAtIndexImpl(const_cast<facebook::jsi::Array &>(AsArray(arr)), index, valueRef.m_value);
}

Microsoft::ReactNative::JsiPointer JsiRuntime::CreateFunctionFromHostFunction(
    Microsoft::ReactNative::JsiPointer const &propNameId,
    uint32_t paramCount,
    Microsoft::ReactNative::JsiHostFunction const &hostFunc) {
  return MakeJsiPointer(
      m_runtime.createFunctionFromHostFunction(AsPropNameID(propNameId), paramCount, MakeHostFunction(hostFunc)));
}

Microsoft::ReactNative::JsiValueData JsiRuntime::Call(
    Microsoft::ReactNative::JsiPointer const &func,
    Microsoft::ReactNative::JsiValueData const &thisData,
    Microsoft::ReactNative::JsiPointer const &thisPtr,
    array_view<Microsoft::ReactNative::JsiValueData const> dataArgs,
    array_view<Microsoft::ReactNative::JsiPointer const> ptrArgs,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  auto thisValueRef = ReturnJsiValueRef(thisData, thisPtr);
  size_t argCount = dataArgs.size();
  VerifyElseCrash(argCount == ptrArgs.size());
  std::vector<ValueRef> args;
  args.reserve(argCount);
  for (size_t i = 0; i < argCount; ++i) {
    args.push_back(ReturnJsiValueRef(dataArgs[static_cast<uint32_t>(i)], ptrArgs[static_cast<uint32_t>(i)]));
  }

  return ReturnValue(
      m_runtime.call(
          AsFunction(func),
          thisValueRef.m_value,
          reinterpret_cast<facebook::jsi::Value const *>(args.data()),
          argCount),
      ptrResult);
}

Microsoft::ReactNative::JsiValueData JsiRuntime::CallAsConstructor(
    Microsoft::ReactNative::JsiPointer const &func,
    array_view<Microsoft::ReactNative::JsiValueData const> dataArgs,
    array_view<Microsoft::ReactNative::JsiPointer const> ptrArgs,
    Microsoft::ReactNative::JsiPointer &ptrResult) {
  size_t argCount = dataArgs.size();
  VerifyElseCrash(argCount == ptrArgs.size());
  std::vector<ValueRef> args;
  args.reserve(argCount);
  for (size_t i = 0; i < argCount; ++i) {
    args.push_back(ReturnJsiValueRef(dataArgs[static_cast<uint32_t>(i)], ptrArgs[static_cast<uint32_t>(i)]));
  }

  return ReturnValue(
      m_runtime.callAsConstructor(
          AsFunction(func), reinterpret_cast<facebook::jsi::Value const *>(args.data()), argCount),
      ptrResult);
}

uint64_t JsiRuntime::PushScope() {
  return reinterpret_cast<uint64_t>(m_runtime.pushScope());
}

void JsiRuntime::PopScope(uint64_t scopeState) {
  m_runtime.popScope(reinterpret_cast<facebook::jsi::Runtime::ScopeState *>(scopeState));
}

bool JsiRuntime::SymbolStrictEquals(
    Microsoft::ReactNative::JsiPointer const &left,
    Microsoft::ReactNative::JsiPointer const &right) {
  return m_runtime.strictEquals(AsSymbol(left), AsSymbol(right));
}

bool JsiRuntime::StringStrictEquals(
    Microsoft::ReactNative::JsiPointer const &left,
    Microsoft::ReactNative::JsiPointer const &right) {
  return m_runtime.strictEquals(AsString(left), AsString(right));
}

bool JsiRuntime::ObjectStrictEquals(
    Microsoft::ReactNative::JsiPointer const &left,
    Microsoft::ReactNative::JsiPointer const &right) {
  return m_runtime.strictEquals(AsObject(left), AsObject(right));
}

bool JsiRuntime::InstanceOf(
    Microsoft::ReactNative::JsiPointer const &obj,
    Microsoft::ReactNative::JsiPointer const &constructor) {
  return m_runtime.instanceOf(AsObject(obj), AsFunction(constructor));
}

} // namespace winrt::Microsoft::ReactNative::implementation

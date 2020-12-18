// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "NapiApi.h"
#include <sstream>
#include <utility>
#include "Unicode.h"

namespace react::jsi {

//=============================================================================
// NapiApi::JsRefHolder implementation
//=============================================================================

NapiApi::NapiRefHolder::NapiRefHolder(napi_env env, napi_ref ref) noexcept : m_env{env}, m_ref{ref} {}

NapiApi::NapiRefHolder::NapiRefHolder(napi_env env, napi_value value) noexcept : m_env{env} {
  // TODO: [vmoroz] Add CHECK_NAPI
  napi_create_reference(m_env, value, 1, &m_ref);
}

NapiApi::NapiRefHolder::NapiRefHolder(NapiRefHolder &&other) noexcept
    : m_env{std::exchange(other.m_env, nullptr)}, m_ref{std::exchange(other.m_ref, nullptr)} {}

NapiApi::NapiRefHolder &NapiApi::NapiRefHolder::operator=(NapiRefHolder &&other) noexcept {
  if (this != &other) {
    NapiRefHolder temp{std::move(*this)};
    m_ref = std::exchange(other.m_ref, nullptr);
  }

  return *this;
}

NapiApi::NapiRefHolder::~NapiRefHolder() noexcept {
  if (m_ref) {
    // Clear m_ref before calling napi_delete_reference on it to make sure that we always hold a valid m_ref.
    // TODO: [vmoroz] Add CHECK_NAPI
    napi_delete_reference(m_env, std::exchange(m_ref, nullptr));
  }
}

//=============================================================================
// NapiApi::ExceptionThrowerHolder implementation
//=============================================================================

/*static*/ thread_local NapiApi::IExceptionThrower *NapiApi::ExceptionThrowerHolder::tls_exceptionThrower{nullptr};

//=============================================================================
// NapiApi implementation
//=============================================================================

[[noreturn]] void NapiApi::ThrowJsException(napi_status errorCode) {
  napi_value jsError{};
  NapiVerifyElseCrash(napi_get_and_clear_last_exception(m_env, &jsError) == napi_ok, "Cannot retrieve JS exception.");
  if (auto thrower = ExceptionThrowerHolder::Get()) {
    thrower->ThrowJsExceptionOverride(errorCode, jsError);
  } else {
    std::ostringstream errorString;
    errorString << "A call to NAPI API returned error code 0x" << std::hex << errorCode << '.';
    throw std::exception(errorString.str().c_str());
  }
}

[[noreturn]] void NapiApi::ThrowNativeException(char const *errorMessage) {
  if (auto thrower = ExceptionThrowerHolder::Get()) {
    thrower->ThrowNativeExceptionOverride(errorMessage);
  } else {
    throw std::exception(errorMessage);
  }
}

napi_ref NapiApi::CreateReference(napi_value value) {
  napi_ref result{};
  CHECK_NAPI(napi_create_reference(m_env, value, 1, &result));
  return result;
}

void NapiApi::DeleteReference(napi_ref ref) {
  // TODO: [vmoroz] make it safe to be called from another thread per JSI spec.
  CHECK_NAPI(napi_delete_reference(m_env, ref));
}

napi_value NapiApi::GetReferenceValue(napi_ref ref) {
  napi_value result{};
  CHECK_NAPI(napi_get_reference_value(m_env, ref, &result));
  return result;
}

///*static*/ JsRuntimeHandle NapiApi::CreateRuntime(
//    JsRuntimeAttributes attributes,
//    JsThreadServiceCallback threadService) {
//  JsRuntimeHandle runtime{JS_INVALID_REFERENCE};
//  NapiVerifyElseThrow(
//      JsCreateRuntime(attributes, threadService, &runtime) == JsErrorCode::JsNoError, "Cannot create Chakra
//      runtime.");
//  return runtime;
//}
//
///*static*/ void NapiApi::DisposeRuntime(JsRuntimeHandle runtime) {
//  NapiVerifyElseThrow(JsDisposeRuntime(runtime) == JsErrorCode::JsNoError, "Cannot dispaose Chakra runtime.");
//}
//
///*static*/ uint32_t NapiApi::AddRef(JsRef ref) {
//  uint32_t result{0};
//  NapiVerifyJsErrorElseThrow(JsAddRef(ref, &result));
//  return result;
//}
//
///*static*/ uint32_t NapiApi::Release(JsRef ref) {
//  uint32_t result{0};
//  NapiVerifyJsErrorElseThrow(JsRelease(ref, &result));
//  return result;
//}
//
///*static*/ JsContextRef NapiApi::CreateContext(JsRuntimeHandle runtime) {
//  JsContextRef context{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateContext(runtime, &context));
//  return context;
//}
//
///*static*/ JsContextRef NapiApi::GetCurrentContext() {
//  JsContextRef context{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetCurrentContext(&context));
//  return context;
//}
//
///*static*/ void NapiApi::SetCurrentContext(JsContextRef context) {
//  NapiVerifyJsErrorElseThrow(JsSetCurrentContext(context));
//}
//
///*static*/ JsPropertyIdRef NapiApi::GetPropertyIdFromName(wchar_t const *name) {
//  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetPropertyIdFromName(name, &propertyId));
//  return propertyId;
//}
//
///*static*/ JsPropertyIdRef NapiApi::GetPropertyIdFromString(JsValueRef value) {
//  return GetPropertyIdFromName(StringToPointer(value).data());
//}
//
///*static*/ JsPropertyIdRef NapiApi::GetPropertyIdFromName(std::string_view name) {
//  NapiVerifyElseThrow(name.data(), "Property name cannot be a nullptr.");
//
//  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
//  // We use a #ifdef here because we can avoid a UTF-8 to UTF-16 conversion
//  // using ChakraCore's JsCreatePropertyId API.
//#ifdef CHAKRACORE
//  NapiVerifyJsErrorElseThrow(JsCreatePropertyId(name.data(), name.length(), &propertyId));
//#else
//  std::wstring utf16 = Common::Unicode::Utf8ToUtf16(name.data(), name.length());
//  NapiVerifyJsErrorElseThrow(JsGetPropertyIdFromName(utf16.data(), &propertyId));
//#endif
//  return propertyId;
//}
//
///*static*/ wchar_t const *NapiApi::GetPropertyNameFromId(JsPropertyIdRef propertyId) {
//  NapiVerifyElseThrow(
//      GetPropertyIdType(propertyId) == JsPropertyIdTypeString,
//      "It is illegal to retrieve the name associated with a property symbol.");
//
//  wchar_t const *name{nullptr};
//  NapiVerifyJsErrorElseThrow(JsGetPropertyNameFromId(propertyId, &name));
//  return name;
//}
//
///*static*/ JsValueRef NapiApi::GetPropertyStringFromId(JsPropertyIdRef propertyId) {
//  return PointerToString(GetPropertyNameFromId(propertyId));
//}
//
///*static*/ JsValueRef NapiApi::GetSymbolFromPropertyId(JsPropertyIdRef propertyId) {
//  NapiVerifyElseThrow(
//      GetPropertyIdType(propertyId) == JsPropertyIdTypeSymbol,
//      "It is illegal to retrieve the symbol associated with a property name.");
//
//  JsValueRef symbol{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetSymbolFromPropertyId(propertyId, &symbol));
//  return symbol;
//}
//
///*static*/ JsPropertyIdType NapiApi::GetPropertyIdType(JsPropertyIdRef propertyId) {
//  JsPropertyIdType type{JsPropertyIdType::JsPropertyIdTypeString};
//  NapiVerifyJsErrorElseThrow(JsGetPropertyIdType(propertyId, &type));
//  return type;
//}
//
///*static*/ JsPropertyIdRef NapiApi::GetPropertyIdFromSymbol(JsValueRef symbol) {
//  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetPropertyIdFromSymbol(symbol, &propertyId));
//  return propertyId;
//}
//
///*static*/ JsPropertyIdRef NapiApi::GetPropertyIdFromSymbol(std::wstring_view symbolDescription) {
//  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetPropertyIdFromSymbol(CreateSymbol(symbolDescription), &propertyId));
//  return propertyId;
//}
//
///*static*/ JsValueRef NapiApi::CreateSymbol(JsValueRef symbolDescription) {
//  JsValueRef symbol{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateSymbol(symbolDescription, &symbol));
//  return symbol;
//}
//
///*static*/ JsValueRef NapiApi::CreateSymbol(std::wstring_view symbolDescription) {
//  return CreateSymbol(PointerToString(symbolDescription));
//}

napi_value NapiApi::GetUndefined() {
  napi_value result{nullptr};
  CHECK_NAPI(napi_get_undefined(m_env, &result));
  return result;
}

napi_value NapiApi::GetNull() {
  napi_value result{nullptr};
  CHECK_NAPI(napi_get_null(m_env, &result));
  return result;
}

napi_value NapiApi::GetBoolean(bool value) {
  napi_value result{nullptr};
  CHECK_NAPI(napi_get_boolean(m_env, value, &result));
  return result;
}

bool NapiApi::GetValueBool(napi_value value) {
  bool result{nullptr};
  CHECK_NAPI(napi_get_value_bool(m_env, value, &result));
  return result;
}

///*static*/ JsValueType NapiApi::GetValueType(JsValueRef value) {
//  napi_valuetype result{nullptr};
//  CHECK_NAPI(napi_get_valuetype(m_env, value, &result));
//  return result;
//}
//
///*static*/ JsValueRef NapiApi::DoubleToNumber(double value) {
//  JsValueRef result{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsDoubleToNumber(value, &result));
//  return result;
//}
//
///*static*/ JsValueRef NapiApi::IntToNumber(int32_t value) {
//  JsValueRef numberValue{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsIntToNumber(value, &numberValue));
//  return numberValue;
//}
//
///*static*/ double NapiApi::NumberToDouble(JsValueRef value) {
//  double doubleValue{0};
//  NapiVerifyJsErrorElseThrow(JsNumberToDouble(value, &doubleValue));
//  return doubleValue;
//}
//
///*static*/ int32_t NapiApi::NumberToInt(JsValueRef value) {
//  int intValue{0};
//  NapiVerifyJsErrorElseThrow(JsNumberToInt(value, &intValue));
//  return intValue;
//}
//
// napi_value NapiApi::CreateStringLatin1(std::string_view value) {
//  NapiVerifyElseThrow(value.data(), "Cannot convert a nullptr to a JS string.");
//
//  napi_value result{};
//  NapiVerifyJsErrorElseThrow(napi_create_string_latin1(m_env, value.data(), value.size(), &result));
//  return result;
//}
//
// napi_value NapiApi::CreateStringUtf8(std::string_view value) {
//  NapiVerifyElseThrow(value.data(), "Cannot convert a nullptr to a JS string.");
//
//  napi_value result{};
//  NapiVerifyJsErrorElseThrow(napi_create_string_utf8(m_env, value.data(), value.size(), &result));
//  return result;
//}
//
// napi_value NapiApi::CreateStringUtf16(std::u16string_view value) {
//  NapiVerifyElseThrow(value.data(), "Cannot convert a nullptr to a JS string.");
//
//  napi_value result{};
//  NapiVerifyJsErrorElseThrow(napi_create_string_utf16(m_env, value.data(), value.size(), &result));
//  return result;
//}
//
// std::string NapiApi::PropertyIdToStdString(napi_ref propertyIdRef) {
//  return StringToStdString(propertyIdRef);
//}
//
// napi_value NapiApi::GetReferenceValue(napi_ref ref) {
//  napi_value value{};
//  NapiVerifyJsErrorElseThrow(napi_get_reference_value(m_env, ref, &value));
//  return value;
//}
//
///*static*/ std::wstring_view NapiApi::StringToPointer(JsValueRef string) {
//  wchar_t const *utf16{nullptr};
//  size_t length{0};
//  NapiVerifyJsErrorElseThrow(JsStringToPointer(string, &utf16, &length));
//  return {utf16, length};
//}
//
// std::string NapiApi::StringToStdString(napi_value stringValue) {
//  std::string result;
//  NapiVerifyElseThrow(
//      GetValueType(stringValue) == napi_valuetype::napi_string,
//      "Cannot convert a non JS string ChakraObjectRef to a std::string.");
//  size_t strLength{};
//  NapiVerifyJsErrorElseThrow(napi_get_value_string_utf8(m_env, stringValue, nullptr, 0, &strLength));
//  result.assign(strLength, '\0');
//  size_t copiedLength{};
//  NapiVerifyJsErrorElseThrow(
//      napi_get_value_string_utf8(m_env, stringValue, result.data(), result.capacity(), &copiedLength));
//  NapiVerifyElseThrow(result.length() == copiedLength, "Unexpected string length");
//  return result;
//}
//
// std::string NapiApi::StringToStdString(napi_ref stringRef) {
//  return StringToStdString(GetReferenceValue(stringRef));
//}
//
///*static*/ JsValueRef NapiApi::ConvertValueToString(JsValueRef value) {
//  JsValueRef stringValue{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsConvertValueToString(value, &stringValue));
//  return stringValue;
//}
//
///*static*/ napi_ref NapiApi::GetGlobalObject(napi_env env) {
//  JsValueRef globalObject{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(napi_get_global(env, &global));
//  return globalObject;
//}
//
///*static*/ JsValueRef NapiApi::CreateObject() {
//  JsValueRef object{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateObject(&object));
//  return object;
//}
//
// napi_value
// NapiApi::CreateExternalObject(void *data, napi_finalize finalizeCallback) {
//  napi_value result{};
//  NapiVerifyJsErrorElseThrow(napi_create_external(m_env, data, finalizeCallback, nullptr, &value));
//  return result;
//}
//
///*static*/ JsValueRef NapiApi::GetPrototype(JsValueRef object) {
//  JsValueRef prototype{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetPrototype(object, &prototype));
//  return prototype;
//}
//
///*static*/ bool NapiApi::InstanceOf(JsValueRef object, JsValueRef constructor) {
//  bool result{false};
//  NapiVerifyJsErrorElseThrow(JsInstanceOf(object, constructor, &result));
//  return result;
//}
//
///*static*/ JsValueRef NapiApi::GetProperty(JsValueRef object, JsPropertyIdRef propertyId) {
//  JsValueRef result{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetProperty(object, propertyId, &result));
//  return result;
//}
//
///*static*/ JsValueRef NapiApi::GetOwnPropertyNames(JsValueRef object) {
//  JsValueRef propertyNames{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetOwnPropertyNames(object, &propertyNames));
//  return propertyNames;
//}
//
///*static*/ void NapiApi::SetProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef value) {
//  NapiVerifyJsErrorElseThrow(JsSetProperty(object, propertyId, value, /*useStringRules:*/ true));
//}
//
///*static*/ bool NapiApi::HasProperty(JsValueRef object, JsPropertyIdRef propertyId) {
//  bool hasProperty{false};
//  NapiVerifyJsErrorElseThrow(JsHasProperty(object, propertyId, &hasProperty));
//  return hasProperty;
//}
//
///*static*/ bool
// NapiApi::DefineProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef propertyDescriptor) {
//  bool isSucceeded{false};
//  NapiVerifyJsErrorElseThrow(JsDefineProperty(object, propertyId, propertyDescriptor, &isSucceeded));
//  return isSucceeded;
//}
//
///*static*/ JsValueRef NapiApi::GetIndexedProperty(JsValueRef object, int32_t index) {
//  JsValueRef result{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsGetIndexedProperty(object, IntToNumber(index), &result));
//  return result;
//}
//
///*static*/ void NapiApi::SetIndexedProperty(JsValueRef object, int32_t index, JsValueRef value) {
//  NapiVerifyJsErrorElseThrow(JsSetIndexedProperty(object, IntToNumber(index), value));
//}
//
///*static*/ bool NapiApi::StrictEquals(JsValueRef object1, JsValueRef object2) {
//  bool result{false};
//  NapiVerifyJsErrorElseThrow(JsStrictEquals(object1, object2, &result));
//  return result;
//}
//
///*static*/ void *NapiApi::GetExternalData(JsValueRef object) {
//  void *data{nullptr};
//  NapiVerifyJsErrorElseThrow(JsGetExternalData(object, &data));
//  return data;
//}
//
///*static*/ JsValueRef NapiApi::CreateArray(size_t length) {
//  JsValueRef result{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateArray(static_cast<unsigned int>(length), &result));
//  return result;
//}
//
///*static*/ JsValueRef NapiApi::CreateArrayBuffer(size_t byteLength) {
//  JsValueRef result{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateArrayBuffer(static_cast<unsigned int>(byteLength), &result));
//  return result;
//}
//
///*static*/ NapiApi::Span<std::byte> NapiApi::GetArrayBufferStorage(JsValueRef arrayBuffer) {
//  BYTE *buffer{nullptr};
//  unsigned int bufferLength{0};
//  NapiVerifyJsErrorElseThrow(JsGetArrayBufferStorage(arrayBuffer, &buffer, &bufferLength));
//  return {reinterpret_cast<std::byte *>(buffer), bufferLength};
//}
//
// napi_value NapiApi::CallFunction(napi_value thisArg, napi_value function, Span<napi_value> args) {
//  napi_value result{};
//  NapiVerifyJsErrorElseThrow(napi_call_function(m_env, thisArg, function, args.size(), args.begin(), &result));
//  return result;
//}
//
// napi_value NapiApi::ConstructObject(napi_value constructor, Span<napi_value> args) {
//  napi_value result{};
//  NapiVerifyJsErrorElseThrow(napi_new_instance(m_env, constructor, args.size(), args.begin(), &result));
//  return result;
//}
//
///*static*/ JsValueRef
// NapiApi::CreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState) {
//  JsValueRef function{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateNamedFunction(name, nativeFunction, callbackState, &function));
//  return function;
//}
//
///*static*/ bool NapiApi::SetException(JsValueRef error) noexcept {
//  // This method must not throw. We return false in case of error.
//  return JsSetException(error) == JsNoError;
//}
//
///*static*/ bool NapiApi::SetException(std::string_view message) noexcept try {
//  JsValueRef error{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateError(PointerToString(message), &error));
//  return SetException(error);
//} catch (...) {
//  // This method must not throw. We return false in case of error.
//  return false;
//}
//
///*static*/ bool NapiApi::SetException(std::wstring_view message) noexcept try {
//  JsValueRef error{JS_INVALID_REFERENCE};
//  NapiVerifyJsErrorElseThrow(JsCreateError(PointerToString(message), &error));
//  return SetException(error);
//} catch (...) {
//  // This method must not throw. We return false in case of error.
//  return false;
//}

} // namespace react::jsi

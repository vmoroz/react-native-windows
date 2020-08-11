// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ChakraContext.h"
#include <sstream>
#include <utility>
#include "Unicode.h"

namespace Microsoft::JSI {

//=============================================================================
// ChakraContext::JsRefHolder implementation
//=============================================================================

ChakraContext::JsRefHolder::JsRefHolder(JsRef ref) noexcept : m_ref{ref} {
  if (m_ref) {
    AddRef(m_ref);
  }
}

ChakraContext::JsRefHolder::JsRefHolder(JsRefHolder const &other) noexcept : m_ref{other.m_ref} {
  if (m_ref) {
    AddRef(m_ref);
  }
}

ChakraContext::JsRefHolder::JsRefHolder(JsRefHolder &&other) noexcept
    : m_ref{std::exchange(other.m_ref, JS_INVALID_REFERENCE)} {}

ChakraContext::JsRefHolder &ChakraContext::JsRefHolder::operator=(JsRefHolder const &other) noexcept {
  if (this != &other) {
    JsRefHolder temp{std::move(*this)};
    m_ref = other.m_ref;
    if (m_ref) {
      AddRef(m_ref);
    }
  }

  return *this;
}

ChakraContext::JsRefHolder &ChakraContext::JsRefHolder::operator=(JsRefHolder &&other) noexcept {
  if (this != &other) {
    JsRefHolder temp{std::move(*this)};
    m_ref = std::exchange(other.m_ref, JS_INVALID_REFERENCE);
  }

  return *this;
}

ChakraContext::JsRefHolder::~JsRefHolder() noexcept {
  if (m_ref) {
    // Clear m_ref before calling JsRelease on it to make sure that we always hold a valid m_ref.
    Release(std::exchange(m_ref, JS_INVALID_REFERENCE));
  }
}

//=============================================================================
// ChakraContext implementation
//=============================================================================

[[noreturn]] void ChakraContext::ThrowJsException(JsErrorCode errorCode, JsValueRef /*exception*/) {
  std::ostringstream errorString;
  errorString << "A call to Chakra API returned error code 0x" << std::hex << errorCode << '.';
  throw std::exception(errorString.str().c_str());
}

[[noreturn]] void ChakraContext::ThrowNativeException(char const *errorMessage) {
  throw std::exception(errorMessage);
}

void ChakraContext::VerifyJsErrorElseThrow(JsErrorCode errorCode) {
  if (errorCode != JsNoError) {
    JsValueRef exception{JS_INVALID_REFERENCE};
    ChakraVerifyElseCrash(JsGetAndClearException(&exception) == JsNoError, "Cannot retrieve JS exception.");
    ThrowJsException(errorCode, exception);
  }
}

void ChakraContext::VerifyElseThrow(bool condition, char const *errorMessage) {
  if (!condition) {
    ThrowNativeException(errorMessage);
  }
}

uint32_t ChakraContext::AddRef(JsRef ref) {
  uint32_t result;
  VerifyJsErrorElseThrow(JsAddRef(ref, &result));
  return result;
}

uint32_t ChakraContext::Release(JsRef ref) {
  uint32_t result;
  VerifyJsErrorElseThrow(JsRelease(ref, &result));
  return result;
}

JsContextRef ChakraContext::CreateContext(JsRuntimeHandle runtime) {
  JsContextRef context{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCreateContext(runtime, &context));
  return context;
}

JsPropertyIdRef ChakraContext::GetPropertyIdFromName(wchar_t const *name) {
  JsPropertyIdRef propertyId;
  VerifyJsErrorElseThrow(JsGetPropertyIdFromName(name, &propertyId));
  return propertyId;
}

JsPropertyIdRef ChakraContext::GetPropertyIdFromName(std::string_view name) {
  VerifyElseThrow(name.data(), "Property name cannot be a nullptr.");

  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  // We use a #ifdef here because we can avoid a UTF-8 to UTF-16 conversion
  // using ChakraCore's JsCreatePropertyId API.
#ifdef CHAKRACORE
  VerifyJsErrorElseThrow(JsCreatePropertyId(name.data(), name.length(), &propertyId));
#else
  std::wstring utf16 = Common::Unicode::Utf8ToUtf16(name.data(), name.length());
  VerifyJsErrorElseThrow(JsGetPropertyIdFromName(utf16.data(), &propertyId));
#endif
  return propertyId;
}

wchar_t const *ChakraContext::GetPropertyNameFromId(JsPropertyIdRef propertyId) {
  VerifyElseThrow(
      GetPropertyIdType(propertyId) == JsPropertyIdTypeString,
      "It is illegal to retrieve the name associated with a property symbol.");

  wchar_t const *name;
  VerifyJsErrorElseThrow(JsGetPropertyNameFromId(propertyId, &name));
  return name;
}

JsValueRef ChakraContext::GetPropertyStringFromId(JsPropertyIdRef propertyId) {
  return PointerToString(GetPropertyNameFromId(propertyId));
}

JsValueRef ChakraContext::GetSymbolFromPropertyId(JsPropertyIdRef propertyId) {
  VerifyElseThrow(
      GetPropertyIdType(propertyId) == JsPropertyIdTypeSymbol,
      "It is illegal to retrieve the symbol associated with a property name.");

  JsValueRef symbol{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetSymbolFromPropertyId(propertyId, &symbol));
  return symbol;
}

JsPropertyIdType ChakraContext::GetPropertyIdType(JsPropertyIdRef propertyId) {
  JsPropertyIdType type;
  VerifyJsErrorElseThrow(JsGetPropertyIdType(propertyId, &type));
  return type;
}

JsPropertyIdRef ChakraContext::GetPropertyIdFromSymbol(JsValueRef symbol) {
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetPropertyIdFromSymbol(symbol, &propertyId));
  return propertyId;
}

JsPropertyIdRef ChakraContext::GetPropertyIdFromSymbol(std::wstring_view symbolDescription) {
  JsPropertyIdRef propertyId;
  VerifyJsErrorElseThrow(JsGetPropertyIdFromSymbol(CreateSymbol(symbolDescription), &propertyId));
  return propertyId;
}

JsValueRef ChakraContext::CreateSymbol(JsValueRef symbolDescription) {
  JsValueRef symbol;
  VerifyJsErrorElseThrow(JsCreateSymbol(symbolDescription, &symbol));
  return symbol;
}

JsValueRef ChakraContext::CreateSymbol(std::wstring_view symbolDescription) {
  return CreateSymbol(PointerToString(symbolDescription));
}

JsValueRef ChakraContext::GetUndefinedValue() {
  JsValueRef undefinedValue;
  VerifyJsErrorElseThrow(JsGetUndefinedValue(&undefinedValue));
  return undefinedValue;
}

JsValueRef ChakraContext::GetNullValue() {
  JsValueRef nullValue;
  VerifyJsErrorElseThrow(JsGetNullValue(&nullValue));
  return nullValue;
}

JsValueRef ChakraContext::BoolToBoolean(bool value) {
  JsValueRef booleanValue;
  VerifyJsErrorElseThrow(JsBoolToBoolean(value, &booleanValue));
  return booleanValue;
}

bool ChakraContext::BooleanToBool(JsValueRef value) {
  bool boolValue;
  VerifyJsErrorElseThrow(JsBooleanToBool(value, &boolValue));
  return boolValue;
}

JsValueType ChakraContext::GetValueType(JsValueRef value) {
  JsValueType type;
  VerifyJsErrorElseThrow(JsGetValueType(value, &type));
  return type;
}

JsValueRef ChakraContext::DoubleToNumber(double value) {
  JsValueRef result;
  VerifyJsErrorElseThrow(JsDoubleToNumber(value, &result));
  return result;
}

JsValueRef ChakraContext::IntToNumber(int32_t value) {
  JsValueRef numberValue;
  VerifyJsErrorElseThrow(JsIntToNumber(value, &numberValue));
  return numberValue;
}

double ChakraContext::NumberToDouble(JsValueRef value) {
  double doubleValue;
  VerifyJsErrorElseThrow(JsNumberToDouble(value, &doubleValue));
  return doubleValue;
}

int32_t ChakraContext::NumberToInt(JsValueRef value) {
  int intValue;
  VerifyJsErrorElseThrow(JsNumberToInt(value, &intValue));
  return intValue;
}

JsValueRef ChakraContext::PointerToString(std::wstring_view value) {
  VerifyElseThrow(value.data(), "Cannot convert a nullptr to a JS string.");

  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsPointerToString(value.data(), value.size(), &result));
  return result;
}

JsValueRef ChakraContext::PointerToString(std::string_view value) {
  VerifyElseThrow(value.data(), "Cannot convert a nullptr to a JS string.");

  // ChakraCore  API helps to reduce cost of UTF-8 to UTF-16 conversion.
#ifdef CHAKRACORE
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCreateString(value.data(), value.length(), &result));
  return result;
#else
  return PointerToString(Common::Unicode::Utf8ToUtf16(value));
#endif
}

std::wstring_view ChakraContext::StringToPointer(JsValueRef string) {
  wchar_t const *utf16{nullptr};
  size_t length{0};
  VerifyJsErrorElseThrow(JsStringToPointer(string, &utf16, &length));
  return {utf16, length};
}

std::string ChakraContext::StringToStdString(JsValueRef string) {
  VerifyElseThrow(GetValueType(string) == JsString, "Cannot convert a non JS string ChakraObjectRef to a std::string.");

  // We use a #ifdef here because we can avoid a UTF-8 to UTF-16 conversion
  // using ChakraCore's JsCopyString API.
#ifdef CHAKRACORE
  size_t length = 0;
  VerifyJsErrorElseThrow(JsCopyString(jsString, nullptr, 0, &length));

  std::string result(length, 'a');
  VerifyJsErrorElseThrow(JsCopyString(jsString, result.data(), result.length(), &length));

  VerifyElseThrow(length == result.length(), "Failed to convert a JS string to a std::string.");
  return result;
#else
  return Common::Unicode::Utf16ToUtf8(StringToPointer(string));
#endif
}

JsValueRef ChakraContext::ConvertValueToString(JsValueRef value) {
  JsValueRef stringValue{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsConvertValueToString(value, &stringValue));
  return stringValue;
}

JsValueRef ChakraContext::GetGlobalObject() {
  JsValueRef globalObject;
  VerifyJsErrorElseThrow(JsGetGlobalObject(&globalObject));
  return globalObject;
}

JsValueRef ChakraContext::CreateObject() {
  JsValueRef object;
  VerifyJsErrorElseThrow(JsCreateObject(&object));
  return object;
}

JsValueRef ChakraContext::CreateExternalObject(void *data, JsFinalizeCallback finalizeCallback) {
  JsValueRef object;
  VerifyJsErrorElseThrow(JsCreateExternalObject(data, finalizeCallback, &object));
  return object;
}

JsValueRef ChakraContext::GetPrototype(JsValueRef object) {
  JsValueRef prototype{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetPrototype(object, &prototype));
  return prototype;
}

bool ChakraContext::InstanceOf(JsValueRef object, JsValueRef constructor) {
  bool result;
  VerifyJsErrorElseThrow(JsInstanceOf(object, constructor, &result));
  return result;
}

JsValueRef ChakraContext::GetProperty(JsValueRef object, JsPropertyIdRef propertyId) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetProperty(object, propertyId, &result));
  return result;
}

JsValueRef ChakraContext::GetOwnPropertyNames(JsValueRef object) {
  JsValueRef propertyNames{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetOwnPropertyNames(object, &propertyNames));
  return propertyNames;
}

void ChakraContext::SetProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef value) {
  VerifyJsErrorElseThrow(JsSetProperty(object, propertyId, value, /*useStringRules:*/true));
}

bool ChakraContext::HasProperty(JsValueRef object, JsPropertyIdRef propertyId) {
  bool hasProperty{false};
  VerifyJsErrorElseThrow(JsHasProperty(object, propertyId, &hasProperty));
  return hasProperty;
}

bool ChakraContext::DefineProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef propertyDescriptor) {
  bool isSucceeded;
  VerifyJsErrorElseThrow(JsDefineProperty(object, propertyId, propertyDescriptor, &isSucceeded));
  return isSucceeded;
}

JsValueRef ChakraContext::GetIndexedProperty(JsValueRef object, int32_t index) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetIndexedProperty(object, IntToNumber(index), &result));
  return result;
}

void ChakraContext::SetIndexedProperty(JsValueRef object, int32_t index, JsValueRef value) {
  VerifyJsErrorElseThrow(JsSetIndexedProperty(object, IntToNumber(index), value));
}

bool ChakraContext::StrictEquals(JsValueRef object1, JsValueRef object2) {
  bool result = false;
  VerifyJsErrorElseThrow(JsStrictEquals(object1, object2, &result));
  return result;
}

void *ChakraContext::GetExternalData(JsValueRef object) {
  void *data{nullptr};
  VerifyJsErrorElseThrow(JsGetExternalData(object, &data));
  return data;
}

JsValueRef ChakraContext::CreateArray(size_t length) {
  JsValueRef result;
  VerifyJsErrorElseThrow(JsCreateArray(static_cast<unsigned int>(length), &result));
  return result;
}

JsValueRef ChakraContext::CreateArrayBuffer(size_t byteLength) {
  JsValueRef result;
  VerifyJsErrorElseThrow(JsCreateArrayBuffer(static_cast<unsigned int>(byteLength), &result));
  return result;
}

JsValueRef ChakraContext::CallFunction(JsValueRef function, JsValueRefSpan args) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCallFunction(function, args.begin(), args.size(), &result));
  return result;
}

JsValueRef ChakraContext::ConstructObject(JsValueRef function, JsValueRefSpan args) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsConstructObject(function, args.begin(), args.size(), &result));
  return result;
}

JsValueRef ChakraContext::CreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState) {
  JsValueRef function;
  VerifyJsErrorElseThrow(JsCreateNamedFunction(name, nativeFunction, callbackState, &function));
  return function;
}

bool ChakraContext::SetException(JsValueRef error) noexcept {
  // This method must not throw. We return false in case of error.
  return JsSetException(error) == JsNoError;
}

bool ChakraContext::SetException(std::string_view message) noexcept try {
  JsValueRef error{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCreateError(PointerToString(message), &error));
  return SetException(error);
} catch (...) {
  // This method must not throw. We return false in case of error.
  return false;
}

bool ChakraContext::SetException(std::wstring_view message) noexcept try {
  JsValueRef error{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCreateError(PointerToString(message), &error));
  return SetException(error);
} catch (...) {
  // This method must not throw. We return false in case of error.
  return false;
}

} // namespace Microsoft::JSI

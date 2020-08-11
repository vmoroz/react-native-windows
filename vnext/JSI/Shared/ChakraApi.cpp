// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ChakraApi.h"
#include <sstream>
#include <utility>
#include "Unicode.h"
#include "Utilities.h"

namespace Microsoft::JSI {

//=============================================================================
// ChakraJsRefHolder implementation
//=============================================================================

ChakraJsRefHolder::ChakraJsRefHolder(ChakraApi *api, JsRef ref) noexcept : m_api{api}, m_ref{ref} {
  if (m_ref) {
    ChakraVerifyElseCrash(m_api, "api must not be null");
    m_api->AddRef(m_ref);
  }
}

ChakraJsRefHolder::ChakraJsRefHolder(ChakraJsRefHolder const &other) noexcept : m_api{other.m_api}, m_ref{other.m_ref} {
  if (m_ref) {
    m_api->AddRef(m_ref);
  }
}

ChakraJsRefHolder::ChakraJsRefHolder(ChakraJsRefHolder &&other) noexcept
    : m_api{std::exchange(other.m_api, nullptr)}, m_ref{std::exchange(other.m_ref, JS_INVALID_REFERENCE)} {}

ChakraJsRefHolder &ChakraJsRefHolder::operator=(ChakraJsRefHolder const &other) noexcept {
  if (this != &other) {
    ChakraJsRefHolder temp{std::move(*this)};
    m_api = other.m_api;
    m_ref = other.m_ref;
    if (m_ref) {
      m_api->AddRef(m_ref);
    }
  }

  return *this;
}

ChakraJsRefHolder &ChakraJsRefHolder::operator=(ChakraJsRefHolder &&other) noexcept {
  if (this != &other) {
    ChakraJsRefHolder temp{std::move(*this)};
    m_api = std::exchange(other.m_api, nullptr);
    m_ref = std::exchange(other.m_ref, JS_INVALID_REFERENCE);
  }

  return *this;
}

ChakraJsRefHolder::~ChakraJsRefHolder() noexcept {
  if (m_ref) {
    // Clear m_ref before calling JsRelease on it to make sure that we always hold a valid m_ref.
    m_api->Release(std::exchange(m_ref, JS_INVALID_REFERENCE));
    m_api = nullptr;
  }
}

//=============================================================================
// ChakraApi implementation
//=============================================================================

[[noreturn]] void ChakraApi::ThrowJsException(JsErrorCode errorCode, JsValueRef /*exception*/) {
  std::ostringstream errorString;
  errorString << "A call to Chakra API returned error code 0x" << std::hex << errorCode << '.';
  throw std::exception(errorString.str().c_str());
}

void ChakraApi::VerifyJsErrorElseThrow(JsErrorCode errorCode) {
  if (errorCode != JsNoError) {
    JsValueRef exception{JS_INVALID_REFERENCE};
    ChakraVerifyElseCrash(JsGetAndClearException(&exception));
    ThrowJsException(errorCode, exception);
  }
}

uint32_t ChakraApi::AddRef(JsRef ref) {
  uint32_t result;
  VerifyJsErrorElseThrow(JsAddRef(ref, &result));
  return result;
}

uint32_t ChakraApi::Release(JsRef ref) {
  uint32_t result;
  VerifyJsErrorElseThrow(JsRelease(ref, &result));
  return result;
}

JsContextRef ChakraApi::CreateContext(JsRuntimeHandle runtime) {
  JsContextRef context{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCreateContext(runtime, &context));
  return context;
}

JsPropertyIdRef ChakraApi::GetPropertyIdFromName(wchar_t const *name) {
  static_assert(false, "not implemented yet");
}

JsPropertyIdRef ChakraApi::GetPropertyIdFromName(std::string_view name) {
  static_assert(false, "not implemented yet");
}

wchar_t const *ChakraApi::GetPropertyNameFromId(JsPropertyIdRef propertyId) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetPropertyStringFromId(JsPropertyIdRef propertyId) {
  static_assert(false, "not implemented yet");
}

JsPropertyIdType ChakraApi::GetPropertyIdType(JsPropertyIdRef propertyId) {
  static_assert(false, "not implemented yet");
}

JsPropertyIdRef ChakraApi::GetPropertyIdFromSymbol(JsValueRef symbol) {
  static_assert(false, "not implemented yet");
}

JsPropertyIdRef ChakraApi::GetPropertyIdFromSymbol(std::wstring_view symbolDescription) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CreateSymbol(JsValueRef symbolDescription) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CreateSymbol(std::wstring_view symbolDescription) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetUndefinedValue() {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetNullValue() {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::BoolToBoolean(bool value) {
  static_assert(false, "not implemented yet");
}

bool ChakraApi::BooleanToBool(JsValueRef value) {
  static_assert(false, "not implemented yet");
}

JsValueType ChakraApi::GetValueType(JsValueRef value) {
  JsValueType type;
  VerifyJsErrorElseThrow(JsGetValueType(value, &type));
  return type;
}

JsValueRef ChakraApi::DoubleToNumber(double value) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::IntToNumber(int32_t value) {
  static_assert(false, "not implemented yet");
}

double ChakraApi::NumberToDouble(JsValueRef value) {
  static_assert(false, "not implemented yet");
}

int32_t ChakraApi::NumberToInt(JsValueRef value) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::PointerToString(std::wstring_view value) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::PointerToString(std::string_view value) {
  static_assert(false, "not implemented yet");
}

std::wstring_view ChakraApi::StringToPointer(JsValueRef string) {
  static_assert(false, "not implemented yet");
}

std::string ChakraApi::StringToStdString(JsValueRef string) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::ConvertValueToString(JsValueRef value) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetGlobalObject() {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CreateObject() {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CreateExternalObject(void *data, JsFinalizeCallback finalizeCallback) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetPrototype(JsValueRef object) {
  static_assert(false, "not implemented yet");
}

bool ChakraApi::InstanceOf(JsValueRef object, JsValueRef constructor) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetProperty(JsValueRef object, JsPropertyIdRef propertyId) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetOwnPropertyNames(JsValueRef object) {
  static_assert(false, "not implemented yet");
}

void ChakraApi::SetProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef value) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::HasProperty(JsValueRef object, JsPropertyIdRef propertyId) {
  static_assert(false, "not implemented yet");
}

bool ChakraApi::DefineProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef propertyDescriptor) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::GetIndexedProperty(JsValueRef object, int32_t index) {
  static_assert(false, "not implemented yet");
}

void ChakraApi::SetIndexedProperty(JsValueRef object, int32_t index, JsValueRef value) {
  static_assert(false, "not implemented yet");
}

bool ChakraApi::StrictEquals(JsValueRef jsValue1, JsValueRef jsValue2) {
  static_assert(false, "not implemented yet");
}

void *ChakraApi::GetExternalData(JsValueRef object) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CreateArray(size_t length) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CreateArrayBuffer(size_t byteLength) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CallFunction(JsValueRef function, JsValueRefSpan args) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::ConstructObject(JsValueRef function, JsValueRefSpan args) {
  static_assert(false, "not implemented yet");
}

JsValueRef ChakraApi::CreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState) {
  static_assert(false, "not implemented yet");
}

bool ChakraApi::SetException(JsValueRef error) noexcept {
  static_assert(false, "not implemented yet");
}

bool ChakraApi::SetException(std::string_view message) noexcept {
  static_assert(false, "not implemented yet");
}

bool ChakraApi::SetException(std::wstring_view message) noexcept {
  static_assert(false, "not implemented yet");
}

void VerifyJsErrorElseThrow(JsErrorCode error) {
  if (error != JsNoError) {
    std::ostringstream errorString;
    errorString << "A call to Chakra(Core) API returned error code 0x" << std::hex << error << '.';
    throw facebook::jsi::JSINativeException(errorString.str());
  }
}

JsPropertyIdType GetPropertyIdType(JsPropertyIdRef propertyId) {
  JsPropertyIdType type;
  VerifyJsErrorElseThrow(JsGetPropertyIdType(propertyId, &type));
  return type;
}

JsValueRef GetPropertySymbol(JsPropertyIdRef propertyId) {
  if (GetPropertyIdType(propertyId) != JsPropertyIdTypeSymbol) {
    throw facebook::jsi::JSINativeException("It is illegal to retrieve the symbol associated with a property name.");
  }
  JsValueRef symbol = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsGetSymbolFromPropertyId(propertyId, &symbol));
  return symbol;
}

JsPropertyIdRef GetPropertyId(std::string_view utf8) {
  if (!utf8.data()) {
    throw facebook::jsi::JSINativeException("Property name cannot be a nullptr.");
  }

  // We use a #ifdef here because we can avoid a UTF-8 to UTF-16 conversion
  // using ChakraCore's JsCreatePropertyId API.
#ifdef CHAKRACORE
  JsPropertyIdRef id = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsCreatePropertyId(utf8.data(), utf8.length(), &id));
  return id;

#else
  std::wstring utf16 = Common::Unicode::Utf8ToUtf16(utf8.data(), utf8.length());
  return GetPropertyId(utf16);
#endif
}

JsPropertyIdRef GetPropertyId(std::wstring_view name) {
  JsPropertyIdRef propertyId;
  VerifyJsErrorElseThrow(JsGetPropertyIdFromName(name.data(), &propertyId));
  return propertyId;
}

wchar_t const *GetPropertyNameFromId(JsPropertyIdRef propertyId) {
  wchar_t const *name;
  VerifyJsErrorElseThrow(JsGetPropertyNameFromId(propertyId, &name));
  return name;
}

JsValueRef PropertyIdToString(JsPropertyIdRef propertyId) {
  return CreateString(GetPropertyNameFromId(propertyId));
}

JsPropertyIdRef GetSymbolPropertyId(std::wstring_view symbolDescription) {
  JsPropertyIdRef propertyId;
  VerifyJsErrorElseThrow(JsGetPropertyIdFromSymbol(CreateSymbol(symbolDescription), &propertyId));
  return propertyId;
}

JsValueRef CreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState) {
  JsValueRef function;
  VerifyJsErrorElseThrow(JsCreateNamedFunction(name, nativeFunction, callbackState, &function));
  return function;
}

JsValueRef CreateSymbol(std::wstring_view symbolDescription) {
  return CreateSymbol(CreateString(symbolDescription));
}

JsValueRef CreateSymbol(JsValueRef symbolDescription) {
  JsValueRef symbol;
  VerifyJsErrorElseThrow(JsCreateSymbol(symbolDescription, &symbol));
  return symbol;
}

JsValueRef CreateString(std::wstring_view strView) {
  JsValueRef str;
  VerifyJsErrorElseThrow(JsPointerToString(strView.data(), strView.size(), &str));
  return str;
}

bool DefineProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef propertyDescriptor) {
  bool isSucceeded;
  VerifyJsErrorElseThrow(JsDefineProperty(object, propertyId, propertyDescriptor, &isSucceeded));
  return isSucceeded;
}

JsValueRef IntToNumber(int32_t value) {
  JsValueRef numberValue;
  VerifyJsErrorElseThrow(JsIntToNumber(value, &numberValue));
  return numberValue;
}

JsValueRef CreateObject() {
  JsValueRef object;
  VerifyJsErrorElseThrow(JsCreateObject(&object));
  return object;
}

JsValueRef CreateExternalObject(void *data, JsFinalizeCallback finalizeCallback) {
  JsValueRef object;
  VerifyJsErrorElseThrow(JsCreateExternalObject(data, finalizeCallback, &object));
  return object;
}

void *GetExternalData(JsValueRef object) {
  void *data;
  VerifyJsErrorElseThrow(JsGetExternalData(object, &data));
  return data;
}

JsValueRef BoolToBoolean(bool value) {
  JsValueRef booleanValue;
  VerifyJsErrorElseThrow(JsBoolToBoolean(value, &booleanValue));
  return booleanValue;
}

std::wstring_view StringToPointer(JsValueRef value) {
  wchar_t const *utf16{nullptr};
  size_t length{0};
  VerifyJsErrorElseThrow(JsStringToPointer(value, &utf16, &length));
  return {utf16, length};
}

JsValueRef PointerToString(std::wstring_view value) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsPointerToString(value.data(), value.size(), &result));
  return result;
}

JsValueRef PointerToString(std::string_view value) {
  return PointerToString(Common::Unicode::Utf8ToUtf16(value));
}

JsPropertyIdRef GetPropertyIdFromName(wchar_t const *name) {
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetPropertyIdFromName(name, &propertyId));
  return propertyId;
}

JsPropertyIdRef GetPropertyIdFromSymbol(JsValueRef symbol) {
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetPropertyIdFromSymbol(symbol, &propertyId));
  return propertyId;
}

JsValueRef GetGlobalObject() {
  JsValueRef globalObject;
  VerifyJsErrorElseThrow(JsGetGlobalObject(&globalObject));
  return globalObject;
}

JsValueRef GetNullValue() {
  JsValueRef nullValue;
  VerifyJsErrorElseThrow(JsGetNullValue(&nullValue));
  return nullValue;
}

JsValueRef GetUndefinedValue() {
  JsValueRef undefinedValue;
  VerifyJsErrorElseThrow(JsGetUndefinedValue(&undefinedValue));
  return undefinedValue;
}

double NumberToDouble(JsValueRef value) {
  double doubleValue;
  VerifyJsErrorElseThrow(JsNumberToDouble(value, &doubleValue));
  return doubleValue;
}

JsValueRef DoubleToNumber(double value) {
  JsValueRef result;
  VerifyJsErrorElseThrow(JsDoubleToNumber(value, &result));
  return result;
}

int NumberToInt(JsValueRef value) {
  int intValue;
  VerifyJsErrorElseThrow(JsNumberToInt(value, &intValue));
  return intValue;
}

bool BooleanToBool(JsValueRef value) {
  bool boolValue;
  VerifyJsErrorElseThrow(JsBooleanToBool(value, &boolValue));
  return boolValue;
}

JsValueRef CreateArray(size_t length) {
  JsValueRef result;
  VerifyJsErrorElseThrow(JsCreateArray(static_cast<unsigned int>(length), &result));
  return result;
}

JsValueRef GetOwnPropertyNames(JsValueRef object) {
  JsValueRef propertyNames{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetOwnPropertyNames(object, &propertyNames));
  return propertyNames;
}

JsValueRef GetIndexedProperty(JsValueRef object, size_t index) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetIndexedProperty(object, IntToNumber(static_cast<int32_t>(index)), &result));
  return result;
}

void SetIndexedProperty(JsValueRef object, size_t index, JsValueRef value) {
  VerifyJsErrorElseThrow(JsSetIndexedProperty(object, IntToNumber(static_cast<int32_t>(index)), value));
}

JsValueRef GetPrototype(JsValueRef object) {
  JsValueRef prototype{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetPrototype(object, &prototype));
  return prototype;
}

void SetException(std::string_view message) noexcept try {
  JsValueRef error{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCreateError(ToJsString(message), &error));
  SetException(error);
} catch (...) {
  // Ignore exceptions. We cannot rethrow them.
}

void SetException(std::wstring_view message) noexcept try {
  JsValueRef error{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsCreateError(ToJsString(message), &error));
  SetException(error);
} catch (...) {
  // Ignore exceptions. We cannot rethrow them.
}

void SetException(JsValueRef error) noexcept {
  // Ignore returned error code. We cannot rethrow it.
  JsSetException(error);
}

std::string ToStdString(JsValueRef jsString) {
  if (GetValueType(jsString) != JsString) {
    throw facebook::jsi::JSINativeException("Cannot convert a non JS string ChakraObjectRef to a std::string.");
  }

  // We use a #ifdef here because we can avoid a UTF-8 to UTF-16 conversion
  // using ChakraCore's JsCopyString API.
#ifdef CHAKRACORE
  size_t length = 0;
  VerifyJsErrorElseThrow(JsCopyString(jsString, nullptr, 0, &length));

  std::string result(length, 'a');
  VerifyJsErrorElseThrow(JsCopyString(jsString, result.data(), result.length(), &length));

  if (length != result.length()) {
    throw facebook::jsi::JSINativeException("Failed to convert a JS string to a std::string.");
  }

  return result;
#else
  return Common::Unicode::Utf16ToUtf8(StringToPointer(jsString));
#endif
}

JsValueRef ToJsString(std::string_view utf8) {
  if (!utf8.data()) {
    throw facebook::jsi::JSINativeException("Cannot convert a nullptr to a JS string.");
  }

  // We use a #ifdef here because we can avoid a UTF-8 to UTF-16 conversion
  // using ChakraCore's JsCreateString API.
#ifdef CHAKRACORE
  JsValueRef result = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsCreateString(utf8.data(), utf8.length(), &result));
  return result;

#else
  std::wstring utf16 = Common::Unicode::Utf8ToUtf16(utf8.data(), utf8.length());
  return ToJsString(std::wstring_view{utf16.c_str(), utf16.length()});
#endif
}

JsValueRef ToJsString(std::wstring_view utf16) {
  if (!utf16.data()) {
    throw facebook::jsi::JSINativeException("Cannot convert a nullptr to a JS string.");
  }

  JsValueRef result = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsPointerToString(utf16.data(), utf16.length(), &result));

  return result;
}

JsValueRef ToJsString(JsValueRef ref) {
  JsValueRef str = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsConvertValueToString(ref, &str));
  return str;
}

int ToInteger(JsValueRef jsNumber) {
  int result = 0;
  VerifyJsErrorElseThrow(JsNumberToInt(jsNumber, &result));
  return result;
}

JsValueRef ToJsNumber(int num) {
  JsValueRef result = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsIntToNumber(num, &result));
  return result;
}

JsValueRef ToJsArrayBuffer(const std::shared_ptr<const facebook::jsi::Buffer> &buffer) {
  if (!buffer) {
    throw facebook::jsi::JSINativeException("Cannot create an external JS ArrayBuffer without backing buffer.");
  }

  size_t size = buffer->size();

  if (size > UINT_MAX) {
    throw facebook::jsi::JSINativeException("The external backing buffer for a JS ArrayBuffer is too large.");
  }

  JsValueRef arrayBuffer = JS_INVALID_REFERENCE;
  auto bufferWrapper = std::make_unique<std::shared_ptr<const facebook::jsi::Buffer>>(buffer);

  // We allocate a copy of buffer on the heap, a shared_ptr which is deleted
  // when the JavaScript garbage collecotr releases the created external array
  // buffer. This ensures that buffer stays alive while the JavaScript engine is
  // using it.
  VerifyJsErrorElseThrow(JsCreateExternalArrayBuffer(
      Common::Utilities::CheckedReinterpretCast<char *>(const_cast<uint8_t *>(buffer->data())),
      static_cast<unsigned int>(size),
      [](void *bufferToDestroy) {
        // We wrap bufferToDestroy in a unique_ptr to avoid calling delete
        // explicitly.
        std::unique_ptr<std::shared_ptr<const facebook::jsi::Buffer>> wrapper{
            static_cast<std::shared_ptr<const facebook::jsi::Buffer> *>(bufferToDestroy)};
      },
      bufferWrapper.get(),
      &arrayBuffer));

  // We only call bufferWrapper.release() after JsCreateExternalObject succeeds.
  // Otherwise, when JsCreateExternalObject fails and an exception is thrown,
  // the shared_ptr that bufferWrapper used to own will be leaked.
  bufferWrapper.release();
  return arrayBuffer;
}

bool StrictEquals(JsValueRef jsValue1, JsValueRef jsValue2) {
  bool result = false;
  // Note that JsStrictEquals should only be used for JsValueRefs and not for
  // other types of JsRefs (e.g. JsPropertyIdRef, etc.).
  VerifyJsErrorElseThrow(JsStrictEquals(jsValue1, jsValue2, &result));
  return result;
}

void ThrowJsException(const std::string_view &message) {
  JsValueRef error = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsCreateError(ToJsString(message), &error));
  VerifyJsErrorElseThrow(JsSetException(error));
}

} // namespace Microsoft::JSI

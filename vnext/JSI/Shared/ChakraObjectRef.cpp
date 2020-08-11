// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ChakraObjectRef.h"

#include "Unicode.h"
#include "Utilities.h"

#include <memory>
#include <sstream>
#include <utility>

namespace Microsoft::JSI {

ChakraObjectRef::ChakraObjectRef(ChakraObjectRef const &other) noexcept : m_ref{other.m_ref} {
  if (m_ref) {
    VerifyChakraErrorElseThrow(JsAddRef(m_ref, nullptr));
  }
}

ChakraObjectRef::ChakraObjectRef(ChakraObjectRef &&other) noexcept : m_ref{std::exchange(other.m_ref, nullptr)} {}

ChakraObjectRef &ChakraObjectRef::operator=(ChakraObjectRef const &other) noexcept {
  if (this != &other) {
    ChakraObjectRef temp{std::move(*this)};
    m_ref = other.m_ref;
    if (m_ref) {
      VerifyChakraErrorElseThrow(JsAddRef(m_ref, nullptr));
    }
  }

  return *this;
}

ChakraObjectRef &ChakraObjectRef::operator=(ChakraObjectRef &&other) noexcept {
  if (this != &other) {
    ChakraObjectRef temp{std::move(*this)};
    m_ref = std::exchange(other.m_ref, JS_INVALID_REFERENCE);
  }

  return *this;
}

ChakraObjectRef::~ChakraObjectRef() noexcept {
  if (m_ref) {
    // Clear m_ref before calling JsRelease on it to make sure that we always hold a valid m_ref.
    VerifyChakraErrorElseThrow(JsRelease(std::exchange(m_ref, JS_INVALID_REFERENCE), nullptr));
  }
}

JsContextRef CreateContext(JsRuntimeHandle runtime) {
  JsContextRef context{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsCreateContext(runtime, &context));
  return context;
}

void VerifyChakraErrorElseThrow(JsErrorCode error) {
  if (error != JsNoError) {
    std::ostringstream errorString;
    errorString << "A call to Chakra(Core) API returned error code 0x" << std::hex << error << '.';
    throw facebook::jsi::JSINativeException(errorString.str());
  }
}

JsValueType GetValueType(JsValueRef value) {
  JsValueType type;
  VerifyChakraErrorElseThrow(JsGetValueType(value, &type));
  return type;
}

JsPropertyIdType GetPropertyIdType(JsPropertyIdRef propertyId) {
  JsPropertyIdType type;
  VerifyChakraErrorElseThrow(JsGetPropertyIdType(propertyId, &type));
  return type;
}

JsValueRef GetPropertySymbol(JsPropertyIdRef propertyId) {
  if (GetPropertyIdType(propertyId) != JsPropertyIdTypeSymbol) {
    throw facebook::jsi::JSINativeException("It is illegal to retrieve the symbol associated with a property name.");
  }
  JsValueRef symbol = JS_INVALID_REFERENCE;
  VerifyChakraErrorElseThrow(JsGetSymbolFromPropertyId(propertyId, &symbol));
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
  VerifyChakraErrorElseThrow(JsCreatePropertyId(utf8.data(), utf8.length(), &id));
  return id;

#else
  std::wstring utf16 = Common::Unicode::Utf8ToUtf16(utf8.data(), utf8.length());
  return GetPropertyId(utf16);
#endif
}

JsPropertyIdRef GetPropertyId(std::wstring_view name) {
  JsPropertyIdRef propertyId;
  VerifyChakraErrorElseThrow(JsGetPropertyIdFromName(name.data(), &propertyId));
  return propertyId;
}

wchar_t const *GetPropertyNameFromId(JsPropertyIdRef propertyId) {
  wchar_t const *name;
  VerifyChakraErrorElseThrow(JsGetPropertyNameFromId(propertyId, &name));
  return name;
}

JsValueRef PropertyIdToString(JsPropertyIdRef propertyId) {
  return CreateString(GetPropertyNameFromId(propertyId));
}

JsPropertyIdRef GetSymbolPropertyId(std::wstring_view symbolDescription) {
  JsPropertyIdRef propertyId;
  VerifyChakraErrorElseThrow(JsGetPropertyIdFromSymbol(CreateSymbol(symbolDescription), &propertyId));
  return propertyId;
}

JsValueRef CreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState) {
  JsValueRef function;
  VerifyChakraErrorElseThrow(JsCreateNamedFunction(name, nativeFunction, callbackState, &function));
  return function;
}

JsValueRef CreateSymbol(std::wstring_view symbolDescription) {
  return CreateSymbol(CreateString(symbolDescription));
}

JsValueRef CreateSymbol(JsValueRef symbolDescription) {
  JsValueRef symbol;
  VerifyChakraErrorElseThrow(JsCreateSymbol(symbolDescription, &symbol));
  return symbol;
}

JsValueRef CreateString(std::wstring_view strView) {
  JsValueRef str;
  VerifyChakraErrorElseThrow(JsPointerToString(strView.data(), strView.size(), &str));
  return str;
}

bool DefineProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef propertyDescriptor) {
  bool isSucceeded;
  VerifyChakraErrorElseThrow(JsDefineProperty(object, propertyId, propertyDescriptor, &isSucceeded));
  return isSucceeded;
}

JsValueRef IntToNumber(int32_t value) {
  JsValueRef numberValue;
  VerifyChakraErrorElseThrow(JsIntToNumber(value, &numberValue));
  return numberValue;
}

JsValueRef CreateObject() {
  JsValueRef object;
  VerifyChakraErrorElseThrow(JsCreateObject(&object));
  return object;
}

JsValueRef CreateExternalObject(void *data, JsFinalizeCallback finalizeCallback) {
  JsValueRef object;
  VerifyChakraErrorElseThrow(JsCreateExternalObject(data, finalizeCallback, &object));
  return object;
}

void *GetExternalData(JsValueRef object) {
  void *data;
  VerifyChakraErrorElseThrow(JsGetExternalData(object, &data));
  return data;
}

JsValueRef BoolToBoolean(bool value) {
  JsValueRef booleanValue;
  VerifyChakraErrorElseThrow(JsBoolToBoolean(value, &booleanValue));
  return booleanValue;
}

std::wstring_view StringToPointer(JsValueRef value) {
  wchar_t const *utf16{nullptr};
  size_t length{0};
  VerifyChakraErrorElseThrow(JsStringToPointer(value, &utf16, &length));
  return {utf16, length};
}

JsValueRef PointerToString(std::wstring_view value) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsPointerToString(value.data(), value.size(), &result));
  return result;
}

JsValueRef PointerToString(std::string_view value) {
  return PointerToString(Common::Unicode::Utf8ToUtf16(value));
}

JsPropertyIdRef GetPropertyIdFromName(wchar_t const *name) {
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsGetPropertyIdFromName(name, &propertyId));
  return propertyId;
}

JsPropertyIdRef GetPropertyIdFromSymbol(JsValueRef symbol) {
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsGetPropertyIdFromSymbol(symbol, &propertyId));
  return propertyId;
}

JsValueRef GetGlobalObject() {
  JsValueRef globalObject;
  VerifyChakraErrorElseThrow(JsGetGlobalObject(&globalObject));
  return globalObject;
}

JsValueRef GetNullValue() {
  JsValueRef nullValue;
  VerifyChakraErrorElseThrow(JsGetNullValue(&nullValue));
  return nullValue;
}

JsValueRef GetUndefinedValue() {
  JsValueRef undefinedValue;
  VerifyChakraErrorElseThrow(JsGetUndefinedValue(&undefinedValue));
  return undefinedValue;
}

double NumberToDouble(JsValueRef value) {
  double doubleValue;
  VerifyChakraErrorElseThrow(JsNumberToDouble(value, &doubleValue));
  return doubleValue;
}

JsValueRef DoubleToNumber(double value) {
  JsValueRef result;
  VerifyChakraErrorElseThrow(JsDoubleToNumber(value, &result));
  return result;
}

int NumberToInt(JsValueRef value) {
  int intValue;
  VerifyChakraErrorElseThrow(JsNumberToInt(value, &intValue));
  return intValue;
}

bool BooleanToBool(JsValueRef value) {
  bool boolValue;
  VerifyChakraErrorElseThrow(JsBooleanToBool(value, &boolValue));
  return boolValue;
}

JsValueRef CreateArray(size_t length) {
  JsValueRef result;
  VerifyChakraErrorElseThrow(JsCreateArray(static_cast<unsigned int>(length), &result));
  return result;
}

JsValueRef GetOwnPropertyNames(JsValueRef object) {
  JsValueRef propertyNames{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsGetOwnPropertyNames(object, &propertyNames));
  return propertyNames;
}

JsValueRef GetIndexedProperty(JsValueRef object, size_t index) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsGetIndexedProperty(object, IntToNumber(static_cast<int32_t>(index)), &result));
  return result;
}

void SetIndexedProperty(JsValueRef object, size_t index, JsValueRef value) {
  VerifyChakraErrorElseThrow(JsSetIndexedProperty(object, IntToNumber(static_cast<int32_t>(index)), value));
}

JsValueRef GetPrototype(JsValueRef object) {
  JsValueRef prototype{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsGetPrototype(object, &prototype));
  return prototype;
}

void SetException(std::string_view message) noexcept try {
  JsValueRef error{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsCreateError(ToJsString(message), &error));
  SetException(error);
} catch (...) {
  // Ignore exceptions. We cannot rethrow them.
}

void SetException(std::wstring_view message) noexcept try {
  JsValueRef error{JS_INVALID_REFERENCE};
  VerifyChakraErrorElseThrow(JsCreateError(ToJsString(message), &error));
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
  VerifyChakraErrorElseThrow(JsCopyString(jsString, nullptr, 0, &length));

  std::string result(length, 'a');
  VerifyChakraErrorElseThrow(JsCopyString(jsString, result.data(), result.length(), &length));

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
  VerifyChakraErrorElseThrow(JsCreateString(utf8.data(), utf8.length(), &result));
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
  VerifyChakraErrorElseThrow(JsPointerToString(utf16.data(), utf16.length(), &result));

  return result;
}

JsValueRef ToJsString(JsValueRef ref) {
  JsValueRef str = JS_INVALID_REFERENCE;
  VerifyChakraErrorElseThrow(JsConvertValueToString(ref, &str));
  return str;
}

int ToInteger(JsValueRef jsNumber) {
  int result = 0;
  VerifyChakraErrorElseThrow(JsNumberToInt(jsNumber, &result));
  return result;
}

JsValueRef ToJsNumber(int num) {
  JsValueRef result = JS_INVALID_REFERENCE;
  VerifyChakraErrorElseThrow(JsIntToNumber(num, &result));
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
  VerifyChakraErrorElseThrow(JsCreateExternalArrayBuffer(
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
  VerifyChakraErrorElseThrow(JsStrictEquals(jsValue1, jsValue2, &result));
  return result;
}

void ThrowJsException(const std::string_view &message) {
  JsValueRef error = JS_INVALID_REFERENCE;
  VerifyChakraErrorElseThrow(JsCreateError(ToJsString(message), &error));
  VerifyChakraErrorElseThrow(JsSetException(error));
}

} // namespace Microsoft::JSI

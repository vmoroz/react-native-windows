// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <jsi/jsi.h>

#ifdef CHAKRACORE
#include "ChakraCore.h"
#else
#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT
#endif
#include <jsrt.h>
#endif

#include <exception>
#include <string>
#include <string_view>

namespace Microsoft::JSI {

inline void VerifyChakraErrorElseCrash(JsErrorCode error) {
  if (error != JsNoError) {
    std::terminate();
  }
}

void VerifyChakraErrorElseThrow(JsErrorCode error);

/**
 * @brief A smart pointer for JsRefs.
 *
 * JsRefs are references to objects owned by the garbage collector and include
 * JsContextRef, JsValueRef, and JsPropertyIdRef, etc. ChakraObjectRef ensures
 * that JsAddRef and JsRelease are called upon initialization and invalidation,
 * respectively. It also allows users to implicitly convert it into a JsRef.
 */
struct ChakraObjectRef {
  ChakraObjectRef(std::nullptr_t = nullptr) noexcept {}
  explicit ChakraObjectRef(JsRef ref) noexcept : m_ref{ref} {
    if (m_ref) {
      VerifyChakraErrorElseThrow(JsAddRef(m_ref, nullptr));
    }
  }

  ChakraObjectRef(ChakraObjectRef const &other) noexcept;
  ChakraObjectRef(ChakraObjectRef &&other) noexcept;

  ChakraObjectRef &operator=(ChakraObjectRef const &other) noexcept;
  ChakraObjectRef &operator=(ChakraObjectRef &&other) noexcept;

  ~ChakraObjectRef() noexcept;

  operator JsRef() const noexcept {
    return m_ref;
  }

 private:
  JsRef m_ref{JS_INVALID_REFERENCE};
};

JsContextRef CreateContext(JsRuntimeHandle runtime);
JsPropertyIdRef GetSymbolPropertyId(std::wstring_view symbolDescription);
JsValueRef CreateSymbol(std::wstring_view symbolDescription);
JsValueRef CreateSymbol(JsValueRef symbolDescription);
JsValueRef CreateString(std::wstring_view strView);

/**
 * @param jsValue A ChakraObjectRef managing a JsValueRef.
 */
JsValueType GetValueType(JsValueRef value);

/**
 * @param jsPropId A ChakraObjectRef managing a JsPropertyIdRef.
 */
JsPropertyIdType GetPropertyIdType(JsPropertyIdRef propertyId);

/**
 * @param jsPropId A ChakraObjectRef managing a JsPropertyIdRef of type
 * JsPropertyIdTypeString.
 */
std::wstring GetPropertyName(JsPropertyIdRef propertyId);

/**
 * @param jsPropId A ChakraObjectRef managing a JsPropertyIdRef of type
 * JsPropertyIdTypeSymbol.
 *
 * @returns A ChakraObjectRef managing a JS Symbol.
 */
JsValueRef GetPropertySymbol(JsPropertyIdRef propertyId);

/**
 * @param utf8 A std::string_view to a UTF-8 encoded char array.
 *
 * @returns A ChakraObjectRef managing a JsPropertyIdRef.
 */
JsPropertyIdRef GetPropertyId(std::string_view utf8);

/**
 * @param utf16 A UTF-16 encoded std::wstring.
 *
 * @returns A ChakraObjectRef managing a JsPropertyIdRef.
 */
JsPropertyIdRef GetPropertyId(std::wstring_view utf16);

/**
 * @param jsString A ChakraObjectRef managing a JS string.
 *
 * @returns A std::string that is UTF-8 encoded.
 *
 * @remarks This function copies the JS string buffer into the returned
 * std::string. When using Chakra instead of ChakraCore, this function incurs
 * a UTF-16 to UTF-8 conversion.
 */
// TODO: Do we need it?
std::string ToStdString(JsValueRef jsString);

/**
 * @param jsString A ChakraObjectRef managing a JS string.
 *
 * @returns A std::wstring that is UTF-16 encoded.
 *
 * @remarks This functions copies the JS string buffer into the returned
 * std::wstring.
 */
// TODO: Do we need it?
std::wstring ToStdWstring(JsValueRef jsString);

/**
 * @param utf8 A std::string_view to a UTF-8 encoded char array.
 *
 * @returns A ChakraObjectRef managing a JS string.
 *
 * @remarks The content of utf8 is copied into JS engine owned memory. When
 * using Chakra instead of ChakraCore, this function incurs a UTF-8 to UTF-16
 * conversion.
 */
// TODO: Rename
JsValueRef ToJsString(std::string_view utf8);

/**
 * @param utf16 A std::wstring_view to a UTF-16 encoded wchar_t array.
 *
 * @returns A ChakraObjectRef managing a JS string.
 *
 * @remarks The content of utf16 is copied into JS engine owned memory.
 */
// TODO: Rename
JsValueRef ToJsString(std::wstring_view utf16);

/**
 * @param jsValue A ChakraObjectRef managing a JsValueRef.
 *
 * @returns A ChakraObjectRef managing the return value of the JS .toString
 * function.
 */
// TODO: Rename
JsValueRef ToJsString(JsValueRef jsValue);

/**
 * @param jsNumber A ChakraObjectRef managing a JS number.
 *
 * @returns The value of jsNumber converted to an integer.
 */
// TODO: Rename
int ToInteger(JsValueRef jsNumber);

/**
 * @returns A ChakraObjectRef managing a JS number.
 */
// TODO: Rename
JsValueRef ToJsNumber(int num);

/**
 * @returns A ChakraObjectRef managing a JS ArrayBuffer.
 *
 * @remarks The returned ArrayBuffer is backed by buffer and keeps buffer alive
 * till the garbage collector finalizes it.
 */
// TODO: Is it used?
JsValueRef ToJsArrayBuffer(const std::shared_ptr<const facebook::jsi::Buffer> &buffer);

/**
 * @returns A ChakraObjectRef managing a JS Object.
 *
 * @remarks The returned Object is backed by data and keeps data alive till the
 * garbage collector finalizes it.
 */
// TODO: Remove - replace with the external object functionality
template <typename T>
JsValueRef ToJsObject(const std::shared_ptr<T> &data) {
  if (!data) {
    throw facebook::jsi::JSINativeException("Cannot create an external JS Object without backing data.");
  }

  JsValueRef obj = nullptr;
  auto dataWrapper = std::make_unique<std::shared_ptr<T>>(data);

  // We allocate a copy of data on the heap, a shared_ptr which is deleted when
  // the JavaScript garbage collecotr releases the created external Object. This
  // ensures that data stays alive while the JavaScript engine is using it.
  VerifyChakraErrorElseThrow(JsCreateExternalObject(
      dataWrapper.get(),
      [](void *dataToDestroy) {
        // We wrap dataToDestroy in a unique_ptr to avoid calling delete
        // explicitly.
        std::unique_ptr<std::shared_ptr<T>> wrapper{static_cast<std::shared_ptr<T> *>(dataToDestroy)};
      },
      &obj));

  // We only call dataWrapper.release() after JsCreateExternalObject succeeds.
  // Otherwise, when JsCreateExternalObject fails and an exception is thrown,
  // the memory that dataWrapper used to own will be leaked.
  dataWrapper.release();
  return obj;
}

/**
 * @param object A ChakraObjectRef returned by ToJsObject.
 *
 * @returns The backing external data for object.
 */
// TODO: simplify - make it non-template
template <typename T>
const std::shared_ptr<T> &GetExternalData(JsValueRef object) {
  void *data;
  VerifyChakraErrorElseThrow(JsGetExternalData(object, &data));
  return *static_cast<std::shared_ptr<T> *>(data);
}

/**
 * @param jsValue1 A ChakraObjectRef managing a JsValueRef.
 * @param jsValue2 A ChakraObjectRef managing a JsValueRef.
 *
 * @returns A boolean indicating whether jsValue1 and jsValue2 are strictly
 * equal.
 */
// TODO: rename to equals
bool CompareJsValues(JsValueRef jsValue1, JsValueRef jsValue2);

/**
 * @param jsPropId1 A ChakraObjectRef managing a JsPropertyIdRef.
 * @param jsPropId2 A ChakraObjectRef managing a JsPropertyIdRef.
 *
 * @returns A boolean indicating whether jsPropId1 and jsPropId2 are strictly
 * equal.
 */
// TODO: rename to equals
bool CompareJsPropertyIds(JsValueRef jsPropId1, JsValueRef jsPropId2);

/**
 * @brief Cause a JS Error to be thrown in the engine's current context.
 *
 * @param message The message of the JS Error thrown.
 */
// TODO: review
void ThrowJsException(const std::string_view &message);

} // namespace Microsoft::JSI

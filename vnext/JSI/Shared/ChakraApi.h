// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#ifdef CHAKRACORE
#include "ChakraCore.h"
#else
#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT
#endif
#include <jsrt.h>
#endif

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#define ChakraVerifyElseCrash(condition, message) \
  do {                                            \
    if (!(condition)) {                           \
      assert(false && (message));                 \
      *((int *)0) = 1;                            \
    }                                             \
  } while (false)

namespace Microsoft::JSI {

// Forward declaration of types defined below.
struct ChakraApi;
struct ChakraJsRefHolder;

/**
 * @brief A smart pointer for Chakra JsRefs.
 *
 * JsRefs are references to objects owned by the garbage collector and include
 * JsContextRef, JsValueRef, and JsPropertyIdRef, etc. JsRefHolder ensures
 * that JsAddRef and JsRelease are called to handle the JsRef lifetime.
 */
struct ChakraJsRefHolder {
  ChakraJsRefHolder(std::nullptr_t = nullptr) noexcept {}
  ChakraJsRefHolder(ChakraApi *api, JsRef ref) noexcept;

  ChakraJsRefHolder(ChakraJsRefHolder const &other) noexcept;
  ChakraJsRefHolder(ChakraJsRefHolder &&other) noexcept;

  ChakraJsRefHolder &operator=(ChakraJsRefHolder const &other) noexcept;
  ChakraJsRefHolder &operator=(ChakraJsRefHolder &&other) noexcept;

  ~ChakraJsRefHolder() noexcept;

  JsRef Get() const noexcept {
    return m_ref;
  }

 private:
  ChakraApi *m_api{nullptr};
  JsRef m_ref{JS_INVALID_REFERENCE};
};

/**
 * @brief A wrapper for Chakra APIs.
 *
 * The ChakraApi class wraps up the Chakra API functions in a way that:
 * - functions throw exceptions instead of returning error code (derived class can define the exception types);
 * - standard library types are used when possible to simplify usage.
 *
 * The order of the functions below follows the corresponding functions order in the chakrart.h
 * Currently we only wrap up functions that are needed to implement the JSI API.
 */
struct ChakraApi {
  /**
   * @brief Throws an exception based on the errorCore.
   *
   * The base implementation throws the generic std::exception.
   * The method can be overridden in derived class to provide more specific exceptions.
   */
  [[noreturn]] virtual void ThrowJsException(JsErrorCode errorCode, JsValueRef exception);

  /**
   * @brief Throws an exception with the provided errorMessage.
   *
   * The base implementation throws the generic std::exception.
   * The method can be overridden in derived class to provide more specific exceptions.
   */
  [[noreturn]] virtual void ThrowNativeException(char const *errorMessage);

  /**
   * @brief Calls ThrowJsException in case if error is not JsNoError.
   */
  void VerifyJsErrorElseThrow(JsErrorCode errorCode);

  /**
   * @brief Calls ThrowNativeException in case if condition is false.
   */
  void VerifyElseThrow(bool condition, char const *errorMessage);

  /**
   * @brief Adds a reference to a garbage collected object.
   */
  uint32_t AddRef(JsRef ref);

  /**
   * @brief Releases a reference to a garbage collected object.
   */
  uint32_t Release(JsRef ref);

  /**
   * @brief Creates a script context for running scripts.
   */
  JsContextRef CreateContext(JsRuntimeHandle runtime);

  /**
   * @brief Gets the property ID associated with the name.
   */
  JsPropertyIdRef GetPropertyIdFromName(wchar_t const *name);

  /**
   * @brief Gets the property ID associated with the name.
   */
  JsPropertyIdRef GetPropertyIdFromName(std::string_view name);

  /**
   * @brief Gets the name associated with the property ID.
   */
  wchar_t const *GetPropertyNameFromId(JsPropertyIdRef propertyId);

  /**
   * @brief Gets the string associated with the property ID.
   */
  JsValueRef GetPropertyStringFromId(JsPropertyIdRef propertyId);

  /**
   * @brief Gets the symbol associated with the property ID.
   */
  JsValueRef GetSymbolFromPropertyId(JsPropertyIdRef propertyId);

  /**
   * @brief Gets the type of property.
   */
  JsPropertyIdType GetPropertyIdType(JsPropertyIdRef propertyId);

  /**
   * @brief Gets the property ID associated with the symbol.
   */
  JsPropertyIdRef GetPropertyIdFromSymbol(JsValueRef symbol);

  /**
   * @brief Creates symbol and gets the property ID associated with the symbol.
   */
  JsPropertyIdRef GetPropertyIdFromSymbol(std::wstring_view symbolDescription);

  /**
   * @brief Creates a JavaScript symbol.
   */
  JsValueRef CreateSymbol(JsValueRef symbolDescription);

  /**
   * @brief Creates a JavaScript symbol.
   */
  JsValueRef CreateSymbol(std::wstring_view symbolDescription);

  /**
   * @brief Gets the value of \c undefined in the current script context.
   */
  JsValueRef GetUndefinedValue();

  /**
   * @brief Gets the value of \c null in the current script context.
   */
  JsValueRef GetNullValue();

  /**
   * @brief Creates a Boolean value from a \c bool value.
   */
  JsValueRef BoolToBoolean(bool value);

  /**
   * @brief Retrieves the \c bool value of a Boolean value.
   */
  bool BooleanToBool(JsValueRef value);

  /**
   * @brief Gets the JavaScript type of a JsValueRef.
   */
  JsValueType GetValueType(JsValueRef value);

  /**
   * @brief Creates a number value from a \c double value.
   */
  JsValueRef DoubleToNumber(double value);

  /**
   * @brief Creates a number value from an \c int value.
   */
  JsValueRef IntToNumber(int32_t value);

  /**
   * @brief Retrieves the \c double value of a number value.
   */
  double NumberToDouble(JsValueRef value);

  /**
   * @brief Retrieves the \c int value of a number value.
   */
  int32_t NumberToInt(JsValueRef value);

  /**
   * @brief Creates a string value from a string pointer represented as std::wstring_view.
   */
  JsValueRef PointerToString(std::wstring_view value);

  /**
   * @brief Creates a string value from a string pointer represented as std::string_view.
   */
  JsValueRef PointerToString(std::string_view value);

  /**
   * @brief Retrieves the string pointer of a string value.
   *
   * This function retrieves the string pointer of a string value. It will fail with
   * an exception with \c JsErrorInvalidArgument if the type of the value is not string. The lifetime
   * of the string returned will be the same as the lifetime of the value it came from, however
   * the string pointer is not considered a reference to the value (and so will not keep it
   *from being collected).
   */
  std::wstring_view StringToPointer(JsValueRef string);

  /**
   * @brief Converts the string to the utf8-encoded std::string.
   */
  std::string StringToStdString(JsValueRef string);

  /**
   * @brief Converts the value to string using standard JavaScript semantics.
   */
  JsValueRef ConvertValueToString(JsValueRef value);

  /**
   * @brief Gets the global object in the current script context.
   */
  JsValueRef GetGlobalObject();

  /**
   * @brief Creates a new object.
   */
  JsValueRef CreateObject();

  /**
   * @brief Creates a new object that stores some external data.
   */
  JsValueRef CreateExternalObject(void *data, JsFinalizeCallback finalizeCallback);

  /**
   * @brief Creates a new object that stores external data as an std::unique_ptr.
   */
  template <typename T>
  JsValueRef CreateExternalObject(std::unique_ptr<T> &&data) {
    JsValueRef object = CreateExternalObject(data.get(), [](void *dataToDestroy) {
      // We wrap dataToDestroy in a unique_ptr to avoid calling delete explicitly.
      std::unique_ptr<T> wrapper{static_cast<T *>(dataToDestroy)};
    });

    // We only call data.release() after the CreateExternalObject succeeds.
    // Otherwise, when CreateExternalObject fails and an exception is thrown,
    // the memory that data used to own will be leaked.
    data.release();
    return object;
  }

  /**
   * @brief Returns the prototype of an object.
   */
  JsValueRef GetPrototype(JsValueRef object);

  /**
   * @brief Performs JavaScript "instanceof" operator test.
   */
  bool InstanceOf(JsValueRef object, JsValueRef constructor);

  /**
   * @brief Gets an object's property.
   */
  JsValueRef GetProperty(JsValueRef object, JsPropertyIdRef propertyId);

  /**
   * @brief Gets the list of all properties on the object.
   */
  JsValueRef GetOwnPropertyNames(JsValueRef object);

  /**
   * @brief Puts an object's property.
   */
  void SetProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef value);

  /**
   * @brief Determines whether an object has a property.
   */
  bool HasProperty(JsValueRef object, JsPropertyIdRef propertyId);

  /**
   * @brief Defines a new object's own property from a property descriptor.
   */
  bool DefineProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef propertyDescriptor);

  /**
   * @brief Retrieve the value at the specified index of an object.
   */
  JsValueRef GetIndexedProperty(JsValueRef object, int32_t index);

  /**
   * @brief Set the value at the specified index of an object.
   */
  void SetIndexedProperty(JsValueRef object, int32_t index, JsValueRef value);

  /**
   * @brief Compare two JavaScript values for strict equality.
   */
  bool StrictEquals(JsValueRef object1, JsValueRef object2);

  /**
   * @brief Retrieves the data from an external object.
   */
  void *GetExternalData(JsValueRef object);

  /**
   * @brief Creates a JavaScript array object.
   */
  JsValueRef CreateArray(size_t length);

  /**
   * @brief Creates a JavaScript ArrayBuffer object.
   */
  JsValueRef CreateArrayBuffer(size_t byteLength);

  /**
   * @brief A span of JsValueRef that can be used to pass arguments to function.
   *
   * For C++20 we should consider to replace it with std::span.
   */
  struct JsValueRefSpan {
    constexpr JsValueRefSpan(std::initializer_list<JsValueRef> il) noexcept
        : m_data{const_cast<JsValueRef *>(il.begin())}, m_size{il.size()} {}
    constexpr JsValueRefSpan(JsValueRef *data, size_t size) noexcept : m_data{data}, m_size{size} {}

    [[nodiscard]] constexpr JsValueRef *begin() const noexcept {
      return m_data;
    }

    [[nodiscard]] constexpr JsValueRef *end() const noexcept {
      return m_data + m_size;
    }

    [[nodiscard]] constexpr size_t size() const noexcept {
      return m_size;
    }

   private:
    JsValueRef *m_data;
    size_t m_size;
  };

  /**
   * @brief Invokes a function.
   */
  JsValueRef CallFunction(JsValueRef function, JsValueRefSpan args);

  /**
   * @brief Invokes a function as a constructor.
   */
  JsValueRef ConstructObject(JsValueRef function, JsValueRefSpan args);

  /**
   * @brief Creates a new JavaScript function with name.
   */
  JsValueRef CreateNamedFunction(JsValueRef name, JsNativeFunction nativeFunction, void *callbackState);

  /**
   * @brief  Sets the runtime of the current context to an exception state.
   *
   * It returns \c false in case if the current context is already in an exception state.
   */
  bool SetException(JsValueRef error) noexcept;

  /**
   * @brief  Sets the runtime of the current context to an exception state.
   */
  bool SetException(std::string_view message) noexcept;

  /**
   * @brief  Sets the runtime of the current context to an exception state.
   */
  bool SetException(std::wstring_view message) noexcept;
};

} // namespace Microsoft::JSI

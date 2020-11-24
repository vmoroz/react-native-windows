// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <JSApi.h>
#include <jsrt.h>

#ifndef NAPI_VERSION
#ifdef NAPI_EXPERIMENTAL
// Use INT_MAX, this should only be consumed by the pre-processor anyway.
#define NAPI_VERSION 2147483647
#else
// The baseline version for N-API
#define NAPI_VERSION 3
#endif
#endif

namespace jsapi {

struct ChakraEnvironment;

// Adapter for JSRT external data + finalize callback.
struct ExternalData {
  ExternalData(ChakraEnvironment *env, void *data, Finalize finalizeCallback, void *hint) noexcept;
  void *Data() noexcept;
  static void CALLBACK Finalize(void *callbackState) noexcept;

 private:
  ChakraEnvironment *m_env;
  void *m_data;
  jsapi::Finalize m_cb;
  void *m_hint;
};

struct ChakraEnvironment final : IEnvironment {
  ChakraEnvironment() noexcept;

  Status __stdcall GetLastErrorInfo(const ExtendedErrorInfo **result) noexcept override;

  // Getters for defined singletons
  Status __stdcall GetUndefined(Value *result) noexcept override;
  Status __stdcall GetNull(Value *result) noexcept override;
  Status __stdcall GetGlobal(Value *result) noexcept override;
  Status __stdcall GetBoolean(bool value, Value *result) noexcept override;

  // Methods to create Primitive types/Objects
  Status __stdcall CreateObject(Value *result) noexcept override;
  Status __stdcall CreateArray(Value *result) noexcept override;
  Status __stdcall CreateArrayWithLength(size_t length, Value *result) noexcept override;
  Status __stdcall CreateDouble(double value, Value *result) noexcept override;
  Status __stdcall CreateInt32(int32_t value, Value *result) noexcept override;
  Status __stdcall CreateUInt32(uint32_t value, Value *result) noexcept override;
  Status __stdcall CreateInt64(int64_t value, Value *result) noexcept override;
  Status __stdcall CreateStringLatin1(const char *str, size_t length, Value *result) noexcept override;
  Status __stdcall CreateStringUtf8(const char *str, size_t length, Value *result) noexcept override;
  Status __stdcall CreateStringUtf16(const char16_t *str, size_t length, Value *result) noexcept override;
  Status __stdcall CreateSymbol(Value description, Value *result) noexcept override;
  Status __stdcall CreateFunction(const char *utf8Name, size_t length, Callback cb, void *data, Value *result) noexcept
      override;
  Status __stdcall CreateError(Value code, Value msg, Value *result) noexcept override;
  Status __stdcall CreateTypeError(Value code, Value msg, Value *result) noexcept override;
  Status __stdcall CreateRangeError(Value code, Value msg, Value *result) noexcept override;

  // Methods to get the native Value from Primitive type
  Status __stdcall TypeOf(Value value, ValueType *result) noexcept override;
  Status __stdcall GetValueDouble(Value value, double *result) noexcept override;
  Status __stdcall GetValueInt32(Value value, int32_t *result) noexcept override;
  Status __stdcall GetValueUInt32(Value value, uint32_t *result) noexcept override;
  Status __stdcall GetValueInt64(Value value, int64_t *result) noexcept override;
  Status __stdcall GetValueBool(Value value, bool *result) noexcept override;

  // Copies LATIN-1 encoded bytes from a string into a buffer.
  Status __stdcall GetValueStringLatin1(Value value, char *buf, size_t bufSize, size_t *result) noexcept override;

  // Copies UTF-8 encoded bytes from a string into a buffer.
  Status __stdcall GetValueStringUtf8(Value value, char *buf, size_t bufSize, size_t *result) noexcept override;

  // Copies UTF-16 encoded bytes from a string into a buffer.
  Status __stdcall GetValueStringUtf16(Value value, char16_t *buf, size_t bufSize, size_t *result) noexcept override;

  // Methods to coerce values
  // These APIs may execute user scripts
  Status __stdcall CoerceToBool(Value value, Value *result) noexcept override;
  Status __stdcall CoerceToNumber(Value value, Value *result) noexcept override;
  Status __stdcall CoerceToObject(Value value, Value *result) noexcept override;
  Status __stdcall CoerceToString(Value value, Value *result) noexcept override;

  // Methods to work with Objects
  Status __stdcall GetPrototype(Value object, Value *result) noexcept override;
  Status __stdcall GetPropertyNames(Value object, Value *result) noexcept override;
  Status __stdcall SetProperty(Value object, Value key, Value value) noexcept override;
  Status __stdcall HasProperty(Value object, Value key, bool *result) noexcept override;
  Status __stdcall GetProperty(Value object, Value key, Value *result) noexcept override;
  Status __stdcall DeleteProperty(Value object, Value key, bool *result) noexcept override;
  Status __stdcall HasOwnProperty(Value object, Value key, bool *result) noexcept override;
  Status __stdcall SetNamedProperty(Value object, const char *utf8Name, Value value) noexcept override;
  Status __stdcall HasNamedProperty(Value object, const char *utf8Name, bool *result) noexcept override;
  Status __stdcall GetNamedProperty(Value object, const char *utf8Name, Value *result) noexcept override;
  Status __stdcall SetElement(Value object, uint32_t index, Value value) noexcept override;
  Status __stdcall HasElement(Value object, uint32_t index, bool *result) noexcept override;
  Status __stdcall GetElement(Value object, uint32_t index, Value *result) noexcept override;
  Status __stdcall DeleteElement(Value object, uint32_t index, bool *result) noexcept override;
  Status __stdcall DefineProperties(Value object, size_t propertyCount, const PropertyDescriptor *properties) noexcept
      override;

  // Methods to work with Arrays
  Status __stdcall IsArray(Value value, bool *result) noexcept override;
  Status __stdcall GetArrayLength(Value value, uint32_t *result) noexcept override;

  // Methods to compare values
  Status __stdcall StrictEquals(Value lhs, Value rhs, bool *result) noexcept override;

  // Methods to work with Functions
  Status __stdcall CallFunction(Value recv, Value func, size_t argc, const Value *argv, Value *result) noexcept
      override;
  Status __stdcall NewInstance(Value constructor, size_t argc, const Value *argv, Value *result) noexcept override;
  Status __stdcall InstanceOf(Value object, Value constructor, bool *result) noexcept override;

  // Methods to work with napi_callbacks

  // Gets all callback info in a single call. (Ugly, but faster.)
  Status __stdcall GetCallbackInfo(
      CallbackInfo callbackInfo, // [in] Opaque callback-info handle
      size_t *argc, // [in-out] Specifies the size of the provided argv array
                    // and receives the actual count of args.
      Value *argv, // [out] Array of values
      Value *thisArg, // [out] Receives the JS 'this' arg for the call
      void **data) noexcept override; // [out] Receives the data pointer for the callback.

  Status __stdcall GetNewTarget(CallbackInfo callbackInfo, Value *result) noexcept override;
  Status __stdcall DefineClass(
      const char *utf8Name,
      size_t length,
      Callback constructor,
      void *data,
      size_t propertyCount,
      const PropertyDescriptor *properties,
      Value *result) noexcept override;

  // Methods to work with external data objects
  Status __stdcall Wrap(
      Value jsObject,
      void *nativeObject,
      Finalize finalizeCallback,
      void *finalizeHint,
      Ref *result) noexcept override;
  Status __stdcall Unwrap(Value js_object, void **result) noexcept override;
  Status __stdcall RemoveWrap(Value jsObject, void **result) noexcept override;
  Status __stdcall CreateExternal(void *data, Finalize finalizeCallback, void *finalizeHint, Value *result) noexcept
      override;
  Status __stdcall GetValueExternal(Value value, void **result) noexcept override;

  // Methods to control object lifespan

  // Set initial_refcount to 0 for a weak reference, >0 for a strong reference.
  Status __stdcall CreateReference(Value value, uint32_t initialRefCount, Ref *result) noexcept override;

  // Deletes a reference. The referenced value is released, and may
  // be GC'd unless there are other references to it.
  Status __stdcall DeleteReference(Ref ref) noexcept override;

  // Increments the reference count, optionally returning the resulting count.
  // After this call the  reference will be a strong reference because its
  // refcount is >0, and the referenced object is effectively "pinned".
  // Calling this when the refcount is 0 and the object is unavailable
  // results in an error.
  Status __stdcall ReferenceRef(Ref ref, uint32_t *result) noexcept override;

  // Decrements the reference count, optionally returning the resulting count.
  // If the result is 0 the reference is now weak and the object may be GC'd
  // at any time if there are no other references. Calling this when the
  // refcount is already 0 results in an error.
  Status __stdcall ReferenceUnref(Ref ref, uint32_t *result) noexcept override;

  // Attempts to get a referenced value. If the reference is weak,
  // the value might no longer be available, in that case the call
  // is still successful but the result is NULL.
  Status __stdcall GetReferenceValue(Ref ref, Value *result) noexcept override;

  Status __stdcall OpenHandleScope(HandleScope *result) noexcept override;
  Status __stdcall CloseHandleScope(HandleScope scope) noexcept override;
  Status __stdcall OpenEscapableHandleScope(EscapableHandleScope *result) noexcept override;
  Status __stdcall CloseEscapableHandleScope(EscapableHandleScope scope) noexcept override;

  Status __stdcall EscapeHandle(EscapableHandleScope scope, Value escapee, Value *result) noexcept override;

  // Methods to support error handling
  Status __stdcall Throw(Value error) noexcept override;
  Status __stdcall ThrowError(const char *code, const char *msg) noexcept override;
  Status __stdcall ThrowTypeError(const char *code, const char *msg) noexcept override;
  Status __stdcall ThrowRangeError(const char *code, const char *msg) noexcept override;
  Status __stdcall IsError(Value value, bool *result) noexcept override;

  // Methods to support catching exceptions
  Status __stdcall IsExceptionPending(bool *result) noexcept override;
  Status __stdcall GetAndClearLastException(Value *result) noexcept override;

  // Methods to work with array buffers and typed arrays
  Status __stdcall IsArrayBuffer(Value value, bool *result) noexcept override;
  Status __stdcall CreateArrayBuffer(size_t byteLength, void **data, Value *result) noexcept override;
  Status __stdcall CreateExternalArrayBuffer(
      void *externalData,
      size_t byteLength,
      Finalize finalizeCallback,
      void *finalizeHint,
      Value *result) noexcept override;
  Status __stdcall GetArrayBufferInfo(Value arrayBuffer, void **data, size_t *byteLength) noexcept override;
  Status __stdcall IsTypedArray(Value value, bool *result) noexcept override;
  Status __stdcall CreateTypedArray(
      TypedArrayType type,
      size_t length,
      Value arrayBuffer,
      size_t byteOffset,
      Value *result) noexcept override;
  Status __stdcall GetTypedArrayInfo(
      Value typedArray,
      TypedArrayType *type,
      size_t *length,
      void **data,
      Value *arrayBuffer,
      size_t *byteOffset) noexcept override;

  Status __stdcall CreateDataView(size_t length, Value arrayBuffer, size_t byteOffset, Value *result) noexcept override;
  Status __stdcall IsDataView(Value value, bool *result) noexcept override;
  Status __stdcall GetDataViewInfo(
      Value dataView,
      size_t *byteLength,
      void **data,
      Value *arrayBuffer,
      size_t *byteOffset) noexcept override;

  // version management
  Status __stdcall GetVersion(uint32_t *result) noexcept override;

  // Promises
  Status __stdcall CreatePromise(Deferred *deferred, Value *promise) noexcept override;
  Status __stdcall ResolveDeferred(Deferred deferred, Value resolution) noexcept override;
  Status __stdcall RejectDeferred(Deferred deferred, Value rejection) noexcept override;
  Status __stdcall IsPromise(Value value, bool *isPromise) noexcept override;

  // Running a script
  Status __stdcall RunScript(Value script, Value *result) noexcept override;

  // Memory management
  Status __stdcall AdjustExternalMemory(int64_t changeInBytes, int64_t *adjustedValue) noexcept override;

  // Dates
  Status __stdcall CreateDate(double time, Value *result) noexcept override;
  Status __stdcall IsDate(Value value, bool *isDate) noexcept override;
  Status __stdcall GetDateValue(Value value, double *result) noexcept override;

  // Add finalizer for pointer
  Status __stdcall AddFinalizer(
      Value jsObject,
      void *nativeObject,
      Finalize finalizeCallback,
      void *finalizeHint,
      Ref *result) noexcept override;

  // BigInt
  Status __stdcall CreateBigIntInt64(int64_t value, Value *result) noexcept override;
  Status __stdcall CreateBigIntUInt64(uint64_t value, Value *result) noexcept override;
  Status __stdcall CreateBigIntWords(int signBit, size_t wordCount, const uint64_t *words, Value *result) noexcept
      override;
  Status __stdcall GetValueBigIntInt64(Value value, int64_t *result, bool *lossless) noexcept override;
  Status __stdcall GetValueBigIntUInt64(Value value, uint64_t *result, bool *lossless) noexcept override;
  Status __stdcall GetValueBigIntWords(Value value, int *signBit, size_t *wordCount, uint64_t *words) noexcept override;

  // Object
  Status __stdcall GetAllPropertyNames(
      Value object,
      KeyCollectionMode keyMode,
      KeyFilter keyFilter,
      KeyConversion keyConversion,
      Value *result) noexcept override;

  // Instance data
  Status __stdcall SetInstanceData(void *data, Finalize finalizeCallback, void *finalizeHint) noexcept override;
  Status __stdcall GetInstanceData(void **data) noexcept override;

  // ArrayBuffer detaching
  Status __stdcall DetachArrayBuffer(Value arraybuffer) noexcept override;
  Status __stdcall IsDetachedArrayBuffer(Value value, bool *result) noexcept override;

  // Type tagging
  Status __stdcall TypeTagObject(Value value, const TypeTag *typeTag) noexcept override;
  Status __stdcall CheckObjectTypeTag(Value value, const TypeTag *typeTag, bool *result) noexcept override;
  Status __stdcall ObjectFreeze(Value object) noexcept override;
  Status __stdcall ObjectSeal(Value object) noexcept override;

 public:
  void ClearLastError() noexcept;
  Status SetLastError(Status errorCode, uint32_t engineErrorCode = 0, void *engineReserved = nullptr) noexcept;
  Status SetLastError(JsErrorCode jsError, void *engineReserved = nullptr) noexcept;

 private: // Inner types
  enum class ChakraPropertyAttibutes {
    None = 0,
    ReadOnly = 1 << 1,
    DontEnum = 1 << 2,
    DontDelete = 1 << 3,
    Frozen = ReadOnly | DontDelete,
    DontEnumAndFrozen = DontEnum | Frozen,
  };

  friend constexpr ChakraPropertyAttibutes operator&(ChakraPropertyAttibutes left, ChakraPropertyAttibutes right) {
    return (ChakraPropertyAttibutes)((int)left & (int)right);
  }

  friend constexpr bool operator!(ChakraPropertyAttibutes attrs) {
    return attrs == ChakraPropertyAttibutes::None;
  }

  // @brief A smart pointer for JsRefs.
  //
  // JsRefs are references to objects owned by the garbage collector and include
  // JsContextRef, JsValueRef, and JsPropertyIdRef, etc. JsRefHolder ensures
  // that JsAddRef and JsRelease are called to handle the JsRef lifetime.
  // struct JsRefHolder final {
  //  JsRefHolder(std::nullptr_t = nullptr) noexcept {}
  //  explicit JsRefHolder(JsRef ref) noexcept;

  //  JsRefHolder(JsRefHolder const &other) noexcept;
  //  JsRefHolder(JsRefHolder &&other) noexcept;

  //  JsRefHolder &operator=(JsRefHolder const &other) noexcept;
  //  JsRefHolder &operator=(JsRefHolder &&other) noexcept;

  //  ~JsRefHolder() noexcept;

  //  operator JsRef() const noexcept {
  //    return m_ref;
  //  }

  // private:
  //  JsRef m_ref{JS_INVALID_REFERENCE};
  //};

  // struct CachedPropertyId final {
  //  const wchar_t *name;
  //  JsRefHolder propertyRef;
  //};

  //// Property ID cache to improve execution speed
  // struct CachedPropertyIds final {
  //  CachedPropertyId Object;
  //  CachedPropertyId Proxy;
  //  CachedPropertyId Symbol;
  //  CachedPropertyId byteLength;
  //  CachedPropertyId configurable;
  //  CachedPropertyId enumerable;
  //  CachedPropertyId get;
  //  CachedPropertyId hostFunctionSymbol;
  //  CachedPropertyId hostObjectSymbol;
  //  CachedPropertyId length;
  //  CachedPropertyId message;
  //  CachedPropertyId ownKeys;
  //  CachedPropertyId propertyIsEnumerable;
  //  CachedPropertyId prototype;
  //  CachedPropertyId set;
  //  CachedPropertyId toString;
  //  CachedPropertyId value;
  //  CachedPropertyId writable;
  //};

 private:
  Status SetErrorCode(JsValueRef error, Value code, const char *codeString) noexcept;
  Status CreatePropertyFunction(Value propertyName, Callback cb, void *callbackData, Value *result) noexcept;
  JsErrorCode JsPropertyIdFromPropertyDescriptor(const PropertyDescriptor *p, JsPropertyIdRef *propertyId) noexcept;
  JsErrorCode JsNameValueFromPropertyDescriptor(const PropertyDescriptor *p, Value *name) noexcept;
  Status FindWrapper(JsValueRef obj, JsValueRef *wrapper, JsValueRef *parent = nullptr) noexcept;
  Status UnwrapInternal(
      JsValueRef obj,
      ExternalData **externalData,
      JsValueRef *wrapper = nullptr,
      JsValueRef *parent = nullptr) noexcept;
  Status ConcludeDeferred(Deferred deferred, const char *property, Value result) noexcept;
  // JsErrorCode ChakraPropertyDescriptor(JsValueRef value, ChakraPropertyAttibutes attrs, JsValueRef *descriptor)
  // noexcept; JsErrorCode ChakraSetProperty(JsValueRef object, CachedPropertyId propertyId, JsValueRef value) noexcept;
  // JsErrorCode ChakraCachedPropertyId(CachedPropertyId cachedPropertyId, JsPropertyIdRef* propertyId) noexcept;

 private:
  ExtendedErrorInfo m_lastError{nullptr, nullptr, 0, Status::OK};
  JsValueRef has_own_property_function = JS_INVALID_REFERENCE;
  JsSourceContext source_context{JS_SOURCE_CONTEXT_NONE};
  // CachedPropertyIds m_propertyIds;
};

} // namespace jsapi

#if 0 
struct napi_env__ {
  JsSourceContext source_context = JS_SOURCE_CONTEXT_NONE;
  napi_extended_error_info last_error{ nullptr, nullptr, 0, napi_ok };
  JsValueRef has_own_property_function = JS_INVALID_REFERENCE;
};

#endif

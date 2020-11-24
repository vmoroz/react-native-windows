// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#ifndef JSAPI_H
#define JSAPI_H

// This file needs to be compatible with C compilers.
#include <cstddef>
#include <cstdint>

namespace jsapi {

constexpr size_t AutoLength = SIZE_MAX;

using Value = struct Value__ *;
using Ref = struct Ref__ *;
using HandleScope = struct HandleScope__ *;
using EscapableHandleScope = struct EscapableHandleScope__ *;
using CallbackInfo = struct CallbackInfo__ *;
using Deferred = struct Deferred__ *;

enum class PropertyAttributes {
  Default = 0,
  Writable = 1 << 0,
  Enumerable = 1 << 1,
  Configurable = 1 << 2,

  // Used with napi_define_class to distinguish static properties
  // from instance properties. Ignored by napi_define_properties.
  Static = 1 << 10,

  // Default for class methods.
  DefaultMethod = Writable | Configurable,

  // Default for object properties, like in JS obj[prop].
  DefaultJSProperty = Writable | Enumerable | Configurable,
};

constexpr PropertyAttributes operator&(PropertyAttributes left, PropertyAttributes right) noexcept {
  return (PropertyAttributes)((int)left & (int)right);
}

constexpr bool operator!(PropertyAttributes attrs) noexcept {
  return attrs == PropertyAttributes::Default;
}

enum class ValueType {
  // ES6 types (corresponds to typeof)
  Undefined,
  Null,
  Boolean,
  Number,
  String,
  Symbol,
  Object,
  Function,
  External,
  BigInt,
};

enum class TypedArrayType {
  Int8Array,
  UInt8Array,
  UInt8ClampedArray,
  Int16Array,
  UInt16Array,
  Int32Array,
  UInt32Array,
  Float32Array,
  Float64Array,
  BigInt64Array,
  BigUInt64Array,
};

enum class Status {
  OK,
  InvalidArg,
  ObjectExpected,
  StringExpected,
  NameExpected,
  FunctionExpected,
  NumberExpected,
  BooleanExpected,
  ArrayExpected,
  GenericFailure,
  PendingException,
  Cancelled,
  EscapeCalledTwice,
  HandleScopeMismatch,
  CallbackScopeMismatch,
  QueueFull,
  Closing,
  BigIntExpected,
  DateExpected,
  ArrayBufferExpected,
  DetachableArrayBufferExpected,
  WouldDeadlock // unused
};

struct IEnvironment;
using Callback = Value (*)(IEnvironment *env, CallbackInfo info) noexcept;
using Finalize = void (*)(IEnvironment *env, void *finalizeData, void *finalizeHint) noexcept;

struct PropertyDescriptor {
  // One of utf8Name or name should be NULL.
  const char *utf8Name;
  Value name;

  Callback method;
  Callback getter;
  Callback setter;
  Value value;

  PropertyAttributes attributes;
  void *data;
};

struct ExtendedErrorInfo {
  const char *errorMessage;
  void *engineReserved;
  uint32_t engineErrorCode;
  Status errorCode;
};

enum class KeyCollectionMode { Prototypes, OwnOnly };

enum class KeyFilter {
  AllProperties = 0,
  Writable = 1,
  Enumerable = 1 << 1,
  Configurable = 1 << 2,
  SkipStrings = 1 << 3,
  SkipSymbols = 1 << 4
};

enum class KeyConversion { KeepNumbers, NumbersToStrings };

struct TypeTag {
  uint64_t lower;
  uint64_t upper;
};

struct IEnvironment {
  virtual Status __stdcall GetLastErrorInfo(const ExtendedErrorInfo **result) noexcept = 0;

  // Getters for defined singletons
  virtual Status __stdcall GetUndefined(Value *result) noexcept = 0;
  virtual Status __stdcall GetNull(Value *result) noexcept = 0;
  virtual Status __stdcall GetGlobal(Value *result) noexcept = 0;
  virtual Status __stdcall GetBoolean(bool value, Value *result) noexcept = 0;

  // Methods to create Primitive types/Objects
  virtual Status __stdcall CreateObject(Value *result) noexcept = 0;
  virtual Status __stdcall CreateArray(Value *result) noexcept = 0;
  virtual Status __stdcall CreateArrayWithLength(size_t length, Value *result) noexcept = 0;
  virtual Status __stdcall CreateDouble(double value, Value *result) noexcept = 0;
  virtual Status __stdcall CreateInt32(int32_t value, Value *result) noexcept = 0;
  virtual Status __stdcall CreateUInt32(uint32_t value, Value *result) noexcept = 0;
  virtual Status __stdcall CreateInt64(int64_t value, Value *result) noexcept = 0;
  virtual Status __stdcall CreateStringLatin1(const char *str, size_t length, Value *result) noexcept = 0;
  virtual Status __stdcall CreateStringUtf8(const char *str, size_t length, Value *result) noexcept = 0;
  virtual Status __stdcall CreateStringUtf16(const char16_t *str, size_t length, Value *result) noexcept = 0;
  virtual Status __stdcall CreateSymbol(Value description, Value *result) noexcept = 0;
  virtual Status __stdcall CreateFunction(
      const char *utf8name,
      size_t length,
      Callback cb,
      void *data,
      Value *result) noexcept = 0;
  virtual Status __stdcall CreateError(Value code, Value msg, Value *result) noexcept = 0;
  virtual Status __stdcall CreateTypeError(Value code, Value msg, Value *result) noexcept = 0;
  virtual Status __stdcall CreateRangeError(Value code, Value msg, Value *result) noexcept = 0;

  // Methods to get the native Value from Primitive type
  virtual Status __stdcall TypeOf(Value value, ValueType *result) noexcept = 0;
  virtual Status __stdcall GetValueDouble(Value value, double *result) noexcept = 0;
  virtual Status __stdcall GetValueInt32(Value value, int32_t *result) noexcept = 0;
  virtual Status __stdcall GetValueUInt32(Value value, uint32_t *result) noexcept = 0;
  virtual Status __stdcall GetValueInt64(Value value, int64_t *result) noexcept = 0;
  virtual Status __stdcall GetValueBool(Value value, bool *result) noexcept = 0;

  // Copies LATIN-1 encoded bytes from a string into a buffer.
  virtual Status __stdcall GetValueStringLatin1(Value value, char *buf, size_t bufSize, size_t *result) noexcept = 0;

  // Copies UTF-8 encoded bytes from a string into a buffer.
  virtual Status __stdcall GetValueStringUtf8(Value value, char *buf, size_t bufSize, size_t *result) noexcept = 0;

  // Copies UTF-16 encoded bytes from a string into a buffer.
  virtual Status __stdcall GetValueStringUtf16(Value value, char16_t *buf, size_t bufSize, size_t *result) noexcept = 0;

  // Methods to coerce values
  // These APIs may execute user scripts
  virtual Status __stdcall CoerceToBool(Value value, Value *result) noexcept = 0;
  virtual Status __stdcall CoerceToNumber(Value value, Value *result) noexcept = 0;
  virtual Status __stdcall CoerceToObject(Value value, Value *result) noexcept = 0;
  virtual Status __stdcall CoerceToString(Value value, Value *result) noexcept = 0;

  // Methods to work with Objects
  virtual Status __stdcall GetPrototype(Value object, Value *result) noexcept = 0;
  virtual Status __stdcall GetPropertyNames(Value object, Value *result) noexcept = 0;
  virtual Status __stdcall SetProperty(Value object, Value key, Value value) noexcept = 0;
  virtual Status __stdcall HasProperty(Value object, Value key, bool *result) noexcept = 0;
  virtual Status __stdcall GetProperty(Value object, Value key, Value *result) noexcept = 0;
  virtual Status __stdcall DeleteProperty(Value object, Value key, bool *result) noexcept = 0;
  virtual Status __stdcall HasOwnProperty(Value object, Value key, bool *result) noexcept = 0;
  virtual Status __stdcall SetNamedProperty(Value object, const char *utf8Name, Value value) noexcept = 0;
  virtual Status __stdcall HasNamedProperty(Value object, const char *utf8Name, bool *result) noexcept = 0;
  virtual Status __stdcall GetNamedProperty(Value object, const char *utf8Name, Value *result) noexcept = 0;
  virtual Status __stdcall SetElement(Value object, uint32_t index, Value value) noexcept = 0;
  virtual Status __stdcall HasElement(Value object, uint32_t index, bool *result) noexcept = 0;
  virtual Status __stdcall GetElement(Value object, uint32_t index, Value *result) noexcept = 0;
  virtual Status __stdcall DeleteElement(Value object, uint32_t index, bool *result) noexcept = 0;
  virtual Status __stdcall DefineProperties(
      Value object,
      size_t propertyCount,
      const PropertyDescriptor *properties) noexcept = 0;

  // Methods to work with Arrays
  virtual Status __stdcall IsArray(Value value, bool *result) noexcept = 0;
  virtual Status __stdcall GetArrayLength(Value value, uint32_t *result) noexcept = 0;

  // Methods to compare values
  virtual Status __stdcall StrictEquals(Value lhs, Value rhs, bool *result) noexcept = 0;

  // Methods to work with Functions
  virtual Status __stdcall CallFunction(
      Value recv,
      Value func,
      size_t argc,
      const Value *argv,
      Value *result) noexcept = 0;
  virtual Status __stdcall NewInstance(Value constructor, size_t argc, const Value *argv, Value *result) noexcept = 0;
  virtual Status __stdcall InstanceOf(Value object, Value constructor, bool *result) noexcept = 0;

  // Methods to work with napi_callbacks

  // Gets all callback info in a single call. (Ugly, but faster.)
  virtual Status __stdcall GetCallbackInfo(
      CallbackInfo callbackInfo, // [in] Opaque callback-info handle
      size_t *argc, // [in-out] Specifies the size of the provided argv array
                    // and receives the actual count of args.
      Value *argv, // [out] Array of values
      Value *thisArg, // [out] Receives the JS 'this' arg for the call
      void **data) noexcept = 0; // [out] Receives the data pointer for the callback.

  virtual Status __stdcall GetNewTarget(CallbackInfo callbackInfo, Value *result) noexcept = 0;
  virtual Status __stdcall DefineClass(
      const char *utf8Name,
      size_t length,
      Callback constructor,
      void *data,
      size_t propertyCount,
      const PropertyDescriptor *properties,
      Value *result) noexcept = 0;

  // Methods to work with external data objects
  virtual Status __stdcall Wrap(
      Value jsObject,
      void *nativeObject,
      Finalize finalizeCallback,
      void *finalizeHint,
      Ref *result) noexcept = 0;
  virtual Status __stdcall Unwrap(Value js_object, void **result) noexcept = 0;
  virtual Status __stdcall RemoveWrap(Value jsObject, void **result) noexcept = 0;
  virtual Status __stdcall CreateExternal(
      void *data,
      Finalize finalizeCallback,
      void *finalizeHint,
      Value *result) noexcept = 0;
  virtual Status __stdcall GetValueExternal(Value value, void **result) noexcept = 0;

  // Methods to control object lifespan

  // Set initial_refcount to 0 for a weak reference, >0 for a strong reference.
  virtual Status __stdcall CreateReference(Value value, uint32_t initialRefCount, Ref *result) noexcept = 0;

  // Deletes a reference. The referenced value is released, and may
  // be GC'd unless there are other references to it.
  virtual Status __stdcall DeleteReference(Ref ref) noexcept = 0;

  // Increments the reference count, optionally returning the resulting count.
  // After this call the  reference will be a strong reference because its
  // refcount is >0, and the referenced object is effectively "pinned".
  // Calling this when the refcount is 0 and the object is unavailable
  // results in an error.
  virtual Status __stdcall ReferenceRef(Ref ref, uint32_t *result) noexcept = 0;

  // Decrements the reference count, optionally returning the resulting count.
  // If the result is 0 the reference is now weak and the object may be GC'd
  // at any time if there are no other references. Calling this when the
  // refcount is already 0 results in an error.
  virtual Status __stdcall ReferenceUnref(Ref ref, uint32_t *result) noexcept = 0;

  // Attempts to get a referenced value. If the reference is weak,
  // the value might no longer be available, in that case the call
  // is still successful but the result is NULL.
  virtual Status __stdcall GetReferenceValue(Ref ref, Value *result) noexcept = 0;

  virtual Status __stdcall OpenHandleScope(HandleScope *result) noexcept = 0;
  virtual Status __stdcall CloseHandleScope(HandleScope scope) noexcept = 0;
  virtual Status __stdcall OpenEscapableHandleScope(EscapableHandleScope *result) noexcept = 0;
  virtual Status __stdcall CloseEscapableHandleScope(EscapableHandleScope scope) noexcept = 0;

  virtual Status __stdcall EscapeHandle(EscapableHandleScope scope, Value escapee, Value *result) noexcept = 0;

  // Methods to support error handling
  virtual Status __stdcall Throw(Value error) noexcept = 0;
  virtual Status __stdcall ThrowError(const char *code, const char *msg) noexcept = 0;
  virtual Status __stdcall ThrowTypeError(const char *code, const char *msg) noexcept = 0;
  virtual Status __stdcall ThrowRangeError(const char *code, const char *msg) noexcept = 0;
  virtual Status __stdcall IsError(Value value, bool *result) noexcept = 0;

  // Methods to support catching exceptions
  virtual Status __stdcall IsExceptionPending(bool *result) noexcept = 0;
  virtual Status __stdcall GetAndClearLastException(Value *result) noexcept = 0;

  // Methods to work with array buffers and typed arrays
  virtual Status __stdcall IsArrayBuffer(Value value, bool *result) noexcept = 0;
  virtual Status __stdcall CreateArrayBuffer(size_t byteLength, void **data, Value *result) noexcept = 0;
  virtual Status __stdcall CreateExternalArrayBuffer(
      void *externalData,
      size_t byteLength,
      Finalize finalizeCallback,
      void *finalizeHint,
      Value *result) noexcept = 0;
  virtual Status __stdcall GetArrayBufferInfo(Value arrayBuffer, void **data, size_t *byteLength) noexcept = 0;
  virtual Status __stdcall IsTypedArray(Value value, bool *result) noexcept = 0;
  virtual Status __stdcall CreateTypedArray(
      TypedArrayType type,
      size_t length,
      Value arrayBuffer,
      size_t byteOffset,
      Value *result) noexcept = 0;
  virtual Status __stdcall GetTypedArrayInfo(
      Value typedArray,
      TypedArrayType *type,
      size_t *length,
      void **data,
      Value *arrayBuffer,
      size_t *byteOffset) noexcept = 0;

  virtual Status __stdcall CreateDataView(
      size_t length,
      Value arrayBuffer,
      size_t byteOffset,
      Value *result) noexcept = 0;
  virtual Status __stdcall IsDataView(Value value, bool *result) noexcept = 0;
  virtual Status __stdcall GetDataViewInfo(
      Value dataView,
      size_t *byteLength,
      void **data,
      Value *arrayBuffer,
      size_t *byteOffset) noexcept = 0;

  // version management
  virtual Status __stdcall GetVersion(uint32_t *result) noexcept = 0;

  // Promises
  virtual Status __stdcall CreatePromise(Deferred *deferred, Value *promise) noexcept = 0;
  virtual Status __stdcall ResolveDeferred(Deferred deferred, Value resolution) noexcept = 0;
  virtual Status __stdcall RejectDeferred(Deferred deferred, Value rejection) noexcept = 0;
  virtual Status __stdcall IsPromise(Value value, bool *isPromise) noexcept = 0;

  // Running a script
  virtual Status __stdcall RunScript(Value script, Value *result) noexcept = 0;

  // Memory management
  virtual Status __stdcall AdjustExternalMemory(int64_t changeInBytes, int64_t *adjustedValue) noexcept = 0;

  // Dates
  virtual Status __stdcall CreateDate(double time, Value *result) noexcept = 0;
  virtual Status __stdcall IsDate(Value value, bool *isDate) noexcept = 0;
  virtual Status __stdcall GetDateValue(Value value, double *result) noexcept = 0;

  // Add finalizer for pointer
  virtual Status __stdcall AddFinalizer(
      Value jsObject,
      void *nativeObject,
      Finalize finalizeCallback,
      void *finalizeHint,
      Ref *result) noexcept = 0;

  // BigInt
  virtual Status __stdcall CreateBigIntInt64(int64_t value, Value *result) noexcept = 0;
  virtual Status __stdcall CreateBigIntUInt64(uint64_t value, Value *result) noexcept = 0;
  virtual Status __stdcall CreateBigIntWords(
      int signBit,
      size_t wordCount,
      const uint64_t *words,
      Value *result) noexcept = 0;
  virtual Status __stdcall GetValueBigIntInt64(Value value, int64_t *result, bool *lossless) noexcept = 0;
  virtual Status __stdcall GetValueBigIntUInt64(Value value, uint64_t *result, bool *lossless) noexcept = 0;
  virtual Status __stdcall GetValueBigIntWords(
      Value value,
      int *signBit,
      size_t *wordCount,
      uint64_t *words) noexcept = 0;

  // Object
  virtual Status __stdcall GetAllPropertyNames(
      Value object,
      KeyCollectionMode keyMode,
      KeyFilter keyFilter,
      KeyConversion keyConversion,
      Value *result) noexcept = 0;

  // Instance data
  virtual Status __stdcall SetInstanceData(void *data, Finalize finalizeCallback, void *finalizeHint) noexcept = 0;
  virtual Status __stdcall GetInstanceData(void **data) noexcept = 0;

  // ArrayBuffer detaching
  virtual Status __stdcall DetachArrayBuffer(Value arraybuffer) noexcept = 0;
  virtual Status __stdcall IsDetachedArrayBuffer(Value value, bool *result) noexcept = 0;

  // Type tagging
  virtual Status __stdcall TypeTagObject(Value value, const TypeTag *typeTag) noexcept = 0;
  virtual Status __stdcall CheckObjectTypeTag(Value value, const TypeTag *typeTag, bool *result) noexcept = 0;
  virtual Status __stdcall ObjectFreeze(Value object) noexcept = 0;
  virtual Status __stdcall ObjectSeal(Value object) noexcept = 0;
};

} // namespace jsapi

#endif // JSAPI_H

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifdef CHAKRACORE
#include "ChakraCore.h"
#else
#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT
#endif
#include <jsrt.h>
#endif
#include <array>
#include <cassert>
#include <cmath>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "ChakraNapi.h"
#include "../JSI/ChakraRuntimeArgs.h"

[[noreturn]] void CrashWithAccessViolation() noexcept {
  *((int *)0) = 1;
  __fastfail(FAST_FAIL_INVALID_ARG);
}

// Check condition and crash process if it fails.
#define CRASH_IF_FALSE(condition)             \
  do {                                        \
    if (!(condition)) {                       \
      assert(false && "Failed: " #condition); \
      CrashWithAccessViolation();             \
    }                                         \
  } while (false)

#define RETURN_ENV_STATUS_IF_FALSE(env, condition, status) \
  do {                                                     \
    if (!(condition)) {                                    \
      return (env)->SetLastError(status);                  \
    }                                                      \
  } while (0)

#define CHECK_ENV_ARG(env, arg) RETURN_ENV_STATUS_IF_FALSE((env), ((arg) != nullptr), napi_invalid_arg)

#define CHECK_ENV_JSRT(env, expr)      \
  do {                                 \
    JsErrorCode err = (expr);          \
    if (err != JsNoError) {            \
      return (env)->SetLastError(err); \
    }                                  \
  } while (0)

#define RETURN_STATUS_IF_FALSE(condition, status) RETURN_ENV_STATUS_IF_FALSE(this, (condition), (status))

#define CHECK_ARG(arg) CHECK_ENV_ARG(this, (arg))

#define CHECK_JSRT(expr) CHECK_ENV_JSRT(this, (expr))

#define CHECK_JSRT_EXPECTED(expr, expected) \
  do {                                      \
    JsErrorCode err = (expr);               \
    if (err == JsErrorInvalidArgument)      \
      return SetLastError(expected);        \
    if (err != JsNoError)                   \
      return SetLastError(err);             \
  } while (0)

#define CHECKED_ENV(env) ((env) == nullptr) ? napi_invalid_arg : (env)

#define CHECKED_REF(ref) ((ref) == nullptr) ? napi_invalid_arg : reinterpret_cast<chakra::Reference *>(ref)

#define CHECK_JSRT_ERROR_CODE(expr)         \
  do {                                      \
    auto result__ = (expr);                 \
    if (result__ != JsErrorCode::JsNoError) \
      return result__;                      \
  } while (0)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)         \
  do {                           \
    napi_status status = (expr); \
    if (status != napi_ok)       \
      return status;             \
  } while (0)

// utf8 multibyte codepoint start check
#define UTF8_MULTIBYTE_START(c) (((c)&0xC0) == 0xC0)

#define STR_AND_LENGTH(str) str, std::size(str) - 1

namespace chakra {

template <typename T, size_t CallstackSize>
struct SmallBuffer final {
  SmallBuffer(size_t size) noexcept
      : m_size{size}, m_heapData{m_size > CallstackSize ? std::make_unique<T[]>(m_size) : nullptr} {}

  T *Data() noexcept {
    return m_heapData ? m_heapData.get() : m_stackData.data();
  }

  size_t Size() const noexcept {
    return m_size;
  }

 private:
  size_t const m_size{};
  std::array<T, CallstackSize> m_stackData{};
  std::unique_ptr<T[]> const m_heapData{};
};

struct RefTracker {
  RefTracker() = default;
  virtual ~RefTracker() noexcept {}
  virtual void Finalize(bool isEnvTeardown) noexcept {}

  using RefList = RefTracker;

  inline void Link(RefList *list) noexcept {
    m_prev = list;
    m_next = list->m_next;
    if (m_next != nullptr) {
      m_next->m_prev = this;
    }
    list->m_next = this;
  }

  inline void Unlink() noexcept {
    if (m_prev != nullptr) {
      m_prev->m_next = m_next;
    }
    if (m_next != nullptr) {
      m_next->m_prev = m_prev;
    }
    m_prev = nullptr;
    m_next = nullptr;
  }

  static void FinalizeAll(RefList *list) noexcept {
    while (list->m_next != nullptr) {
      list->m_next->Finalize(true);
    }
  }

 private:
  RefList *m_next{nullptr};
  RefList *m_prev{nullptr};
};

struct JsRefHolder final {
  JsRefHolder(std::nullptr_t = nullptr) noexcept;
  explicit JsRefHolder(JsRef ref) noexcept;

  JsRefHolder(JsRefHolder const &other) noexcept;
  JsRefHolder(JsRefHolder &&other) noexcept;

  JsRefHolder &operator=(JsRefHolder const &other) noexcept;
  JsRefHolder &operator=(JsRefHolder &&other) noexcept;

  ~JsRefHolder() noexcept;

  operator JsRef() const noexcept {
    return m_ref;
  }

 private:
  JsRef m_ref{JS_INVALID_REFERENCE};
};

struct CachedPropertyId {
  CachedPropertyId(std::wstring_view name, JsPropertyIdType propertyIdType = JsPropertyIdTypeString) noexcept
      : m_name{std::move(name)}, m_propertyIdType{propertyIdType} {}

  JsErrorCode Get(JsPropertyIdRef *result) noexcept {
    if (m_propertyId == JS_INVALID_REFERENCE) {
      if (m_propertyIdType == JsPropertyIdTypeString) {
        CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromName(m_name.data(), &m_propertyId));
      } else {
        CRASH_IF_FALSE(m_propertyIdType == JsPropertyIdTypeSymbol);
        JsValueRef propertyStr{JS_INVALID_REFERENCE};
        JsValueRef propertySymbol{JS_INVALID_REFERENCE};
        CHECK_JSRT_ERROR_CODE(JsPointerToString(m_name.data(), m_name.size(), &propertyStr));
        CHECK_JSRT_ERROR_CODE(JsCreateSymbol(propertyStr, &propertySymbol));
        CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromSymbol(propertySymbol, &m_propertyId));
      }

      CHECK_JSRT_ERROR_CODE(JsAddRef(m_propertyId, nullptr));
    }

    *result = m_propertyId;
    return JsErrorCode::JsNoError;
  }

  static JsErrorCode GetPropertyId(JsPropertyIdRef propertyId, JsPropertyIdRef *result) noexcept {
    *result = propertyId;
    return JsNoError;
  }

  static JsErrorCode GetPropertyId(CachedPropertyId &propertyId, JsPropertyIdRef *result) noexcept {
    return propertyId.Get(result);
  }

 private:
  JsPropertyIdRef m_propertyId{JS_INVALID_REFERENCE};
  const std::wstring_view m_name{nullptr};
  JsPropertyIdType m_propertyIdType;
};

enum class PropertyAttibutes {
  None = 0,
  ReadOnly = 1 << 1,
  DontEnum = 1 << 2,
  DontDelete = 1 << 3,
  Frozen = ReadOnly | DontDelete,
  DontEnumAndFrozen = DontEnum | Frozen,
};

constexpr PropertyAttibutes operator&(PropertyAttibutes left, PropertyAttibutes right) {
  return (PropertyAttibutes)((int)left & (int)right);
}

constexpr bool operator!(PropertyAttibutes attrs) {
  return attrs == PropertyAttibutes::None;
}

struct Environment;

struct CachedValue {
  using StaticGetter = JsErrorCode (*)(JsValueRef *);
  using InstanceGetter = JsErrorCode (Environment::*)(JsValueRef *);

  CachedValue(StaticGetter staticGetter) noexcept : m_staticGetter{staticGetter} {}

  CachedValue(Environment *env, InstanceGetter instanceGetter) noexcept
      : m_env{env}, m_instanceGetter{instanceGetter} {}

  JsErrorCode Get(JsValueRef *result) noexcept {
    if (m_value == JS_INVALID_REFERENCE) {
      JsValueRef propertyStr{}, propertySymbol{};
      if (m_env) {
        CHECK_JSRT_ERROR_CODE((m_env->*m_instanceGetter)(&m_value));
      } else {
        CHECK_JSRT_ERROR_CODE(m_staticGetter(&m_value));
      }
      CHECK_JSRT_ERROR_CODE(JsAddRef(m_value, nullptr));
    }

    *result = m_value;
    return JsErrorCode::JsNoError;
  }

  static JsErrorCode GetValue(JsValueRef value, JsValueRef *result) noexcept {
    *result = value;
    return JsNoError;
  }

  static JsErrorCode GetValue(CachedValue &value, JsValueRef *result) noexcept {
    return value.Get(result);
  }

 private:
  JsValueRef m_value{};
  Environment *m_env{};
  union {
    StaticGetter m_staticGetter;
    InstanceGetter m_instanceGetter;
  };
};

struct Environment {
  explicit Environment(Microsoft::JSI::ChakraRuntimeArgs &&args) noexcept;
  ~Environment() noexcept;

  JsContextRef Context() const noexcept;

  void Ref() noexcept;
  void Unref() noexcept;

  void LinkReference(RefTracker::RefList *reference) noexcept;
  void LinkFinalizingReference(RefTracker::RefList *reference) noexcept;

 public: // N-API implementation
  // The number of arguments that we keep on stack.
  // We use heap if we have more argument.
  constexpr static size_t MaxStackArgCount = 8;

  napi_status SetErrorCode(JsValueRef error, napi_value code, const char *codeString) noexcept;
  napi_status CreatePropertyFunction(
      napi_value propertyName,
      napi_callback callback,
      void *callbackData,
      napi_value *result) noexcept;

  void ClearLastError() noexcept;
  napi_status
  SetLastError(napi_status errorCode, uint32_t engineErrorCode = 0, void *engineReserved = nullptr) noexcept;
  napi_status SetLastError(JsErrorCode jsError, void *engineReserved = nullptr) noexcept;
  napi_status GetLastErrorInfo(const napi_extended_error_info **result) noexcept;
  napi_status GetUndefined(napi_value *result) noexcept;
  napi_status GetNull(napi_value *result) noexcept;
  napi_status GetGlobal(napi_value *result) noexcept;
  napi_status GetBoolean(bool value, napi_value *result) noexcept;
  napi_status CreateObject(napi_value *result) noexcept;
  napi_status CreateArray(napi_value *result) noexcept;
  napi_status CreateArrayWithLength(size_t length, napi_value *result) noexcept;
  napi_status CreateDouble(double value, napi_value *result) noexcept;
  napi_status CreateInt32(int32_t value, napi_value *result) noexcept;
  napi_status CreateUInt32(uint32_t value, napi_value *result) noexcept;
  napi_status CreateInt64(int64_t value, napi_value *result) noexcept;
  napi_status CreateStringLatin1(const char *str, size_t length, napi_value *result) noexcept;
  napi_status CreateStringUtf8(const char *str, size_t length, napi_value *result) noexcept;
  napi_status CreateStringUtf16(const char16_t *str, size_t length, napi_value *result) noexcept;
  napi_status CreateSymbol(napi_value description, napi_value *result) noexcept;

  napi_status GetUniqueString(napi_value str, napi_value *result) noexcept;
  napi_status GetUniqueStringLatin1(const char *str, size_t length, napi_value *result) noexcept;
  napi_status GetUniqueStringUtf8(const char *str, size_t length, napi_value *result) noexcept;
  napi_status GetUniqueStringUtf16(const char16_t *str, size_t length, napi_value *result) noexcept;

  napi_status
  CreateFunction(const char *utf8Name, size_t length, napi_callback callback, void *data, napi_value *result) noexcept;

  napi_status CreateError(napi_value code, napi_value msg, napi_value *result) noexcept;

  napi_status CreateTypeError(napi_value code, napi_value msg, napi_value *result) noexcept;

  napi_status CreateRangeError(napi_value code, napi_value msg, napi_value *result) noexcept;

  napi_status TypeOf(napi_value value, napi_valuetype *result) noexcept;

  napi_status GetValueDouble(napi_value value, double *result) noexcept;

  napi_status GetValueInt32(napi_value value, int32_t *result) noexcept;

  napi_status GetValueUInt32(napi_value value, uint32_t *result) noexcept;

  napi_status GetValueInt64(napi_value value, int64_t *result) noexcept;

  napi_status GetValueBool(napi_value value, bool *result) noexcept;

  napi_status GetValueStringLatin1(napi_value value, char *buf, size_t bufSize, size_t *result) noexcept;
  napi_status GetValueStringUtf8(napi_value value, char *buf, size_t bufSize, size_t *result) noexcept;
  napi_status GetValueStringUtf16(napi_value value, char16_t *buf, size_t bufSize, size_t *result) noexcept;

  napi_status CoerceToBool(napi_value value, napi_value *result) noexcept;

  napi_status CoerceToNumber(napi_value value, napi_value *result) noexcept;

  napi_status CoerceToObject(napi_value value, napi_value *result) noexcept;
  napi_status CoerceToString(napi_value value, napi_value *result) noexcept;

  napi_status GetPrototype(napi_value object, napi_value *result) noexcept;

  napi_status GetPropertyNames(napi_value object, napi_value *result) noexcept;

  napi_status SetProperty(napi_value object, napi_value key, napi_value value) noexcept;

  napi_status HasProperty(napi_value object, napi_value key, bool *result) noexcept;

  napi_status GetProperty(napi_value object, napi_value key, napi_value *result) noexcept;

  napi_status DeleteProperty(napi_value object, napi_value key, bool *result) noexcept;

  napi_status HasOwnProperty(napi_value object, napi_value key, bool *result) noexcept;

  napi_status SetNamedProperty(napi_value object, const char *utf8Name, napi_value value) noexcept;

  napi_status HasNamedProperty(napi_value object, const char *utf8Name, bool *result) noexcept;

  napi_status GetNamedProperty(napi_value object, const char *utf8Name, napi_value *result) noexcept;

  napi_status SetElement(napi_value object, uint32_t index, napi_value value) noexcept;

  napi_status HasElement(napi_value object, uint32_t index, bool *result) noexcept;

  napi_status GetElement(napi_value object, uint32_t index, napi_value *result) noexcept;

  napi_status DeleteElement(napi_value object, uint32_t index, bool *result) noexcept;

  napi_status
  DefineProperties(napi_value object, size_t propertyCount, const napi_property_descriptor *properties) noexcept;

  napi_status IsArray(napi_value value, bool *result) noexcept;
  napi_status GetArrayLength(napi_value value, uint32_t *result) noexcept;
  napi_status StrictEquals(napi_value lhs, napi_value rhs, bool *result) noexcept;

  napi_status
  CallFunction(napi_value recv, napi_value func, size_t argc, const napi_value *argv, napi_value *result) noexcept;
  napi_status NewInstance(napi_value constructor, size_t argc, const napi_value *argv, napi_value *result) noexcept;
  napi_status InstanceOf(napi_value object, napi_value constructor, bool *result) noexcept;

  // Gets all callback info in a single call. (Ugly, but faster.)
  napi_status GetCallbackInfo(
      napi_callback_info callbackInfo,
      size_t *argc,
      napi_value *argv,
      napi_value *thisArg,
      void **data) noexcept;

  napi_status GetNewTarget(napi_callback_info callbackInfo, napi_value *result) noexcept;

  napi_status DefineClass(
      const char *utf8Name,
      size_t length,
      napi_callback constructor,
      void *data,
      size_t propertyCount,
      const napi_property_descriptor *properties,
      napi_value *result) noexcept;

  napi_status
  Wrap(napi_value obj, void *nativeObj, napi_finalize finalizeCallback, void *finalizeHint, napi_ref *result) noexcept;

  napi_status Unwrap(napi_value js_object, void **result) noexcept;

  napi_status RemoveWrap(napi_value js_object, void **result) noexcept;
  napi_status CreateExternal(void *data, napi_finalize finalize_cb, void *finalize_hint, napi_value *result) noexcept;

  napi_status GetValueExternal(napi_value value, void **result) noexcept;

  napi_status CreateReference(napi_value value, uint32_t initialRefCount, napi_ref *result) noexcept;
  napi_status DeleteReference(napi_ref ref) noexcept;

  napi_status ReferenceRef(napi_ref ref, uint32_t *result) noexcept;

  napi_status ReferenceUnref(napi_ref ref, uint32_t *result) noexcept;
  napi_status GetReferenceValue(napi_ref ref, napi_value *result) noexcept;
  napi_status OpenHandleScope(napi_handle_scope *result) noexcept;
  napi_status CloseHandleScope(napi_handle_scope scope) noexcept;
  napi_status OpenEscapableHandleScope(napi_escapable_handle_scope *result) noexcept;
  napi_status CloseEscapableHandleScope(napi_escapable_handle_scope scope) noexcept;
  napi_status EscapeHandle(napi_escapable_handle_scope scope, napi_value escapee, napi_value *result) noexcept;

  napi_status Throw(napi_value error) noexcept;
  napi_status ThrowError(const char *code, const char *msg) noexcept;

  napi_status ThrowTypeError(const char *code, const char *msg) noexcept;

  napi_status ThrowRangeError(const char *code, const char *msg) noexcept;

  napi_status IsError(napi_value value, bool *result) noexcept;

  napi_status IsExceptionPending(bool *result) noexcept;
  napi_status GetAndClearLastException(napi_value *result) noexcept;

  napi_status IsArrayBuffer(napi_value value, bool *result) noexcept;
  napi_status CreateArrayBuffer(size_t byteLength, void **data, napi_value *result) noexcept;
  napi_status CreateExternalArrayBuffer(
      void *externalData,
      size_t byteLength,
      napi_finalize finalizeCallback,
      void *finalizeHint,
      napi_value *result) noexcept;

  napi_status GetArrayBufferInfo(napi_value arrayBuffer, void **data, size_t *byteLength) noexcept;
  napi_status IsTypedArray(napi_value value, bool *result) noexcept;

  napi_status CreateTypedArray(
      napi_typedarray_type type,
      size_t length,
      napi_value arrayBuffer,
      size_t byteOffset,
      napi_value *result) noexcept;

  napi_status GetTypedArrayInfo(
      napi_value typedArray,
      napi_typedarray_type *type,
      size_t *length,
      void **data,
      napi_value *arrayBuffer,
      size_t *byteOffset) noexcept;

  napi_status CreateDataView(size_t byteLength, napi_value arrayBuffer, size_t byteOffset, napi_value *result) noexcept;

  napi_status IsDataView(napi_value value, bool *result) noexcept;
  napi_status GetDataViewInfo(
      napi_value dataview,
      size_t *byteLength,
      void **data,
      napi_value *arrayBuffer,
      size_t *byteOffset) noexcept;

  napi_status GetVersion(uint32_t *result) noexcept;
  napi_status CreatePromise(napi_deferred *deferred, napi_value *promise) noexcept;

  napi_status ResolveDeferred(napi_deferred deferred, napi_value resolution) noexcept;
  napi_status RejectDeferred(napi_deferred deferred, napi_value rejection) noexcept;

  napi_status ConcludeDeferred(napi_deferred deferred, CachedPropertyId propertyId, napi_value result) noexcept;

  napi_status IsPromise(napi_value value, bool *is_promise) noexcept;

  napi_status RunScript(napi_value script, napi_value *result) noexcept;

  napi_status AdjustExternalMemory(int64_t changeInBytes, int64_t *adjustedValue) noexcept;

  napi_status CreateDate(double time, napi_value *result) noexcept;
  napi_status IsDate(napi_value value, bool *isDate) noexcept;

  napi_status GetDateValue(napi_value value, double *result) noexcept;

  napi_status AddFinalizer(
      napi_value jsObject,
      void *nativeObject,
      napi_finalize finalizeCallback,
      void *finalizeHint,
      napi_ref *result) noexcept;

  napi_status CreateBigIntInt64(int64_t value, napi_value *result) noexcept;

  napi_status CreateBigIntUInt64(uint64_t value, napi_value *result) noexcept;

  napi_status CreateBigIntWords(int sign_bit, size_t word_count, const uint64_t *words, napi_value *result) noexcept;

  napi_status GetValueBigIntInt64(napi_value value, int64_t *result, bool *isLossless) noexcept;

  napi_status GetValueBigIntUInt64(napi_value value, uint64_t *result, bool *isLossless) noexcept;

  napi_status GetValueBigIntWords(napi_value value, int *signBit, size_t *wordCount, uint64_t *words) noexcept;

  napi_status GetAllPropertyNames(
      napi_value object,
      napi_key_collection_mode keyMode,
      napi_key_filter keyFilter,
      napi_key_conversion keyConversion,
      napi_value *result) noexcept;

  napi_status SetInstanceData(void *data, napi_finalize finalizeCallback, void *finalizeHint) noexcept;

  napi_status GetInstanceData(void **data) noexcept;

  napi_status DetachArrayBuffer(napi_value arrayBuffer) noexcept;

  napi_status IsDetachedArrayBuffer(napi_value value, bool *result) noexcept;

  napi_status TypeTagObject(napi_value value, const napi_type_tag *typeTag) noexcept;

  napi_status CheckObjectTypeTag(napi_value value, const napi_type_tag *typeTag, bool *result) noexcept;

  napi_status ObjectFreeze(napi_value object) noexcept;

  napi_status ObjectSeal(napi_value object) noexcept;

  napi_status SerializeScript(const char *script, uint8_t *buffer, size_t *bufferSize) noexcept;

  napi_status
  RunSerializedScript(const char *script, uint8_t *buffer, const char *sourceUrl, napi_value *result) noexcept;

 private:
  static JsErrorCode ChakraPointerToString(std::wstring_view value, JsValueRef *result) noexcept;

  template <class TObject, class TPropertyId>
  static JsErrorCode ChakraGetProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept;

  template <class TObject, class TPropertyId>
  JsErrorCode ChakraGetBoolProperty(TObject &&object, TPropertyId &&propertyId, bool *result) noexcept;

  template <class TObject, class TPropertyId, class TValue>
  static JsErrorCode ChakraSetProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept;

  template <typename TValue>
  JsErrorCode ChakraCreatePropertyDescriptor(TValue &&value, PropertyAttibutes attrs, JsValueRef *result) noexcept;

  template <typename TObject, typename TPropertyId>
  static JsErrorCode ChakraDefineProperty(
      TObject &&object,
      TPropertyId &&propertyId,
      JsValueRef propertyDescriptor,
      bool *isSucceeded) noexcept;

  template <typename TObject, typename TPropertyId, typename TValue>
  JsErrorCode ChakraDefineProperty(
      TObject &&object,
      TPropertyId &&propertyId,
      TValue &&value,
      PropertyAttibutes attrs,
      bool *isSucceeded) noexcept;

  template <class TObject, class TPropertyId>
  JsErrorCode ChakraHasPrivateProperty(TObject &&object, TPropertyId &&propertyId, bool *result) noexcept;

  template <class TObject, class TPropertyId>
  JsErrorCode ChakraGetPrivateProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept;

  template <class TObject, class TPropertyId, class TValue>
  JsErrorCode ChakraSetPrivateProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept;

  template <typename TFunction, typename... TArgs>
  static JsErrorCode ChakraCallFunction(TFunction &&function, JsValueRef *result, TArgs &&... args) noexcept;

  template <typename TConstructor, typename... TArgs>
  static JsErrorCode ChakraConstructObject(TConstructor &&constructor, JsValueRef *result, TArgs &&... args) noexcept;

  JsErrorCode GetObject(JsValueRef *result) noexcept;
  JsErrorCode GetObjectFreeze(JsValueRef *result) noexcept;
  JsErrorCode GetObjectHasOwnProperty(JsValueRef *result) noexcept;
  JsErrorCode GetObjectPrototype(JsValueRef *result) noexcept;
  JsErrorCode GetObjectSeal(JsValueRef *result) noexcept;

  JsErrorCode ChakraCreatePromise(
      _Out_ JsValueRef *promise,
      _Out_ JsValueRef *resolveFunction,
      _Out_ JsValueRef *rejectFunction) noexcept;

 private:
  Microsoft::JSI::ChakraRuntimeArgs m_args;
  JsRuntimeHandle m_runtime;
  JsRefHolder m_context;
  JsRefHolder m_prevContext;

  napi_extended_error_info m_lastError{nullptr, nullptr, 0, napi_ok};

  // We store references in two different lists, depending on whether they have
  // `napi_finalizer` callbacks, because we must first finalize the ones that
  // have such a callback. See `~Environment()` above for details.
  RefTracker::RefList m_refList;
  RefTracker::RefList m_finalizingRefList;
  int m_refCount{1};
  JsSourceContext m_sourceContext{JS_SOURCE_CONTEXT_NONE};

  struct PropertyId final {
    CachedPropertyId Date{L"Date"};
    CachedPropertyId Object{L"Object"};
    CachedPropertyId Promise{L"Promise"};
    CachedPropertyId configurable{L"configurable"};
    CachedPropertyId enumerable{L"enumerable"};
    CachedPropertyId freeze{L"freeze"};
    CachedPropertyId hasOwnProperty{L"hasOwnProperty"};
    CachedPropertyId hostObject{L"hostObject", JsPropertyIdTypeSymbol};
    CachedPropertyId prototype{L"prototype"};
    CachedPropertyId reject{L"reject"};
    CachedPropertyId seal{L"seal"};
    CachedPropertyId tag{L"tag", JsPropertyIdTypeSymbol};
    CachedPropertyId resolve{L"resolve"};
    CachedPropertyId value{L"value"};
    CachedPropertyId valueOf{L"valueOf"};
    CachedPropertyId writable{L"writable"};
  } m_propertyId;

  struct Value final {
    Environment *m_env{};
    Value(Environment *env) : m_env{env} {}

    CachedValue False{JsGetFalseValue};
    CachedValue Global{JsGetGlobalObject};
    CachedValue Null{JsGetNullValue};
    CachedValue Undefined{JsGetUndefinedValue};
    CachedValue True{JsGetTrueValue};

    CachedValue Object{m_env, &GetObject};
    CachedValue ObjectFreeze{m_env, &GetObjectFreeze};
    CachedValue ObjectHasOwnProperty{m_env, &GetObjectHasOwnProperty};
    CachedValue ObjectPrototype{m_env, &GetObjectPrototype};
    CachedValue ObjectSeal{m_env, &GetObjectSeal};
  } m_value{this};

  struct UniqueString {
    napi_value value;
    std::wstring_view stringView;
  };

  static void CALLBACK FinalizeUniqueString(_In_ JsRef ref, _In_opt_ void *callbackState) {
    UniqueString *uniqueStr{};
    Environment *env = reinterpret_cast<Environment *>(callbackState);
    const auto it = env->m_uniqueStrings.find(reinterpret_cast<napi_value>(ref));
    if (it != env->m_uniqueStrings.end()) {
      uniqueStr = it->second;
      env->m_uniqueStrings.erase(it);

      const auto indexIt = env->m_uniqueStringIndex.find(uniqueStr->stringView);
      if (indexIt != env->m_uniqueStringIndex.end()) {
        env->m_uniqueStringIndex.erase(indexIt);
      }

      delete uniqueStr;
    }
  }

  std::unordered_map<napi_value, UniqueString *> m_uniqueStrings;
  std::unordered_map<std::wstring_view, UniqueString *> m_uniqueStringIndex;
};

napi_env MakeChakraNapiEnv(Microsoft::JSI::ChakraRuntimeArgs &&args) noexcept {
  return reinterpret_cast<napi_env>(new Environment(std::move(args)));
}

} // namespace chakra

// TODO: [vmoroz] Remove definition of napi_env__

// Pseudo alias for Environment. It must be fine as long as they have the same size.
struct napi_env__ : chakra::Environment {};
static_assert(sizeof(napi_env__) == sizeof(chakra::Environment));

namespace chakra {

// Adapter for napi_finalize callbacks.
struct Finalizer {
  // Some Finalizers are run during shutdown when the napi_env is destroyed,
  // and some need to keep an explicit reference to the napi_env because they
  // are run independently.
  enum class EnvReferenceMode { NoEnvReference, KeepEnvReference };

 protected:
  Finalizer(
      napi_env env,
      napi_finalize finalizeCallback,
      void *finalizeData,
      void *finalizeHint,
      EnvReferenceMode refMode = EnvReferenceMode::NoEnvReference) noexcept
      : m_env{env},
        m_finalizeCallback{finalizeCallback},
        m_finalizeData{finalizeData},
        m_finalizeHint(finalizeHint),
        m_hasEnvReference(refMode == EnvReferenceMode::KeepEnvReference) {
    if (m_hasEnvReference) {
      m_env->Ref();
    }
  }

  ~Finalizer() noexcept {
    if (m_hasEnvReference) {
      m_env->Unref();
    }
  }

 public:
  static Finalizer *New(
      napi_env env,
      napi_finalize finalizeCallback = nullptr,
      void *finalizeData = nullptr,
      void *finalizeHint = nullptr,
      EnvReferenceMode refMode = EnvReferenceMode::NoEnvReference) noexcept {
    return new Finalizer(env, finalizeCallback, finalizeData, finalizeHint, refMode);
  }

  static void Delete(Finalizer *finalizer) noexcept {
    delete finalizer;
  }

 protected:
  napi_env m_env{nullptr};
  napi_finalize m_finalizeCallback{nullptr};
  void *m_finalizeData{nullptr};
  void *m_finalizeHint{nullptr};
  bool m_didFinalizeRun{false};
  bool m_hasEnvReference{false};
};

// TODO: [vmoroz] Separate implementation from the declaration.
struct Reference : RefTracker {
  // We use std::unique_ptr to avoid memory leaks after allocation.
  friend std::default_delete<Reference>;

  static napi_status New(napi_env env, napi_value value, uint32_t initialRefCount, napi_ref *result) noexcept {
    CHECK_ENV_ARG(env, value);
    CHECK_ENV_ARG(env, result);

    JsValueRef jsValue{reinterpret_cast<JsValueRef>(value)};

    // JsValueType jsValueType{JsValueType::JsUndefined};
    // CHECK_ENV_JSRT(env, JsGetValueType(jsValue, &jsValueType));
    // RETURN_ENV_STATUS_IF_FALSE(env, jsValueType >= JsValueType::JsObject, napi_status::napi_object_expected);

    // Allocate new Reference and make sure that it is not null.
    auto ref = std::unique_ptr<Reference>{new (std::nothrow) Reference{
        jsValue, initialRefCount, /*hasBeforeCollectCallback:*/ initialRefCount == 0, /*shouldDeleteSelf:*/ false}};
    RETURN_ENV_STATUS_IF_FALSE(env, ref, napi_status::napi_generic_failure);

    if (initialRefCount == 0) {
      CHECK_ENV_JSRT(env, JsSetObjectBeforeCollectCallback(jsValue, ref.get(), BeforeCollectCallback));
    } else {
      CHECK_ENV_JSRT(env, JsAddRef(jsValue, nullptr));
    }

    env->LinkReference(ref.get());
    *result = reinterpret_cast<napi_ref>(ref.release());
    return napi_status::napi_ok;
  }

  napi_status Delete(napi_env env) noexcept {
    // Delete must not be called if we expect it to be deleted by Finalizer
    RETURN_ENV_STATUS_IF_FALSE(env, !m_shouldDeleteSelf, napi_status::napi_generic_failure);

    // Only delete if the BeforeCollectCallback is not set
    // or if it is already run and value is removed.
    if (!m_hasBeforeCollectCallback || !m_value) {
      delete this;
    } else {
      // Defer until BeforeCollectCallback runs.
      m_shouldDeleteSelf = true;
    }

    return napi_status::napi_ok;
  }

  napi_status Ref(napi_env env, uint32_t *result) noexcept {
    if (m_value) {
      if (m_refCount == 0) {
        CHECK_ENV_JSRT(env, JsAddRef(m_value, nullptr));
      }
      ++m_refCount;
    }

    if (result) {
      *result = m_refCount;
    }

    return napi_status::napi_ok;
  }

  napi_status Unref(napi_env env, uint32_t *result) noexcept {
    RETURN_ENV_STATUS_IF_FALSE(env, m_refCount > 0, napi_status::napi_generic_failure);

    --m_refCount;
    if (m_value) {
      if (m_refCount == 0) {
        if (!m_hasBeforeCollectCallback) {
          CHECK_ENV_JSRT(env, JsSetObjectBeforeCollectCallback(m_value, this, BeforeCollectCallback));
          m_hasBeforeCollectCallback = true;
        }

        CHECK_ENV_JSRT(env, JsRelease(m_value, nullptr));
      }
    }

    if (result) {
      *result = m_refCount;
    }

    return napi_status::napi_ok;
  }

  napi_status Value(napi_env env, napi_value *result) noexcept {
    CHECK_ENV_ARG(env, result);

    *result = reinterpret_cast<napi_value>(m_value);
    return napi_status::napi_ok;
  }

 protected:
  Reference(JsValueRef value, uint32_t refCount, bool hasBeforeCollectCallback, bool shouldDeleteSelf) noexcept
      : m_value{value},
        m_refCount{refCount},
        m_hasBeforeCollectCallback{hasBeforeCollectCallback},
        m_shouldDeleteSelf{shouldDeleteSelf} {}

  ~Reference() noexcept override {
    Unlink();
  }

  static void BeforeCollectCallback(_In_ JsRef /*ref*/, _In_opt_ void *callbackState) {
    if (callbackState != nullptr) {
      Reference *reference = static_cast<Reference *>(callbackState);
      reference->m_value = JS_INVALID_REFERENCE;
      reference->Finalize(/*isEnvTeardown:*/ false);
    }
  }

  void Finalize(bool isEnvTeardown) noexcept override {
    // We delete here if we do not expect the Delete function to run anymore.
    if (m_shouldDeleteSelf || isEnvTeardown) {
      delete this;
    }
  }

 private:
  JsValueRef m_value{JS_INVALID_REFERENCE};
  uint32_t m_refCount{1};
  bool m_hasBeforeCollectCallback{false};
  bool m_shouldDeleteSelf{false};
};

struct FinalizingReference : Reference {
  // We use std::unique_ptr to avoid memory leaks after allocation.
  friend std::default_delete<Reference>;

  static napi_status New(
      Environment *env,
      napi_value value,
      bool shouldDeleteSelf,
      napi_finalize finalizeCallback,
      void *finalizeData,
      void *finalizeHint,
      napi_ref *result) noexcept {
    CHECK_ENV_ARG(env, value);

    JsValueRef jsValue{reinterpret_cast<JsValueRef>(value)};

    JsValueType jsValueType{JsValueType::JsUndefined};
    CHECK_ENV_JSRT(env, JsGetValueType(jsValue, &jsValueType));
    if (jsValueType < JsValueType::JsObject) {
      return env->SetLastError(napi_status::napi_object_expected);
    }

    // Allocate new Reference and make sure that it is not null.
    auto ref = std::unique_ptr<FinalizingReference>{new (std::nothrow) FinalizingReference{
        env, jsValue, shouldDeleteSelf, finalizeCallback, finalizeData, finalizeHint}};
    RETURN_ENV_STATUS_IF_FALSE(env, ref, napi_status::napi_generic_failure);

    CHECK_ENV_JSRT(env, JsSetObjectBeforeCollectCallback(jsValue, ref.get(), BeforeCollectCallback));

    env->LinkFinalizingReference(ref.get());
    if (result) {
      *result = reinterpret_cast<napi_ref>(ref.get());
    }

    ref.release();
    return napi_status::napi_ok;
  }

  void *Data() {
    return m_finalizeData;
  }

 protected:
  using Super = Reference;

  FinalizingReference(
      Environment *env,
      JsValueRef value,
      bool shouldDeleteSelf,
      napi_finalize finalizeCallback,
      void *finalizeData,
      void *finalizeHint) noexcept
      : Super{value, /*refCount:*/ 0, /*hasBeforeCollectCallback:*/ true, shouldDeleteSelf},
        m_env(reinterpret_cast<napi_env>(env)),
        m_finalizeCallback{finalizeCallback},
        m_finalizeData{finalizeData},
        m_finalizeHint{finalizeHint} {}

  void Finalize(bool isEnvTeardown) noexcept override {
    if (m_finalizeCallback) {
      m_finalizeCallback(m_env, m_finalizeData, m_finalizeHint);
    }
    Super::Finalize(isEnvTeardown);
  }

 private:
  napi_env m_env{nullptr};
  napi_finalize m_finalizeCallback{nullptr};
  void *m_finalizeData{nullptr};
  void *m_finalizeHint{nullptr};
};

//=============================================================================
// JsRefHolder implementation
//=============================================================================

JsRefHolder::JsRefHolder(std::nullptr_t) noexcept {}

JsRefHolder::JsRefHolder(JsRef ref) noexcept : m_ref{ref} {
  if (m_ref) {
    // TODO: [vmoroz] How to handle error here?
    JsAddRef(m_ref, nullptr);
  }
}

JsRefHolder::JsRefHolder(JsRefHolder const &other) noexcept : m_ref{other.m_ref} {
  if (m_ref) {
    // TODO: [vmoroz] How to handle error here?
    JsAddRef(m_ref, nullptr);
  }
}

JsRefHolder::JsRefHolder(JsRefHolder &&other) noexcept : m_ref{std::exchange(other.m_ref, JS_INVALID_REFERENCE)} {}

JsRefHolder &JsRefHolder::operator=(JsRefHolder const &other) noexcept {
  if (this != &other) {
    JsRefHolder temp{std::move(*this)};
    m_ref = other.m_ref;
    if (m_ref) {
      // TODO: [vmoroz] How to handle error here?
      JsAddRef(m_ref, nullptr);
    }
  }

  return *this;
}

JsRefHolder &JsRefHolder::operator=(JsRefHolder &&other) noexcept {
  if (this != &other) {
    JsRefHolder temp{std::move(*this)};
    m_ref = std::exchange(other.m_ref, JS_INVALID_REFERENCE);
  }

  return *this;
}

JsRefHolder::~JsRefHolder() noexcept {
  if (m_ref) {
    // Clear m_ref before calling JsRelease on it to make sure that we always hold a valid m_ref.
    // TODO: [vmoroz] How to handle error here?
    JsRelease(std::exchange(m_ref, JS_INVALID_REFERENCE), nullptr);
  }
}

// Callback Info struct as per JSRT native function.
struct CallbackInfo {
  napi_value newTarget;
  napi_value thisArg;
  napi_value *argv;
  void *data;
  uint16_t argc;
  bool isConstructCall;
};

// Adapter for JSRT external data + finalize callback.
class ExternalData {
 public:
  ExternalData(Environment *env, void *data, napi_finalize finalize_cb, void *hint)
      : _env(reinterpret_cast<napi_env>(env)), _data(data), _cb(finalize_cb), _hint(hint) {}

  void *Data() {
    return _data;
  }

  // JsFinalizeCallback
  static void CALLBACK Finalize(void *callbackState) {
    ExternalData *externalData = reinterpret_cast<ExternalData *>(callbackState);
    if (externalData != nullptr) {
      if (externalData->_cb != nullptr) {
        externalData->_cb(externalData->_env, externalData->_data, externalData->_hint);
      }

      delete externalData;
    }
  }

 private:
  napi_env _env;
  void *_data;
  napi_finalize _cb;
  void *_hint;
};

// Adapter for JSRT external callback + callback data.
class ExternalCallback {
 public:
  ExternalCallback(Environment *env, napi_callback cb, void *data)
      : _env(reinterpret_cast<napi_env>(env)), _cb(cb), _data(data) {}

  // JsNativeFunction
  static JsValueRef CALLBACK Callback(
      JsValueRef callee,
      bool isConstructCall,
      JsValueRef *arguments,
      unsigned short argumentCount,
      void *callbackState) {
    ExternalCallback *externalCallback = reinterpret_cast<ExternalCallback *>(callbackState);

    // Make sure any errors encountered last time we were in N-API are gone.
    externalCallback->_env->ClearLastError();

    CallbackInfo cbInfo;
    cbInfo.thisArg = reinterpret_cast<napi_value>(arguments[0]);
    cbInfo.newTarget = reinterpret_cast<napi_value>(externalCallback->newTarget);
    cbInfo.isConstructCall = isConstructCall;
    cbInfo.argc = argumentCount - 1;
    cbInfo.argv = reinterpret_cast<napi_value *>(arguments + 1);
    cbInfo.data = externalCallback->_data;

    napi_value result = externalCallback->_cb(externalCallback->_env, reinterpret_cast<napi_callback_info>(&cbInfo));
    return reinterpret_cast<JsValueRef>(result);
  }

  // JsObjectBeforeCollectCallback
  static void CALLBACK Finalize(JsRef ref, void *callbackState) {
    ExternalCallback *externalCallback = reinterpret_cast<ExternalCallback *>(callbackState);
    delete externalCallback;
  }

  // Value for 'new.target'
  JsValueRef newTarget;

 private:
  napi_env _env;
  napi_callback _cb;
  void *_data;
};

// Adapter for NAPI finalizer.
class FinalizerInfo {
 public:
  // ExternalCallback(napi_env env, napi_callback cb, void *data) : _env(env), _cb(cb), _data(data) {}

  // JsObjectBeforeCollectCallback
  static void CALLBACK Finalize(JsRef ref, void *callbackState) {
    ExternalCallback *externalCallback = reinterpret_cast<ExternalCallback *>(callbackState);
    delete externalCallback;
  }

  // Value for 'new.target'
  JsValueRef newTarget;

 private:
  napi_env _env;
  napi_callback _cb;
  void *_data;
};

namespace {

std::wstring NarrowToWide(std::string_view value, UINT codePage = CP_UTF8) {
  if (value.size() == 0) {
    return {};
  }

  int requiredSize = ::MultiByteToWideChar(codePage, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  assert(requiredSize != 0);
  std::wstring wstr(requiredSize, 0);
  int result = ::MultiByteToWideChar(codePage, 0, value.data(), static_cast<int>(value.size()), &wstr[0], requiredSize);
  assert(result != 0);
  return wstr;
}

JsErrorCode JsCreateString(_In_ const char *content, _In_ size_t length, _Out_ JsValueRef *value) {
  auto str = (length == NAPI_AUTO_LENGTH ? NarrowToWide({content}) : NarrowToWide({content, length}));
  return JsPointerToString(str.data(), str.size(), value);
}

JsErrorCode JsCopyString(
    _In_ JsValueRef value,
    _Out_opt_ char *buffer,
    _In_ size_t bufferSize,
    _Out_opt_ size_t *length,
    UINT codePage = CP_UTF8) noexcept {
  const wchar_t *stringValue;
  size_t stringLength;
  CHECK_JSRT_ERROR_CODE(JsStringToPointer(value, &stringValue, &stringLength));

  int result = 0;
  if (stringLength != 0) {
    result = ::WideCharToMultiByte(
        codePage,
        0,
        stringValue,
        static_cast<int>(stringLength),
        buffer,
        static_cast<int>(bufferSize),
        nullptr,
        nullptr);
  }

  if (length != nullptr) {
    *length = static_cast<size_t>(result);
  }

  return JsErrorCode::JsNoError;
}

JsErrorCode
JsCopyStringUtf16(_In_ JsValueRef value, _Out_opt_ char16_t *buffer, _In_ size_t bufferSize, _Out_opt_ size_t *length) {
  const wchar_t *stringValue;
  size_t stringLength;
  CHECK_JSRT_ERROR_CODE(JsStringToPointer(value, &stringValue, &stringLength));

  if (buffer == nullptr) {
    if (length != nullptr) {
      *length = stringLength;
    }
  } else {
    size_t copied = (std::min)(bufferSize, stringLength);
    if (length != nullptr) {
      *length = copied;
    }

    static_assert(sizeof(char16_t) == sizeof(wchar_t));
    memcpy_s(buffer, bufferSize * sizeof(wchar_t), stringValue, copied * sizeof(wchar_t));
  }

  return JsErrorCode::JsNoError;
}

JsErrorCode JsCreatePropertyId(_In_z_ const char *name, _In_ size_t length, _Out_ JsPropertyIdRef *propertyId) {
  auto str = (length == NAPI_AUTO_LENGTH ? NarrowToWide({name}) : NarrowToWide({name, length}));
  return JsGetPropertyIdFromName(str.data(), propertyId);
}

JsErrorCode JsPropertyIdFromKey(JsValueRef key, JsPropertyIdRef *propertyId) {
  JsValueType keyType;
  CHECK_JSRT_ERROR_CODE(JsGetValueType(key, &keyType));

  if (keyType == JsString) {
    const wchar_t *stringValue;
    size_t stringLength;
    CHECK_JSRT_ERROR_CODE(JsStringToPointer(key, &stringValue, &stringLength));
    CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromName(stringValue, propertyId));
  } else if (keyType == JsSymbol) {
    CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromSymbol(key, propertyId));
  } else {
    return JsErrorCode::JsErrorInvalidArgument;
  }
  return JsErrorCode::JsNoError;
}

JsErrorCode JsPropertyIdFromPropertyDescriptor(const napi_property_descriptor *p, JsPropertyIdRef *propertyId) {
  if (p->utf8name != nullptr) {
    return JsCreatePropertyId(p->utf8name, strlen(p->utf8name), propertyId);
  } else {
    return JsPropertyIdFromKey(p->name, propertyId);
  }
}

JsErrorCode JsNameValueFromPropertyDescriptor(const napi_property_descriptor *p, napi_value *name) {
  if (p->utf8name != nullptr) {
    return JsCreateString(p->utf8name, NAPI_AUTO_LENGTH, reinterpret_cast<JsValueRef *>(name));
  } else {
    *name = p->name;
    return JsErrorCode::JsNoError;
  }
}

// A span of values that can be used to pass arguments to function.
// For C++20 we should consider to replace it with std::span.
template <typename T>
struct Span final {
  constexpr Span(std::initializer_list<T> il) noexcept : m_data{const_cast<T *>(il.begin())}, m_size{il.size()} {}
  constexpr Span(T const *data, size_t size) noexcept : m_data{data}, m_size{size} {}

  [[nodiscard]] constexpr T const *begin() const noexcept {
    return m_data;
  }

  [[nodiscard]] constexpr T const *end() const noexcept {
    return m_data + m_size;
  }

  [[nodiscard]] constexpr size_t size() const noexcept {
    return m_size;
  }

 private:
  T const *const m_data;
  size_t const m_size;
};

// JsValueArgs helps to optimize passing arguments to Chakra function.
// If number of arguments is below or equal to MaxStackArgCount,
// then they are kept on call stack, otherwise arguments are allocated on heap.
struct JsValueArgs final {
  JsValueArgs(napi_value thisArg, Span<napi_value> args) noexcept
      : m_count{args.size() + 1},
        m_heapArgs{m_count > MaxStackArgCount ? std::make_unique<JsValueRef[]>(m_count) : nullptr} {
    JsValueRef *const jsArgs = m_heapArgs ? m_heapArgs.get() : m_stackArgs.data();
    jsArgs[0] = reinterpret_cast<JsValueRef>(thisArg);
    for (size_t i = 1; i < m_count; ++i) {
      jsArgs[i] = reinterpret_cast<JsValueRef>(args.begin()[i - 1]);
    }
  }

  JsValueRef *Data() noexcept {
    return m_heapArgs ? m_heapArgs.get() : m_stackArgs.data();
  }

  size_t Size() noexcept {
    return m_count;
  };

 private:
  static constexpr size_t MaxStackArgCount = 8;

  size_t const m_count{};
  std::array<JsValueRef, MaxStackArgCount> m_stackArgs{{JS_INVALID_REFERENCE}};
  std::unique_ptr<JsValueRef[]> const m_heapArgs;
};

#if 0
template <WrapType wrapType>
inline napi_status Wrap(
    napi_env env,
    napi_value js_object,
    void *native_object,
    napi_finalize finalize_cb,
    void *finalize_hint,
    napi_ref *result) {
  CHECK_ENV_AND_ARG(env)env, js_object);

  // v8::Local<v8::Context> context = env->context();

  JsValueRef value = reinterpret_cast<JsValueRef>(js_object);
  // RETURN_STATUS_IF_FALSE(env, value->IsObject(), napi_invalid_arg);
  // v8::Local<v8::Object> obj = value.As<v8::Object>();

  if (wrapType == WrapType::Retrievable) {
    // If we've already wrapped this object, we error out.
    RETURN_STATUS_IF_FALSE(
        env, !obj->HasPrivate(context, NAPI_PRIVATE_KEY(context, wrapper)).FromJust(), napi_invalid_arg);
  } else if (wrapType == WrapType::Anonymous) {
    // If no finalize callback is provided, we error out.
    CHECK_ARG(finalize_cb);
  }

  v8impl::Reference *reference = nullptr;
  if (result != nullptr) {
    // The returned reference should be deleted via napi_delete_reference()
    // ONLY in response to the finalize callback invocation. (If it is deleted
    // before then, then the finalize callback will never be invoked.)
    // Therefore a finalize callback is required when returning a reference.
    CHECK_ARG(finalize_cb);
    reference = v8impl::Reference::New(env, obj, 0, false, finalize_cb, native_object, finalize_hint);
    *result = reinterpret_cast<napi_ref>(reference);
  } else {
    // Create a self-deleting reference.
    reference = v8impl::Reference::New(
        env, obj, 0, true, finalize_cb, native_object, finalize_cb == nullptr ? nullptr : finalize_hint);
  }

  if (wrap_type == retrievable) {
    CHECK(obj->SetPrivate(context, NAPI_PRIVATE_KEY(context, wrapper), v8::External::New(env->isolate, reference))
              .FromJust());
  }

  return GET_RETURN_STATUS(env);
}
#endif

struct RefInfo {
  JsValueRef value;
  uint32_t count;
};

struct DataViewInfo {
  JsValueRef dataView;
  JsValueRef arrayBuffer;
  size_t byteOffset;
  size_t byteLength;

  static void CALLBACK Finalize(_In_opt_ void *data) {
    delete reinterpret_cast<DataViewInfo *>(data);
  }
};

} // namespace

//=============================================================================
// Environment implementation
//=============================================================================

Environment::Environment(Microsoft::JSI::ChakraRuntimeArgs &&args) noexcept : m_args{std::move(args)} {
  JsRuntimeAttributes runtimeAttributes = JsRuntimeAttributeNone;

  if (!m_args.enableJITCompilation) {
    runtimeAttributes = static_cast<JsRuntimeAttributes>(
        runtimeAttributes | JsRuntimeAttributeDisableNativeCodeGeneration |
        JsRuntimeAttributeDisableExecutablePageAllocation);
  }

  // TODO: [vmoroz] add error handling
  JsCreateRuntime(runtimeAttributes, nullptr, &m_runtime);

  // setupMemoryTracker();

  JsValueRef context{};
  JsCreateContext(m_runtime, &context);
  m_context = JsRefHolder{context};

  // Note :: We currently assume that the runtime will be created and
  // exclusively used in a single thread.
  // Preserve the current context if it is already associated with the thread.
  JsValueRef currentContext{};
  JsGetCurrentContext(&currentContext);
  m_prevContext = JsRefHolder{currentContext};

  JsSetCurrentContext(context);

  // startDebuggingIfNeeded();

  // setupNativePromiseContinuation();

  // std::call_once(s_runtimeVersionInitFlag, initRuntimeVersion);

  // m_propertyId.Object = JsRefHolder{GetPropertyIdFromName(L"Object")};
  // m_propertyId.Proxy = JsRefHolder{GetPropertyIdFromName(L"Proxy")};
  // m_propertyId.Symbol = JsRefHolder{GetPropertyIdFromName(L"Symbol")};
  // m_propertyId.byteLength = JsRefHolder{GetPropertyIdFromName(L"byteLength")};
  // m_propertyId.configurable = JsRefHolder{GetPropertyIdFromName(L"configurable")};
  // m_propertyId.enumerable = JsRefHolder{GetPropertyIdFromName(L"enumerable")};
  // m_propertyId.get = JsRefHolder{GetPropertyIdFromName(L"get")};
  // m_propertyId.hostFunctionSymbol = JsRefHolder{GetPropertyIdFromSymbol(L"hostFunctionSymbol")};
  // m_propertyId.hostObjectSymbol = JsRefHolder{GetPropertyIdFromSymbol(L"hostObjectSymbol")};
  // m_propertyId.length = JsRefHolder{GetPropertyIdFromName(L"length")};
  // m_propertyId.message = JsRefHolder{GetPropertyIdFromName(L"message")};
  // m_propertyId.ownKeys = JsRefHolder{GetPropertyIdFromName(L"ownKeys")};
  // m_propertyId.propertyIsEnumerable = JsRefHolder{GetPropertyIdFromName(L"propertyIsEnumerable")};
  // m_propertyId.prototype = JsRefHolder{GetPropertyIdFromName(L"prototype")};
  // m_propertyId.set = JsRefHolder{GetPropertyIdFromName(L"set")};
  // m_propertyId.toString = JsRefHolder{GetPropertyIdFromName(L"toString")};
  // m_propertyId.value = JsRefHolder{GetPropertyIdFromName(L"value")};
  // m_propertyId.writable = JsRefHolder{GetPropertyIdFromName(L"writable")};

  // m_undefinedValue = JsRefHolder{GetUndefinedValue()};
}

Environment::~Environment() noexcept {
  // First we must finalize those references that have `napi_finalizer`
  // callbacks. The reason is that addons might store other references which
  // they delete during their `napi_finalizer` callbacks. If we deleted such
  // references here first, they would be doubly deleted when the
  // `napi_finalizer` deleted them subsequently.
  RefTracker::FinalizeAll(&m_finalizingRefList);
  RefTracker::FinalizeAll(&m_refList);
}

JsContextRef Environment::Context() const noexcept {
  return static_cast<JsRef>(m_context);
}

void Environment::Ref() noexcept {
  ++m_refCount;
}

void Environment::Unref() noexcept {
  if (--m_refCount == 0) {
    delete this;
  }
}

void Environment::LinkReference(RefTracker::RefList *reference) noexcept {
  reference->Link(&m_refList);
}

void Environment::LinkFinalizingReference(RefTracker::RefList *reference) noexcept {
  reference->Link(&m_finalizingRefList);
}

JsErrorCode Environment::ChakraPointerToString(std::wstring_view value, JsValueRef *result) noexcept {
  return JsPointerToString(value.data(), value.size(), result);
}

napi_status Environment::SetErrorCode(JsValueRef error, napi_value code, const char *codeString) noexcept {
  if ((code != nullptr) || (codeString != nullptr)) {
    JsValueRef codeValue = reinterpret_cast<JsValueRef>(code);
    if (codeValue != JS_INVALID_REFERENCE) {
      JsValueType valueType = JsUndefined;
      CHECK_JSRT(JsGetValueType(codeValue, &valueType));
      RETURN_STATUS_IF_FALSE(valueType == JsString, napi_string_expected);
    } else {
      CHECK_JSRT(JsCreateString(codeString, NAPI_AUTO_LENGTH, &codeValue));
    }

    JsPropertyIdRef codePropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsGetPropertyIdFromName(L"code", &codePropId));

    CHECK_JSRT(JsSetProperty(error, codePropId, codeValue, true));

    JsValueRef nameArray{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsCreateArray(0, &nameArray));

    JsPropertyIdRef pushPropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsGetPropertyIdFromName(L"push", &pushPropId));

    JsValueRef pushFunction{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsGetProperty(nameArray, pushPropId, &pushFunction));

    JsPropertyIdRef namePropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsGetPropertyIdFromName(L"name", &namePropId));

    bool hasProp = false;
    CHECK_JSRT(JsHasProperty(error, namePropId, &hasProp));

    JsValueRef nameValue{JS_INVALID_REFERENCE};
    std::array<JsValueRef, 2> args = {nameArray, JS_INVALID_REFERENCE};

    if (hasProp) {
      CHECK_JSRT(JsGetProperty(error, namePropId, &nameValue));

      args[1] = nameValue;
      CHECK_JSRT(JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));
    }

    JsValueRef openBracketValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsPointerToString(STR_AND_LENGTH(L" ["), &openBracketValue));

    args[1] = openBracketValue;
    CHECK_JSRT(JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    args[1] = codeValue;
    CHECK_JSRT(JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    JsValueRef closeBracketValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsPointerToString(STR_AND_LENGTH(L"]"), &closeBracketValue));

    args[1] = closeBracketValue;
    CHECK_JSRT(JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    JsValueRef emptyValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsPointerToString(L"", 0, &emptyValue));

    JsPropertyIdRef joinPropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsGetPropertyIdFromName(L"join", &joinPropId));

    JsValueRef joinFunction = JS_INVALID_REFERENCE;
    CHECK_JSRT(JsGetProperty(nameArray, joinPropId, &joinFunction));

    args[1] = emptyValue;
    CHECK_JSRT(JsCallFunction(joinFunction, args.data(), static_cast<unsigned short>(args.size()), &nameValue));

    CHECK_JSRT(JsSetProperty(error, namePropId, nameValue, true));
  }
  return napi_ok;
}

napi_status Environment::CreatePropertyFunction(
    napi_value propertyName,
    napi_callback callback,
    void *callbackData,
    napi_value *result) noexcept {
  CHECK_ARG(propertyName);
  CHECK_ARG(result);

  // TODO: [vmoroz] avoid exception
  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(this, callback, callbackData)};

  napi_valuetype nameType;
  CHECK_NAPI(TypeOf(propertyName, &nameType));

  JsValueRef function;
  if (nameType == napi_string) {
    JsValueRef name{JS_INVALID_REFERENCE};
    name = propertyName;
    CHECK_JSRT(JsCreateNamedFunction(name, ExternalCallback::Callback, externalCallback.get(), &function));
  } else {
    CHECK_JSRT(JsCreateFunction(ExternalCallback::Callback, externalCallback.get(), &function));
  }

  externalCallback->newTarget = function;

  CHECK_JSRT(JsSetObjectBeforeCollectCallback(function, externalCallback.get(), ExternalCallback::Finalize));
  externalCallback.release();

  *result = reinterpret_cast<napi_value>(function);
  return napi_ok;
}

// TODO: [vmoroz] Remove?
napi_status Unwrap(
    napi_env env,
    JsValueRef obj,
    ExternalData **externalData,
    JsValueRef *wrapper = nullptr,
    JsValueRef *parent = nullptr) {
  // JsValueRef candidate = JS_INVALID_REFERENCE;
  // JsValueRef candidateParent = JS_INVALID_REFERENCE;
  // CHECK_NAPI(FindWrapper(env, obj, &candidate, &candidateParent));
  // RETURN_STATUS_IF_FALSE(env, candidate != JS_INVALID_REFERENCE, napi_invalid_arg);

  // CHECK_JSRT(JsGetExternalData(candidate, reinterpret_cast<void **>(externalData)));

  // if (wrapper != nullptr) {
  //  *wrapper = candidate;
  //}

  // if (parent != nullptr) {
  //  *parent = candidateParent;
  //}

  return napi_ok;
}

template <class TObject, class TPropertyId>
JsErrorCode Environment::ChakraGetProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept {
  JsValueRef jsObject{};
  JsPropertyIdRef jsPropertyId{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  return JsGetProperty(jsObject, jsPropertyId, result);
}

template <class TObject, class TPropertyId>
JsErrorCode Environment::ChakraGetBoolProperty(TObject &&object, TPropertyId &&propertyId, bool *result) noexcept {
  JsValueRef value{};
  CHECK_JSRT_ERROR_CODE(
      ChakraGetProperty(std::forward<TObject>(object), std::forward<TPropertyId>(propertyId), &value));
  return JsBooleanToBool(value, result);
}

template <class TObject, class TPropertyId, class TValue>
JsErrorCode Environment::ChakraSetProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept {
  JsValueRef jsObject{};
  JsPropertyIdRef jsPropertyId{};
  JsValueRef jsValue{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TValue>(value), &jsValue));
  return JsSetProperty(jsObject, jsPropertyId, jsValue, /*useStrictRules:*/ true);
}

template <typename TValue>
JsErrorCode
Environment::ChakraCreatePropertyDescriptor(TValue &&value, PropertyAttibutes attrs, JsValueRef *result) noexcept {
  JsValueRef descriptor{};
  CHECK_JSRT_ERROR_CODE(JsCreateObject(&descriptor));
  CHECK_JSRT_ERROR_CODE(ChakraSetProperty(descriptor, m_propertyId.value, std::forward<TValue>(value)));
  if (!(attrs & PropertyAttibutes::ReadOnly)) {
    CHECK_JSRT_ERROR_CODE(ChakraSetProperty(descriptor, m_propertyId.writable, m_value.True));
  }
  if (!(attrs & PropertyAttibutes::DontEnum)) {
    CHECK_JSRT_ERROR_CODE(ChakraSetProperty(descriptor, m_propertyId.enumerable, m_value.True));
  }
  if (!(attrs & PropertyAttibutes::DontDelete)) {
    CHECK_JSRT_ERROR_CODE(ChakraSetProperty(descriptor, m_propertyId.configurable, m_value.True));
  }
  *result = descriptor;
  return JsErrorCode::JsNoError;
}

template <typename TObject, typename TPropertyId>
JsErrorCode Environment::ChakraDefineProperty(
    TObject &&object,
    TPropertyId &&propertyId,
    JsValueRef propertyDescriptor,
    bool *isSucceeded) noexcept {
  JsValueRef jsObject{};
  JsPropertyIdRef jsPropertyId{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  return JsDefineProperty(jsObject, jsPropertyId, propertyDescriptor, isSucceeded);
}

template <typename TObject, typename TPropertyId, typename TValue>
JsErrorCode Environment::ChakraDefineProperty(
    TObject &&object,
    TPropertyId &&propertyId,
    TValue &&value,
    PropertyAttibutes attrs,
    bool *isSucceeded) noexcept {
  JsValueRef descriptor{};
  CHECK_JSRT_ERROR_CODE(ChakraCreatePropertyDescriptor(std::forward<TValue>(value), attrs, &descriptor));
  return ChakraDefineProperty(
      std::forward<TObject>(object), std::forward<TPropertyId>(propertyId), descriptor, isSucceeded);
}

JsErrorCode Environment::GetObject(JsValueRef *result) noexcept {
  return ChakraGetProperty(m_value.Global, m_propertyId.Object, result);
}

JsErrorCode Environment::GetObjectPrototype(JsValueRef *result) noexcept {
  return ChakraGetProperty(m_value.Object, m_propertyId.prototype, result);
}

JsErrorCode Environment::GetObjectHasOwnProperty(JsValueRef *result) noexcept {
  return ChakraGetProperty(m_value.ObjectPrototype, m_propertyId.hasOwnProperty, result);
}

JsErrorCode Environment::GetObjectFreeze(JsValueRef *result) noexcept {
  return ChakraGetProperty(m_value.Object, m_propertyId.freeze, result);
}

JsErrorCode Environment::GetObjectSeal(JsValueRef *result) noexcept {
  return ChakraGetProperty(m_value.Object, m_propertyId.seal, result);
}

template <class TObject, class TPropertyId>
JsErrorCode Environment::ChakraHasPrivateProperty(TObject &&object, TPropertyId &&propertyId, bool *result) noexcept {
  JsValueRef jsObject{}, descriptor{};
  JsPropertyIdRef jsPropertyId{};
  JsValueType descriptorType{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  CHECK_JSRT_ERROR_CODE(JsGetOwnPropertyDescriptor(jsObject, jsPropertyId, &descriptor));
  CHECK_JSRT_ERROR_CODE(JsGetValueType(descriptor, &descriptorType));
  *result = descriptorType == JsValueType::JsObject;
  return JsErrorCode::JsNoError;
}

template <class TObject, class TPropertyId>
JsErrorCode
Environment::ChakraGetPrivateProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept {
  JsValueRef jsObject{}, descriptor{};
  JsPropertyIdRef jsPropertyId{};
  JsValueType descriptorType{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  CHECK_JSRT_ERROR_CODE(JsGetOwnPropertyDescriptor(jsObject, jsPropertyId, &descriptor));
  CHECK_JSRT_ERROR_CODE(JsGetValueType(descriptor, &descriptorType));
  if (descriptorType == JsValueType::JsUndefined) {
    *result = descriptor;
    return JsErrorCode::JsNoError;
  }
  return ChakraGetProperty(descriptor, m_propertyId.value, result);
}

template <class TObject, class TPropertyId, class TValue>
JsErrorCode Environment::ChakraSetPrivateProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept {
  JsValueRef descriptor{};
  bool isSucceeded{false};
  CHECK_JSRT_ERROR_CODE(ChakraDefineProperty(
      std::forward<TObject>(object),
      std::forward<TPropertyId>(propertyId),
      std::forward<TValue>(value),
      PropertyAttibutes::DontEnum,
      &isSucceeded));
  if (isSucceeded) {
    return JsErrorCode::JsNoError;
  } else {
    return ChakraSetProperty(
        std::forward<TObject>(object), std::forward<TPropertyId>(propertyId), std::forward<TValue>(value));
  }
}

template <size_t Index, typename TArray>
JsErrorCode InitArgs(TArray & /*jsArgs*/) noexcept {
  return JsErrorCode::JsNoError;
}

template <size_t Index, typename TArray, typename TArg0, typename... TArgs>
JsErrorCode InitArgs(TArray &jsArgs, TArg0 &&arg0, TArgs &&... args) noexcept {
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TArg0>(arg0), &jsArgs[Index]));
  return InitArgs<Index + 1>(jsArgs, std::forward<TArgs>(args)...);
}

template <typename TFunction, typename... TArgs>
/*static*/ JsErrorCode
Environment::ChakraCallFunction(TFunction &&function, JsValueRef *result, TArgs &&... args) noexcept {
  JsValueRef jsFunction{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TFunction>(function), &jsFunction));
  std::array<JsValueRef, sizeof...(args)> jsArgs;
  CHECK_JSRT_ERROR_CODE(InitArgs<0>(jsArgs, std::forward<TArgs>(args)...));
  return JsCallFunction(jsFunction, jsArgs.data(), static_cast<unsigned short>(jsArgs.size()), result);
}

template <typename TConstructor, typename... TArgs>
static JsErrorCode
Environment::ChakraConstructObject(TConstructor &&constructor, JsValueRef *result, TArgs &&... args) noexcept {
  JsValueRef jsConstructor{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TConstructor>(constructor), &jsConstructor));
  std::array<JsValueRef, sizeof...(args)> jsArgs;
  CHECK_JSRT_ERROR_CODE(InitArgs<0>(jsArgs, std::forward<TArgs>(args)...));
  return JsConstructObject(jsConstructor, jsArgs.data(), static_cast<unsigned short>(jsArgs.size()), result);
}

/// <summary>
///     Creates a new JavaScript Promise object.
/// </summary>
/// <remarks>
///     Requires an active script context.
/// </remarks>
/// <param name="promise">The new Promise object.</param>
/// <param name="resolveFunction">The function called to resolve the created Promise object.</param>
/// <param name="rejectFunction">The function called to reject the created Promise object.</param>
/// <returns>
///     The code <c>JsNoError</c> if the operation succeeded, a failure code otherwise.
/// </returns>
JsErrorCode Environment::ChakraCreatePromise(
    _Out_ JsValueRef *promise,
    _Out_ JsValueRef *resolveFunction,
    _Out_ JsValueRef *rejectFunction) noexcept {
  JsValueRef promiseConstructor{};
  CHECK_JSRT_ERROR_CODE(ChakraGetProperty(m_value.Global, m_propertyId.Promise, &promiseConstructor));

  // The executor function is to be executed by the constructor during the process of constructing
  // the new Promise object. The executor is custom code that ties an outcome to a promise.
  // We return the resolveFunction and rejectFunction given to the executor.
  // Since the execution is synchronous, we allocate executorData on the callstack.
  struct ExecutorData {
    static JsValueRef CALLBACK Callback(
        JsValueRef callee,
        bool isConstructCall,
        JsValueRef *arguments,
        unsigned short argumentCount,
        void *callbackState) {
      return (reinterpret_cast<ExecutorData *>(callbackState))
          ->Callback(callee, isConstructCall, arguments, argumentCount);
    }

    JsValueRef Callback(JsValueRef callee, bool isConstructCall, JsValueRef *arguments, unsigned short argumentCount) {
      *resolve = arguments[1];
      *reject = arguments[2];

      return JS_INVALID_REFERENCE;
    }

    JsValueRef *resolve{};
    JsValueRef *reject{};
  } executorData{resolveFunction, rejectFunction};

  JsValueRef executorFunction{};
  CHECK_JSRT_ERROR_CODE(JsCreateFunction(&ExecutorData::Callback, &executorData, &executorFunction));
  CHECK_JSRT_ERROR_CODE(ChakraConstructObject(promiseConstructor, promise, m_value.Undefined, executorFunction));

  return JsErrorCode::JsNoError;
}

//=============================================================================
// Environment N-API implementaton.
//=============================================================================

void Environment::ClearLastError() noexcept {
  m_lastError.error_code = napi_ok;
  m_lastError.engine_error_code = 0;
  m_lastError.engine_reserved = nullptr;
}

napi_status Environment::SetLastError(napi_status errorCode, uint32_t engineErrorCode, void *engineReserved) noexcept {
  m_lastError.error_code = errorCode;
  m_lastError.engine_error_code = engineErrorCode;
  m_lastError.engine_reserved = engineReserved;

  return errorCode;
}

napi_status Environment::SetLastError(JsErrorCode jsError, void *engineReserved) noexcept {
  napi_status status;
  switch (jsError) {
    case JsNoError:
      status = napi_ok;
      break;
    case JsErrorNullArgument:
    case JsErrorInvalidArgument:
      status = napi_invalid_arg;
      break;
    case JsErrorPropertyNotString:
      status = napi_string_expected;
      break;
    case JsErrorArgumentNotObject:
      status = napi_object_expected;
      break;
    case JsErrorScriptException:
    case JsErrorInExceptionState:
      status = napi_pending_exception;
      break;
    default:
      status = napi_generic_failure;
      break;
  }

  m_lastError.error_code = status;
  m_lastError.engine_error_code = jsError;
  m_lastError.engine_reserved = engineReserved;
  return status;
}

napi_status Environment::GetLastErrorInfo(const napi_extended_error_info **result) noexcept {
  CHECK_ARG(result);

  // Warning: Keep in-sync with napi_status enum
  static constexpr const char *s_errorMmessages[] = {
      nullptr,
      "Invalid argument",
      "An object was expected",
      "A string was expected",
      "A string or symbol was expected",
      "A function was expected",
      "A number was expected",
      "A boolean was expected",
      "An array was expected",
      "Unknown failure",
      "An exception is pending",
      "The async work item was canceled",
      "napi_escape_handle already called on scope",
      "Invalid handle scope usage",
      "Invalid callback scope usage",
      "Thread-safe function queue is full",
      "Thread-safe function handle is closing",
      "A BigInt was expected",
      "A Date was expected",
      "An ArrayBuffer was expected",
      "A detachable ArrayBuffer was expected",
      "Main thread would deadlock",
  };

  // you must update this assert to reference the last message
  // in the napi_status enum each time a new error message is added.
  // We don't have a napi_status_last as this would result in an ABI
  // change each time a message was added.
  static_assert(
      std::size(s_errorMmessages) == napi_would_deadlock + 1,
      "Count of error messages must match count of error values");
  assert(m_lastError.error_code <= napi_callback_scope_mismatch);

  // Wait until someone requests the last error information to fetch the error message string.
  m_lastError.error_message = s_errorMmessages[m_lastError.error_code];

  *result = &m_lastError;
  return napi_ok;
}

napi_status Environment::GetUndefined(napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsGetUndefinedValue(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::GetNull(napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsGetNullValue(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::GetGlobal(napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsGetGlobalObject(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::GetBoolean(bool value, napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsBoolToBoolean(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateObject(napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsCreateObject(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateArray(napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsCreateArray(0, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateArrayWithLength(size_t length, napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsCreateArray(static_cast<unsigned int>(length), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateDouble(double value, napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsDoubleToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateInt32(int32_t value, napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsIntToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateUInt32(uint32_t value, napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateInt64(int64_t value, napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateStringLatin1(const char *str, size_t length, napi_value *result) noexcept {
  CHECK_ARG(str);
  CHECK_ARG(result);
  if (length == NAPI_AUTO_LENGTH) {
    length = strlen(str);
  }

  // The Latin1 encoding is the 256 characters of the extended ASCII set.
  // To convert it to UTF-16 we just expand each character to two bytes.
  auto buffer = SmallBuffer<wchar_t, 256>(length + 1);
  for (size_t i = 0; i < length; ++i) {
    buffer.Data()[i] = static_cast<wchar_t>(static_cast<uint8_t>(str[i]));
  }
  buffer.Data()[length] = 0;
  CHECK_JSRT(JsPointerToString(buffer.Data(), buffer.Size() - 1, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateStringUtf8(const char *str, size_t length, napi_value *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsCreateString(str, length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateStringUtf16(const char16_t *str, size_t length, napi_value *result) noexcept {
  CHECK_ARG(result);
  static_assert(sizeof(char16_t) == sizeof(wchar_t));
  CHECK_JSRT(JsPointerToString(reinterpret_cast<const wchar_t *>(str), length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CreateSymbol(napi_value description, napi_value *result) noexcept {
  CHECK_ARG(result);
  JsValueRef jsDescription = reinterpret_cast<JsValueRef>(description);
  CHECK_JSRT(JsCreateSymbol(jsDescription, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::GetUniqueString(napi_value str, napi_value *result) noexcept {
  // Search using fast path
  const auto it = m_uniqueStrings.find(str);
  if (it != m_uniqueStrings.end()) {
    *result = str;
    return napi_ok;
  }

  // Search using slow path
  JsValueRef jsStr = reinterpret_cast<JsValueRef>(str);
  const wchar_t *strValue{};
  size_t strLength{};
  CHECK_JSRT(JsStringToPointer(jsStr, &strValue, &strLength));
  const auto indexIt = m_uniqueStringIndex.find(std::wstring_view(strValue, strLength));
  if (indexIt != m_uniqueStringIndex.end()) {
    *result = indexIt->second->value;
    return napi_ok;
  }

  // Add new unique string
  auto uniqueStr = std::unique_ptr<UniqueString>(new UniqueString{str, std::wstring_view(strValue, strLength)});
  m_uniqueStrings.try_emplace(*result, uniqueStr.get());
  m_uniqueStringIndex.try_emplace(std::wstring_view(strValue, strLength), uniqueStr.get());

  CHECK_JSRT(JsSetObjectBeforeCollectCallback(jsStr, this, FinalizeUniqueString));
  uniqueStr.release();

  *result = str;
  return napi_ok;
}

napi_status Environment::GetUniqueStringLatin1(const char *str, size_t length, napi_value *result) noexcept {
  CHECK_ARG(str);
  CHECK_ARG(result);
  if (length == NAPI_AUTO_LENGTH) {
    length = strlen(str);
  }
  // The Latin1 encoding is the 256 characters of the extended ASCII set.
  // To convert it to UTF-16 we just expand each character to two bytes.
  auto buffer = SmallBuffer<wchar_t, 256>(length + 1);
  for (size_t i = 0; i < length; ++i) {
    buffer.Data()[i] = static_cast<wchar_t>(str[i]);
  }

  const auto indexIt = m_uniqueStringIndex.find(std::wstring_view(buffer.Data(), buffer.Size()));
  if (indexIt != m_uniqueStringIndex.end()) {
    *result = indexIt->second->value;
    return napi_ok;
  }

  // Add new unique string
  CHECK_JSRT(JsPointerToString(buffer.Data(), buffer.Size(), reinterpret_cast<JsValueRef *>(result)));

  // Index the string buffer associated with the string.
  const wchar_t *strValue{};
  size_t strLength{};
  CHECK_JSRT(JsStringToPointer(reinterpret_cast<JsValueRef>(*result), &strValue, &strLength));
  auto uniqueStr = std::unique_ptr<UniqueString>(new UniqueString{*result, std::wstring_view(strValue, strLength)});
  m_uniqueStrings.try_emplace(*result, uniqueStr.get());
  m_uniqueStringIndex.try_emplace(std::wstring_view(strValue, strLength), uniqueStr.release());
  // TODO: add GC hook to remove items
  return napi_ok;
}

napi_status Environment::GetUniqueStringUtf8(const char *str, size_t length, napi_value *result) noexcept {
  std::wstring wstr = (length == NAPI_AUTO_LENGTH) ? NarrowToWide({str}) : NarrowToWide({str, length});

  const auto indexIt = m_uniqueStringIndex.find(std::wstring_view(wstr));
  if (indexIt != m_uniqueStringIndex.end()) {
    *result = indexIt->second->value;
    return napi_ok;
  }

  // Add new unique string
  CHECK_JSRT(JsPointerToString(wstr.data(), wstr.size(), reinterpret_cast<JsValueRef *>(result)));
  const wchar_t *strValue{};
  size_t strLength{};
  CHECK_JSRT(JsStringToPointer(reinterpret_cast<JsValueRef>(*result), &strValue, &strLength));
  auto uniqueStr = std::unique_ptr<UniqueString>(new UniqueString{*result, std::wstring_view(strValue, strLength)});
  m_uniqueStrings.try_emplace(*result, uniqueStr.get());
  m_uniqueStringIndex.try_emplace(std::wstring_view(strValue, strLength), uniqueStr.release());
  return napi_ok;
}

napi_status Environment::GetUniqueStringUtf16(const char16_t *str, size_t length, napi_value *result) noexcept {
  const auto indexIt = m_uniqueStringIndex.find(std::wstring_view(reinterpret_cast<const wchar_t *>(str), length));
  if (indexIt != m_uniqueStringIndex.end()) {
    *result = indexIt->second->value;
    return napi_ok;
  }

  // Add new unique string
  CHECK_NAPI(CreateStringUtf16(str, length, result));
  const wchar_t *strValue{};
  size_t strLength{};
  CHECK_JSRT(JsStringToPointer(reinterpret_cast<JsValueRef>(*result), &strValue, &strLength));
  auto uniqueStr = std::unique_ptr<UniqueString>(new UniqueString{*result, std::wstring_view(strValue, strLength)});
  m_uniqueStrings.try_emplace(*result, uniqueStr.get());
  m_uniqueStringIndex.try_emplace(std::wstring_view(strValue, strLength), uniqueStr.release());
  return napi_ok;
}

napi_status Environment::CreateFunction(
    const char *utf8Name,
    size_t length,
    napi_callback callback,
    void *data,
    napi_value *result) noexcept {
  CHECK_ARG(result);

  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(this, callback, data)};

  JsValueRef function;
  if (utf8Name != nullptr) {
    JsValueRef name{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsCreateString(utf8Name, length, &name));
    CHECK_JSRT(JsCreateNamedFunction(name, ExternalCallback::Callback, externalCallback.get(), &function));
  } else {
    CHECK_JSRT(JsCreateFunction(ExternalCallback::Callback, externalCallback.get(), &function));
  }

  externalCallback->newTarget = function;

  CHECK_JSRT(JsSetObjectBeforeCollectCallback(function, externalCallback.get(), ExternalCallback::Finalize));
  externalCallback.release();

  *result = reinterpret_cast<napi_value>(function);
  return napi_ok;
}

napi_status Environment::CreateError(napi_value code, napi_value msg, napi_value *result) noexcept {
  CHECK_ARG(msg);
  CHECK_ARG(result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsCreateError(message, &error));
  CHECK_NAPI(SetErrorCode(error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

napi_status Environment::CreateTypeError(napi_value code, napi_value msg, napi_value *result) noexcept {
  CHECK_ARG(msg);
  CHECK_ARG(result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsCreateTypeError(message, &error));
  CHECK_NAPI(SetErrorCode(error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

napi_status Environment::CreateRangeError(napi_value code, napi_value msg, napi_value *result) noexcept {
  CHECK_ARG(msg);
  CHECK_ARG(result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsCreateRangeError(message, &error));
  CHECK_NAPI(SetErrorCode(error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

napi_status Environment::TypeOf(napi_value value, napi_valuetype *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType = JsUndefined;
  CHECK_JSRT(JsGetValueType(jsValue, &valueType));

  switch (valueType) {
    case JsUndefined:
      *result = napi_undefined;
      break;
    case JsNull:
      *result = napi_null;
      break;
    case JsNumber:
      *result = napi_number;
      break;
    case JsString:
      *result = napi_string;
      break;
    case JsBoolean:
      *result = napi_boolean;
      break;
    case JsFunction:
      *result = napi_function;
      break;
    case JsSymbol:
      *result = napi_symbol;
      break;
    case JsError:
      *result = napi_object;
      break;

    default:
      bool hasExternalData;
      if (JsHasExternalData(jsValue, &hasExternalData) != JsNoError) {
        hasExternalData = false;
      }

      *result = hasExternalData ? napi_external : napi_object;
      break;
      // TODO: [vmoroz] add detection of napi_bigint
  }

  return napi_ok;
}

napi_status Environment::GetValueDouble(napi_value value, double *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT_EXPECTED(JsNumberToDouble(jsValue, result), napi_number_expected);
  return napi_ok;
}

napi_status Environment::GetValueInt32(napi_value value, int32_t *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  int valueInt;
  CHECK_JSRT_EXPECTED(JsNumberToInt(jsValue, &valueInt), napi_number_expected);
  *result = static_cast<int32_t>(valueInt);
  return napi_ok;
}

napi_status Environment::GetValueUInt32(napi_value value, uint32_t *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  double valueDouble;
  CHECK_JSRT_EXPECTED(JsNumberToDouble(jsValue, &valueDouble), napi_number_expected);
  if (std::isfinite(valueDouble)) {
    *result = static_cast<int32_t>(valueDouble);
  } else {
    *result = 0;
  }
  return napi_ok;
}

napi_status Environment::GetValueInt64(napi_value value, int64_t *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  double valueDouble;
  CHECK_JSRT_EXPECTED(JsNumberToDouble(jsValue, &valueDouble), napi_number_expected);

  if (std::isfinite(valueDouble)) {
    *result = static_cast<int64_t>(valueDouble);
  } else {
    *result = 0;
  }

  return napi_ok;
}

napi_status Environment::GetValueBool(napi_value value, bool *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT_EXPECTED(JsBooleanToBool(jsValue, result), napi_boolean_expected);
  return napi_ok;
}

// Copies a JavaScript string into a LATIN-1 string buffer. The result is the
// number of bytes (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufSize is insufficient, the string will be truncated and null terminated.
// If buf is nullptr, this method returns the length of the string (in bytes)
// via the result parameter.
// The result argument is optional unless buf is nullptr.
napi_status Environment::GetValueStringLatin1(napi_value value, char *buf, size_t bufSize, size_t *result) noexcept {
  CHECK_ARG(value);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  // The Latin1 encoding is the 256 characters of the extended ASCII set.
  // To convert from UTF-16 we just narrow each character to 8-bits.
  // If the UTF-16 character value was more than 255 we output question mark '?'.

  const char16_t *stringValue{};
  size_t stringLength{};
  CHECK_JSRT(JsStringToPointer(jsValue, reinterpret_cast<const wchar_t **>(&stringValue), &stringLength));
  if (!buf) {
    CHECK_ARG(result);
    *result = stringLength;
  } else {
    RETURN_ENV_STATUS_IF_FALSE(this, bufSize > 0, napi_invalid_arg);
    size_t lengthToCopy = (std::min)(stringLength, bufSize - 1);
    for (size_t i = 0; i < lengthToCopy; ++i) {
      if (char16_t ch16 = stringValue[i]; ch16 < 256) {
        buf[i] = static_cast<char>(ch16);
      } else {
        buf[i] = '?';
      }
    }
    buf[lengthToCopy] = '\0';
    if (result) {
      *result = lengthToCopy;
    }
  }

  return napi_ok;
}

// Copies a JavaScript string into a UTF-8 string buffer. The result is the
// number of bytes (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in bytes)
// via the result parameter.
// The result argument is optional unless buf is NULL.
napi_status Environment::GetValueStringUtf8(napi_value value, char *buf, size_t bufSize, size_t *result) noexcept {
  CHECK_ARG(value);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(result);
    CHECK_JSRT_EXPECTED(JsCopyString(jsValue, nullptr, 0, result), napi_string_expected);
  } else {
    size_t count = 0;
    CHECK_JSRT_EXPECTED(JsCopyString(jsValue, nullptr, 0, &count), napi_string_expected);

    if (bufSize <= count) {
      // if bufsize == count there is no space for null terminator
      // Slow path: must implement truncation here.
      std::unique_ptr<char[]> fullBuffer{new char[count]};
      // CHAKRA_VERIFY(fullBuffer != nullptr);

      CHECK_JSRT_EXPECTED(JsCopyString(jsValue, fullBuffer.get(), count, nullptr), napi_string_expected);
      memmove(buf, fullBuffer.get(), sizeof(char) * bufSize);
      fullBuffer.reset();

      // Truncate string to the start of the last codepoint
      if (bufSize > 0 && (((buf[bufSize - 1] & 0x80) == 0) || UTF8_MULTIBYTE_START(buf[bufSize - 1]))) {
        // Last byte is a single byte codepoint or
        // starts a multibyte codepoint
        bufSize -= 1;
      } else if (bufSize > 1 && UTF8_MULTIBYTE_START(buf[bufSize - 2])) {
        // Second last byte starts a multibyte codepoint,
        bufSize -= 2;
      } else if (bufSize > 2 && UTF8_MULTIBYTE_START(buf[bufSize - 3])) {
        // Third last byte starts a multibyte codepoint
        bufSize -= 3;
      } else if (bufSize > 3 && UTF8_MULTIBYTE_START(buf[bufSize - 4])) {
        // Fourth last byte starts a multibyte codepoint
        bufSize -= 4;
      }

      // TODO: [vmoroz] it seems that we write after the buffer: fix it.
      buf[bufSize] = '\0';

      if (result) {
        *result = bufSize;
      }

      return napi_ok;
    }

    // Fast path, result fits in the buffer
    CHECK_JSRT_EXPECTED(JsCopyString(jsValue, buf, bufSize - 1, &count), napi_string_expected);

    buf[count] = 0;

    if (result != nullptr) {
      *result = count;
    }
  }

  return napi_ok;
}

// Copies a JavaScript string into a UTF-16 string buffer. The result is the
// number of 2-byte code units (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in 2-byte
// code units) via the result parameter.
// The result argument is optional unless buf is NULL.
napi_status Environment::GetValueStringUtf16(napi_value value, char16_t *buf, size_t bufSize, size_t *result) noexcept {
  CHECK_ARG(value);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(result);
    CHECK_JSRT_EXPECTED(JsCopyStringUtf16(jsValue, nullptr, 0, result), napi_string_expected);
  } else {
    size_t copied = 0;
    CHECK_JSRT_EXPECTED(JsCopyStringUtf16(jsValue, buf, bufSize - 1, &copied), napi_string_expected);
    buf[copied] = 0;
    if (result != nullptr) {
      *result = copied;
    }
  }

  return napi_ok;
}

napi_status Environment::CoerceToBool(napi_value value, napi_value *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(JsConvertValueToBoolean(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CoerceToNumber(napi_value value, napi_value *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(JsConvertValueToNumber(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CoerceToObject(napi_value value, napi_value *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(JsConvertValueToObject(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::CoerceToString(napi_value value, napi_value *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(JsConvertValueToString(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::GetPrototype(napi_value object, napi_value *result) noexcept {
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(JsGetPrototype(obj, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::GetPropertyNames(napi_value object, napi_value *result) noexcept {
  return GetAllPropertyNames(
      object,
      napi_key_collection_mode::napi_key_include_prototypes,
      napi_key_filter(napi_key_filter::napi_key_enumerable | napi_key_filter::napi_key_skip_symbols),
      napi_key_conversion::napi_key_numbers_to_strings,
      result);
}

napi_status Environment::SetProperty(napi_value object, napi_value key, napi_value value) noexcept {
  CHECK_ARG(key);
  CHECK_ARG(value);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsPropertyIdFromKey(key, &propertyId));
  JsValueRef js_value = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(JsSetProperty(obj, propertyId, js_value, /*useStrictRules:*/ true));
  return napi_ok;
}

napi_status Environment::HasProperty(napi_value object, napi_value key, bool *result) noexcept {
  CHECK_ARG(key);
  CHECK_ARG(result);
  ClearLastError();

  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsPropertyIdFromKey(key, &propertyId));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(JsHasProperty(obj, propertyId, result));
  return napi_ok;
}

napi_status Environment::GetProperty(napi_value object, napi_value key, napi_value *result) noexcept {
  CHECK_ARG(key);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsPropertyIdFromKey(key, &propertyId));
  CHECK_JSRT(JsGetProperty(obj, propertyId, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::DeleteProperty(napi_value object, napi_value key, bool *result) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(key);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{};
  JsValueRef deletePropertyResult{};
  CHECK_JSRT(JsPropertyIdFromKey(key, &propertyId));
  CHECK_JSRT(JsDeleteProperty(obj, propertyId, false /* isStrictMode */, &deletePropertyResult));
  if (result) {
    CHECK_JSRT(JsBooleanToBool(deletePropertyResult, result));
  }
  return napi_ok;
}

napi_status Environment::HasOwnProperty(napi_value object, napi_value key, bool *result) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(key);
  CHECK_ARG(result);
  ClearLastError();
  JsValueRef jsResult{};

  CHECK_JSRT(ChakraCallFunction(
      m_value.ObjectHasOwnProperty,
      &jsResult,
      reinterpret_cast<JsValueRef>(object),
      reinterpret_cast<JsValueRef>(key)));
  CHECK_JSRT(JsBooleanToBool(jsResult, result));
  return napi_status::napi_ok;
}

napi_status Environment::SetNamedProperty(napi_value object, const char *utf8Name, napi_value value) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(utf8Name);
  CHECK_ARG(value);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsCreatePropertyId(utf8Name, NAPI_AUTO_LENGTH, &propertyId));
  JsValueRef js_value = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(JsSetProperty(obj, propertyId, js_value, true));
  return napi_ok;
}

napi_status Environment::HasNamedProperty(napi_value object, const char *utf8Name, bool *result) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(utf8Name);
  CHECK_ARG(result);
  ClearLastError();

  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsCreatePropertyId(utf8Name, strlen(utf8Name), &propertyId));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(JsHasProperty(obj, propertyId, result));
  return napi_ok;
}

napi_status Environment::GetNamedProperty(napi_value object, const char *utf8Name, napi_value *result) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(utf8Name);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsCreatePropertyId(utf8Name, strlen(utf8Name), &propertyId));
  CHECK_JSRT(JsGetProperty(obj, propertyId, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::SetElement(napi_value object, uint32_t index, napi_value value) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(value);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueRef jsIndex{};
  if (index < static_cast<uint32_t>((std::numeric_limits<int32_t>::max)())) {
    CHECK_JSRT(JsIntToNumber(static_cast<int32_t>(index), &jsIndex));
  } else {
    CHECK_JSRT(JsDoubleToNumber(static_cast<double>(index), &jsIndex));
  }
  CHECK_JSRT(JsSetIndexedProperty(obj, jsIndex, jsValue));
  return napi_ok;
}

napi_status Environment::HasElement(napi_value object, uint32_t index, bool *result) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef jsIndex{};
  if (index < static_cast<uint32_t>((std::numeric_limits<int32_t>::max)())) {
    CHECK_JSRT(JsIntToNumber(static_cast<int32_t>(index), &jsIndex));
  } else {
    CHECK_JSRT(JsDoubleToNumber(static_cast<double>(index), &jsIndex));
  }
  CHECK_JSRT(JsHasIndexedProperty(obj, jsIndex, result));
  return napi_ok;
}

napi_status Environment::GetElement(napi_value object, uint32_t index, napi_value *result) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef jsIndex{};
  if (index < static_cast<uint32_t>((std::numeric_limits<int32_t>::max)())) {
    CHECK_JSRT(JsIntToNumber(static_cast<int32_t>(index), &jsIndex));
  } else {
    CHECK_JSRT(JsDoubleToNumber(static_cast<double>(index), &jsIndex));
  }
  CHECK_JSRT(JsGetIndexedProperty(obj, jsIndex, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::DeleteElement(napi_value object, uint32_t index, bool *result) noexcept {
  CHECK_ARG(object);
  ClearLastError();

  JsValueRef jsIndex{}, element{};
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueType elementType{};
  if (index < static_cast<uint32_t>((std::numeric_limits<int32_t>::max)())) {
    CHECK_JSRT(JsIntToNumber(static_cast<int32_t>(index), &jsIndex));
  } else {
    CHECK_JSRT(JsDoubleToNumber(static_cast<double>(index), &jsIndex));
  }
  CHECK_JSRT(JsDeleteIndexedProperty(obj, jsIndex));
  if (result) {
    CHECK_JSRT(JsGetIndexedProperty(obj, jsIndex, &element));
    CHECK_JSRT(JsGetValueType(element, &elementType));
    *result = elementType == JsValueType::JsUndefined;
  }
  return napi_ok;
}

napi_status Environment::DefineProperties(
    napi_value object,
    size_t propertyCount,
    const napi_property_descriptor *properties) noexcept {
  CHECK_ARG(object);
  if (propertyCount > 0) {
    CHECK_ARG(properties);
  }
  ClearLastError();

  JsPropertyIdRef configurableProperty{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsGetPropertyIdFromName(L"configurable", &configurableProperty));

  // TODO: [vmoroz] Add cached property ID
  JsPropertyIdRef enumerableProperty{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsGetPropertyIdFromName(L"enumerable", &enumerableProperty));

  for (size_t i = 0; i < propertyCount; i++) {
    const napi_property_descriptor *p = properties + i;

    JsValueRef descriptor{JS_INVALID_REFERENCE};
    CHECK_JSRT(JsCreateObject(&descriptor));

    if (p->attributes & napi_configurable) {
      // TODO: [vmoroz] Add cached true/false JsValue
      JsValueRef configurable{JS_INVALID_REFERENCE};
      CHECK_JSRT(JsBoolToBoolean(true, &configurable));
      CHECK_JSRT(JsSetProperty(descriptor, configurableProperty, configurable, true));
    }

    if (p->attributes & napi_enumerable) {
      JsValueRef enumerable{JS_INVALID_REFERENCE};
      CHECK_JSRT(JsBoolToBoolean(true, &enumerable));
      CHECK_JSRT(JsSetProperty(descriptor, enumerableProperty, enumerable, true));
    }

    if (p->getter != nullptr || p->setter != nullptr) {
      napi_value property_name;
      CHECK_JSRT(JsNameValueFromPropertyDescriptor(p, &property_name));

      if (p->getter != nullptr) {
        JsPropertyIdRef getProperty;
        CHECK_JSRT(JsGetPropertyIdFromName(L"get", &getProperty));
        JsValueRef getter;
        CHECK_NAPI(CreatePropertyFunction(property_name, p->getter, p->data, reinterpret_cast<napi_value *>(&getter)));
        CHECK_JSRT(JsSetProperty(descriptor, getProperty, getter, true));
      }

      if (p->setter != nullptr) {
        JsPropertyIdRef setProperty;
        CHECK_JSRT(JsGetPropertyIdFromName(L"set", &setProperty));
        JsValueRef setter;
        CHECK_NAPI(CreatePropertyFunction(property_name, p->setter, p->data, reinterpret_cast<napi_value *>(&setter)));
        CHECK_JSRT(JsSetProperty(descriptor, setProperty, setter, true));
      }
    } else if (p->method != nullptr) {
      napi_value property_name;
      CHECK_JSRT(JsNameValueFromPropertyDescriptor(p, &property_name));

      JsPropertyIdRef valueProperty;
      CHECK_JSRT(JsGetPropertyIdFromName(L"value", &valueProperty));
      JsValueRef method;
      CHECK_NAPI(CreatePropertyFunction(property_name, p->method, p->data, reinterpret_cast<napi_value *>(&method)));
      CHECK_JSRT(JsSetProperty(descriptor, valueProperty, method, true));
    } else {
      RETURN_STATUS_IF_FALSE(p->value != nullptr, napi_invalid_arg);

      if (p->attributes & napi_writable) {
        JsPropertyIdRef writableProperty;
        CHECK_JSRT(JsGetPropertyIdFromName(L"writable", &writableProperty));
        JsValueRef writable;
        CHECK_JSRT(JsBoolToBoolean(true, &writable));
        CHECK_JSRT(JsSetProperty(descriptor, writableProperty, writable, true));
      }

      JsPropertyIdRef valueProperty;
      CHECK_JSRT(JsGetPropertyIdFromName(L"value", &valueProperty));
      CHECK_JSRT(JsSetProperty(descriptor, valueProperty, reinterpret_cast<JsValueRef>(p->value), true));
    }

    JsPropertyIdRef nameProperty;
    CHECK_JSRT(JsPropertyIdFromPropertyDescriptor(p, &nameProperty));
    bool result;
    CHECK_JSRT(JsDefineProperty(
        reinterpret_cast<JsValueRef>(object),
        reinterpret_cast<JsPropertyIdRef>(nameProperty),
        reinterpret_cast<JsValueRef>(descriptor),
        &result));
  }

  return napi_ok;
}

napi_status Environment::IsArray(napi_value value, bool *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType type = JsUndefined;
  CHECK_JSRT(JsGetValueType(jsValue, &type));
  *result = (type == JsArray);
  return napi_ok;
}

napi_status Environment::GetArrayLength(napi_value value, uint32_t *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsPropertyIdRef propertyIdRef;
  CHECK_JSRT(JsGetPropertyIdFromName(L"length", &propertyIdRef));
  JsValueRef lengthRef;
  JsValueRef arrayRef = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(JsGetProperty(arrayRef, propertyIdRef, &lengthRef));
  double sizeInDouble;
  CHECK_JSRT(JsNumberToDouble(lengthRef, &sizeInDouble));
  *result = static_cast<uint32_t>(sizeInDouble);
  return napi_ok;
}

napi_status Environment::StrictEquals(napi_value lhs, napi_value rhs, bool *result) noexcept {
  CHECK_ARG(lhs);
  CHECK_ARG(rhs);
  CHECK_ARG(result);
  JsValueRef object1 = reinterpret_cast<JsValueRef>(lhs);
  JsValueRef object2 = reinterpret_cast<JsValueRef>(rhs);
  CHECK_JSRT(JsStrictEquals(object1, object2, result));
  return napi_ok;
}

napi_status Environment::CallFunction(
    napi_value recv,
    napi_value func,
    size_t argc,
    const napi_value *argv,
    napi_value *result) noexcept {
  CHECK_ARG(recv);
  if (argc > 0) {
    CHECK_ARG(argv);
  }

  JsValueRef function = reinterpret_cast<JsValueRef>(func);
  JsValueArgs args{recv, Span<napi_value>{argv, argc}};
  JsValueRef returnValue;
  CHECK_JSRT(JsCallFunction(function, args.Data(), static_cast<uint16_t>(args.Size()), &returnValue));
  if (result != nullptr) {
    *result = reinterpret_cast<napi_value>(returnValue);
  }
  return napi_ok;
}

napi_status
Environment::NewInstance(napi_value constructor, size_t argc, const napi_value *argv, napi_value *result) noexcept {
  CHECK_ARG(constructor);
  CHECK_ARG(result);
  if (argc > 0) {
    CHECK_ARG(argv);
  }
  JsValueRef function = reinterpret_cast<JsValueRef>(constructor);
  napi_value thisArg;
  CHECK_NAPI(GetUndefined(&thisArg));
  JsValueArgs args{thisArg, Span<napi_value>{argv, argc}};
  CHECK_JSRT(JsConstructObject(
      function, args.Data(), static_cast<uint16_t>(args.Size()), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status Environment::InstanceOf(napi_value object, napi_value constructor, bool *result) noexcept {
  CHECK_ARG(object);
  CHECK_ARG(result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef jsConstructor = reinterpret_cast<JsValueRef>(constructor);

  // FIXME: Remove this type check when we switch to a version of Chakracore
  // where passing an integer into JsInstanceOf as the constructor parameter
  // does not cause a segfault. The need for this if-statement is removed in at
  // least Chakracore 1.4.0, but maybe in an earlier version too.
  napi_valuetype valuetype;
  CHECK_NAPI(TypeOf(constructor, &valuetype));
  if (valuetype != napi_function) {
    ThrowTypeError("ERR_NAPI_CONS_FUNCTION", "constructor must be a function");

    return SetLastError(napi_invalid_arg);
  }

  CHECK_JSRT(JsInstanceOf(obj, jsConstructor, result));
  return napi_ok;
}

// Gets all callback info in a single call. (Ugly, but faster.)
napi_status Environment::GetCallbackInfo(
    napi_callback_info callbackInfo, // [in] Opaque callback-info handle
    size_t *argc, // [in-out] Specifies the size of the provided argv array
                  // and receives the actual count of args.
    napi_value *argv, // [out] Array of values
    napi_value *thisArg, // [out] Receives the JS 'this' arg for the call
    void **data) noexcept // [out] Receives the data pointer for the callback.
{
  CHECK_ARG(callbackInfo);
  const CallbackInfo *info = reinterpret_cast<CallbackInfo *>(callbackInfo);

  if (argv != nullptr) {
    CHECK_ARG(argc);

    size_t i = 0;
    size_t min = (std::min)(*argc, static_cast<size_t>(info->argc));

    for (; i < min; i++) {
      argv[i] = info->argv[i];
    }

    if (i < *argc) {
      napi_value undefined;
      CHECK_JSRT(JsGetUndefinedValue(reinterpret_cast<JsValueRef *>(&undefined)));
      for (; i < *argc; i++) {
        argv[i] = undefined;
      }
    }
  }

  if (argc != nullptr) {
    *argc = info->argc;
  }

  if (thisArg != nullptr) {
    *thisArg = info->thisArg;
  }

  if (data != nullptr) {
    *data = info->data;
  }

  return napi_ok;
}

napi_status Environment::GetNewTarget(napi_callback_info callbackInfo, napi_value *result) noexcept {
  CHECK_ARG(callbackInfo);
  CHECK_ARG(result);

  const CallbackInfo *info = reinterpret_cast<CallbackInfo *>(callbackInfo);
  if (info->isConstructCall) {
    *result = info->newTarget;
  } else {
    *result = nullptr;
  }

  return napi_ok;
}

napi_status Environment::DefineClass(
    const char *utf8Name,
    size_t length,
    napi_callback constructor,
    void *data,
    size_t propertyCount,
    const napi_property_descriptor *properties,
    napi_value *result) noexcept {
  CHECK_ARG(utf8Name);
  CHECK_ARG(constructor);
  CHECK_ARG(result);
  if (propertyCount > 0) {
    CHECK_ARG(properties);
  }

  napi_value nameString;
  CHECK_NAPI(CreateStringUtf8(utf8Name, length, &nameString));

  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(this, constructor, data)};

  JsValueRef jsConstructor;
  CHECK_JSRT(JsCreateNamedFunction(nameString, ExternalCallback::Callback, externalCallback.get(), &jsConstructor));

  externalCallback->newTarget = jsConstructor;

  CHECK_JSRT(JsSetObjectBeforeCollectCallback(jsConstructor, externalCallback.get(), ExternalCallback::Finalize));
  externalCallback.release();

  JsPropertyIdRef pid = nullptr;
  JsValueRef prototype = nullptr;
  CHECK_JSRT(JsGetPropertyIdFromName(L"prototype", &pid));
  CHECK_JSRT(JsGetProperty(jsConstructor, pid, &prototype));

  CHECK_JSRT(JsGetPropertyIdFromName(L"constructor", &pid));
  CHECK_JSRT(JsSetProperty(prototype, pid, jsConstructor, false));

  int instancePropertyCount = 0;
  int staticPropertyCount = 0;
  for (size_t i = 0; i < propertyCount; i++) {
    if ((properties[i].attributes & napi_static) != 0) {
      staticPropertyCount++;
    } else {
      instancePropertyCount++;
    }
  }

  std::vector<napi_property_descriptor> staticDescriptors;
  std::vector<napi_property_descriptor> instanceDescriptors;
  staticDescriptors.reserve(staticPropertyCount);
  instanceDescriptors.reserve(instancePropertyCount);

  for (size_t i = 0; i < propertyCount; i++) {
    if ((properties[i].attributes & napi_static) != 0) {
      staticDescriptors.push_back(properties[i]);
    } else {
      instanceDescriptors.push_back(properties[i]);
    }
  }

  if (staticPropertyCount > 0) {
    CHECK_NAPI(DefineProperties(
        reinterpret_cast<napi_value>(jsConstructor), staticDescriptors.size(), staticDescriptors.data()));
  }

  if (instancePropertyCount > 0) {
    CHECK_NAPI(DefineProperties(
        reinterpret_cast<napi_value>(prototype), instanceDescriptors.size(), instanceDescriptors.data()));
  }

  *result = reinterpret_cast<napi_value>(jsConstructor);
  return napi_ok;
}

napi_status Environment::Wrap(
    napi_value obj,
    void *nativeObj,
    napi_finalize finalizeCallback,
    void *finalizeHint,
    napi_ref *result) noexcept {
  CHECK_ARG(obj);

  JsValueRef jsValue{reinterpret_cast<JsValueRef>(obj)};

  JsValueType jsValueType{JsValueType::JsUndefined};
  CHECK_JSRT(JsGetValueType(jsValue, &jsValueType));
  RETURN_STATUS_IF_FALSE(jsValueType == JsValueType::JsObject, napi_status::napi_object_expected);

  // If we've already wrapped this object, we error out.
  bool hasHostObjectProperty{};
  CHECK_JSRT(ChakraHasPrivateProperty(jsValue, m_propertyId.hostObject, &hasHostObjectProperty));
  RETURN_STATUS_IF_FALSE(!hasHostObjectProperty, napi_invalid_arg);

  napi_ref reference{};
  if (result != nullptr) {
    // The returned reference should be deleted via napi_delete_reference()
    // ONLY in response to the finalize callback invocation. (If it is deleted
    // before then, then the finalize callback will never be invoked.)
    // Therefore a finalize callback is required when returning a reference.
    CHECK_ARG(finalizeCallback);
    CHECK_NAPI(FinalizingReference::New(
        static_cast<napi_env>(this),
        obj,
        /*shouldDeleteSelf:*/ false,
        finalizeCallback,
        nativeObj,
        finalizeHint,
        &reference));
    *result = reference;
  } else {
    // Create a self-deleting reference.
    CHECK_NAPI(FinalizingReference::New(
        static_cast<napi_env>(this),
        obj,
        /*shouldDeleteSelf:*/ true,
        finalizeCallback,
        nativeObj,
        finalizeCallback ? finalizeHint : nullptr,
        &reference));
  }

  JsValueRef external{};
  CHECK_JSRT(JsCreateExternalObject(reference, nullptr, &external));
  CHECK_JSRT(ChakraSetPrivateProperty(jsValue, m_propertyId.hostObject, external));

  return napi_status::napi_ok;
}

napi_status Environment::Unwrap(napi_value js_object, void **result) noexcept {
  CHECK_ARG(js_object);
  CHECK_ARG(result);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(js_object);

  JsValueRef wrapper;
  FinalizingReference *finalizingReferece{};
  CHECK_JSRT(ChakraGetPrivateProperty(jsValue, m_propertyId.hostObject, &wrapper));
  CHECK_JSRT(JsGetExternalData(wrapper, reinterpret_cast<void **>(&finalizingReferece)));

  *result = (finalizingReferece != nullptr ? finalizingReferece->Data() : nullptr);

  return napi_ok;
}

napi_status Environment::RemoveWrap(napi_value js_object, void **result) noexcept {
  // TODO: [vmoroz] Implement
  // CHECK_ARG(js_object);

  // JsValueRef value = reinterpret_cast<JsValueRef>(js_object);

  // ExternalData *externalData = nullptr;
  // JsValueRef parent = JS_INVALID_REFERENCE;
  // JsValueRef wrapper = JS_INVALID_REFERENCE;
  // CHECK_NAPI(Unwrap(env, value, &externalData, &wrapper, &parent));
  // RETURN_STATUS_IF_FALSE(parent != JS_INVALID_REFERENCE, napi_invalid_arg);
  // RETURN_STATUS_IF_FALSE(wrapper != JS_INVALID_REFERENCE, napi_invalid_arg);

  //// Remove the external from the prototype chain
  // JsValueRef wrapperProto = JS_INVALID_REFERENCE;
  // CHECK_JSRT(JsGetPrototype(wrapper, &wrapperProto));
  // CHECK_JSRT(JsSetPrototype(parent, wrapperProto));

  //// Clear the external data from the object
  // CHECK_JSRT(JsSetExternalData(wrapper, nullptr));

  // if (externalData != nullptr) {
  //  *result = externalData->Data();
  //  delete externalData;
  //} else {
  //  *result = nullptr;
  //}

  return napi_ok;
}

napi_status
Environment::CreateExternal(void *data, napi_finalize finalize_cb, void *finalize_hint, napi_value *result) noexcept {
  CHECK_ARG(result);
  std::unique_ptr<ExternalData> externalData{
      new ExternalData(static_cast<napi_env>(this), data, finalize_cb, finalize_hint)};

  CHECK_JSRT(
      JsCreateExternalObject(externalData.get(), ExternalData::Finalize, reinterpret_cast<JsValueRef *>(result)));
  externalData.release();

  return napi_ok;
}

napi_status Environment::GetValueExternal(napi_value value, void **result) noexcept {
  ExternalData *externalData;
  CHECK_JSRT(JsGetExternalData(reinterpret_cast<JsValueRef>(value), reinterpret_cast<void **>(&externalData)));

  *result = (externalData != nullptr ? externalData->Data() : nullptr);

  return napi_ok;
}

napi_status Environment::CreateReference(napi_value value, uint32_t initialRefCount, napi_ref *result) noexcept {
  return Reference::New(static_cast<napi_env>(this), value, initialRefCount, result);
}

napi_status Environment::DeleteReference(napi_ref ref) noexcept {
  return CHECKED_REF(ref)->Delete(static_cast<napi_env>(this));
}

napi_status Environment::ReferenceRef(napi_ref ref, uint32_t *result) noexcept {
  return CHECKED_REF(ref)->Ref(static_cast<napi_env>(this), result);
}

napi_status Environment::ReferenceUnref(napi_ref ref, uint32_t *result) noexcept {
  return CHECKED_REF(ref)->Unref(static_cast<napi_env>(this), result);
}

napi_status Environment::GetReferenceValue(napi_ref ref, napi_value *result) noexcept {
  return CHECKED_REF(ref)->Value(static_cast<napi_env>(this), result);
}

// Stub implementation of handle scope apis for JSRT.
napi_status Environment::OpenHandleScope(napi_handle_scope *result) noexcept {
  CHECK_ARG(result);
  *result = reinterpret_cast<napi_handle_scope>(1);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status Environment::CloseHandleScope(napi_handle_scope scope) noexcept {
  CHECK_ARG(scope);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status Environment::OpenEscapableHandleScope(napi_escapable_handle_scope *result) noexcept {
  CHECK_ARG(result);
  *result = reinterpret_cast<napi_escapable_handle_scope>(1);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status Environment::CloseEscapableHandleScope(napi_escapable_handle_scope scope) noexcept {
  CHECK_ARG(scope);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status
Environment::EscapeHandle(napi_escapable_handle_scope scope, napi_value escapee, napi_value *result) noexcept {
  CHECK_ARG(scope);
  CHECK_ARG(escapee);
  CHECK_ARG(result);
  *result = escapee;
  return napi_ok;
}

napi_status Environment::Throw(napi_value error) noexcept {
  JsValueRef exception = reinterpret_cast<JsValueRef>(error);
  CHECK_JSRT(JsSetException(exception));
  return napi_ok;
}

napi_status Environment::ThrowError(const char *code, const char *msg) noexcept {
  JsValueRef strRef;
  JsValueRef exception;
  size_t length = strlen(msg);
  CHECK_JSRT(JsCreateString(msg, length, &strRef));
  CHECK_JSRT(JsCreateError(strRef, &exception));
  CHECK_NAPI(SetErrorCode(exception, nullptr, code));
  CHECK_JSRT(JsSetException(exception));
  return napi_ok;
}

napi_status Environment::ThrowTypeError(const char *code, const char *msg) noexcept {
  JsValueRef strRef;
  JsValueRef exception;
  size_t length = strlen(msg);
  CHECK_JSRT(JsCreateString(msg, length, &strRef));
  CHECK_JSRT(JsCreateTypeError(strRef, &exception));
  CHECK_NAPI(SetErrorCode(exception, nullptr, code));
  CHECK_JSRT(JsSetException(exception));
  return napi_ok;
}

napi_status Environment::ThrowRangeError(const char *code, const char *msg) noexcept {
  JsValueRef strRef;
  JsValueRef exception;
  size_t length = strlen(msg);
  CHECK_JSRT(JsCreateString(msg, length, &strRef));
  CHECK_JSRT(JsCreateRangeError(strRef, &exception));
  CHECK_NAPI(SetErrorCode(exception, nullptr, code));
  CHECK_JSRT(JsSetException(exception));
  return napi_ok;
}

napi_status Environment::IsError(napi_value value, bool *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);
  JsValueType valueType;
  CHECK_JSRT(JsGetValueType(value, &valueType));
  *result = (valueType == JsError);
  return napi_ok;
}

napi_status Environment::IsExceptionPending(bool *result) noexcept {
  CHECK_ARG(result);
  CHECK_JSRT(JsHasException(result));
  return napi_ok;
}

napi_status Environment::GetAndClearLastException(napi_value *result) noexcept {
  CHECK_ARG(result);

  bool hasException;
  CHECK_JSRT(JsHasException(&hasException));
  if (hasException) {
    CHECK_JSRT(JsGetAndClearException(reinterpret_cast<JsValueRef *>(result)));
  } else {
    CHECK_NAPI(GetUndefined(result));
  }

  return napi_ok;
}

napi_status Environment::IsArrayBuffer(napi_value value, bool *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType;
  CHECK_JSRT(JsGetValueType(jsValue, &valueType));

  *result = (valueType == JsArrayBuffer);
  return napi_ok;
}

napi_status Environment::CreateArrayBuffer(size_t byteLength, void **data, napi_value *result) noexcept {
  CHECK_ARG(result);

  JsValueRef arrayBuffer;
  CHECK_JSRT(JsCreateArrayBuffer(static_cast<unsigned int>(byteLength), &arrayBuffer));

  if (data != nullptr) {
    CHECK_JSRT(JsGetArrayBufferStorage(
        arrayBuffer, reinterpret_cast<BYTE **>(data), reinterpret_cast<unsigned int *>(&byteLength)));
  }

  *result = reinterpret_cast<napi_value>(arrayBuffer);
  return napi_ok;
}

napi_status Environment::CreateExternalArrayBuffer(
    void *externalData,
    size_t byteLength,
    napi_finalize finalizeCallback,
    void *finalizeHint,
    napi_value *result) noexcept {
  CHECK_ARG(result);

  std::unique_ptr<ExternalData> externalDataWrapper{
      new ExternalData(this, externalData, finalizeCallback, finalizeHint)};

  JsValueRef arrayBuffer;
  CHECK_JSRT(JsCreateExternalArrayBuffer(
      externalData,
      static_cast<unsigned int>(byteLength),
      ExternalData::Finalize,
      externalDataWrapper.get(),
      &arrayBuffer));
  externalDataWrapper.release();

  *result = reinterpret_cast<napi_value>(arrayBuffer);
  return napi_ok;
}

napi_status Environment::GetArrayBufferInfo(napi_value arrayBuffer, void **data, size_t *byteLength) noexcept {
  CHECK_ARG(arrayBuffer);

  BYTE *storageData;
  unsigned int storageLength;
  CHECK_JSRT(JsGetArrayBufferStorage(reinterpret_cast<JsValueRef>(arrayBuffer), &storageData, &storageLength));

  if (data != nullptr) {
    *data = reinterpret_cast<void *>(storageData);
  }

  if (byteLength != nullptr) {
    *byteLength = static_cast<size_t>(storageLength);
  }

  return napi_ok;
}

napi_status Environment::IsTypedArray(napi_value value, bool *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType;
  CHECK_JSRT(JsGetValueType(jsValue, &valueType));

  *result = (valueType == JsTypedArray);
  return napi_ok;
}

napi_status Environment::CreateTypedArray(
    napi_typedarray_type type,
    size_t length,
    napi_value arrayBuffer,
    size_t byteOffset,
    napi_value *result) noexcept {
  CHECK_ARG(arrayBuffer);
  CHECK_ARG(result);

  JsTypedArrayType jsType;
  switch (type) {
    case napi_int8_array:
      jsType = JsArrayTypeInt8;
      break;
    case napi_uint8_array:
      jsType = JsArrayTypeUint8;
      break;
    case napi_uint8_clamped_array:
      jsType = JsArrayTypeUint8Clamped;
      break;
    case napi_int16_array:
      jsType = JsArrayTypeInt16;
      break;
    case napi_uint16_array:
      jsType = JsArrayTypeUint16;
      break;
    case napi_int32_array:
      jsType = JsArrayTypeInt32;
      break;
    case napi_uint32_array:
      jsType = JsArrayTypeUint32;
      break;
    case napi_float32_array:
      jsType = JsArrayTypeFloat32;
      break;
    case napi_float64_array:
      jsType = JsArrayTypeFloat64;
      break;
    default:
      return SetLastError(napi_invalid_arg);
  }

  JsValueRef jsArrayBuffer = reinterpret_cast<JsValueRef>(arrayBuffer);

  CHECK_JSRT(JsCreateTypedArray(
      jsType,
      jsArrayBuffer,
      static_cast<unsigned int>(byteOffset),
      static_cast<unsigned int>(length),
      reinterpret_cast<JsValueRef *>(result)));

  return napi_ok;
}

napi_status Environment::GetTypedArrayInfo(
    napi_value typedArray,
    napi_typedarray_type *type,
    size_t *length,
    void **data,
    napi_value *arrayBuffer,
    size_t *byteOffset) noexcept {
  CHECK_ARG(typedArray);

  JsTypedArrayType jsType;
  JsValueRef jsArrayBuffer;
  unsigned int byteOffset2;
  unsigned int byteLength;
  BYTE *bufferData;
  unsigned int bufferLength;
  int elementSize;

  CHECK_JSRT(JsGetTypedArrayInfo(
      reinterpret_cast<JsValueRef>(typedArray), &jsType, &jsArrayBuffer, &byteOffset2, &byteLength));

  CHECK_JSRT(JsGetTypedArrayStorage(
      reinterpret_cast<JsValueRef>(typedArray), &bufferData, &bufferLength, &jsType, &elementSize));

  if (type != nullptr) {
    switch (jsType) {
      case JsArrayTypeInt8:
        *type = napi_int8_array;
        break;
      case JsArrayTypeUint8:
        *type = napi_uint8_array;
        break;
      case JsArrayTypeUint8Clamped:
        *type = napi_uint8_clamped_array;
        break;
      case JsArrayTypeInt16:
        *type = napi_int16_array;
        break;
      case JsArrayTypeUint16:
        *type = napi_uint16_array;
        break;
      case JsArrayTypeInt32:
        *type = napi_int32_array;
        break;
      case JsArrayTypeUint32:
        *type = napi_uint32_array;
        break;
      case JsArrayTypeFloat32:
        *type = napi_float32_array;
        break;
      case JsArrayTypeFloat64:
        *type = napi_float64_array;
        break;
      default:
        return SetLastError(napi_generic_failure);
    }
  }

  if (length != nullptr) {
    *length = static_cast<size_t>(byteLength / elementSize);
  }

  if (data != nullptr) {
    *data = static_cast<uint8_t *>(bufferData);
  }

  if (arrayBuffer != nullptr) {
    *arrayBuffer = reinterpret_cast<napi_value>(jsArrayBuffer);
  }

  if (byteOffset != nullptr) {
    *byteOffset = static_cast<size_t>(byteOffset2);
  }

  return napi_ok;
}

napi_status
Environment::CreateDataView(size_t byteLength, napi_value arrayBuffer, size_t byteOffset, napi_value *result) noexcept {
  CHECK_ARG(arrayBuffer);
  CHECK_ARG(result);

  JsValueRef jsArrayBuffer = reinterpret_cast<JsValueRef>(arrayBuffer);

  BYTE *unused = nullptr;
  unsigned int bufferLength = 0;

  CHECK_JSRT(JsGetArrayBufferStorage(jsArrayBuffer, &unused, &bufferLength));

  if (byteLength + byteOffset > bufferLength) {
    ThrowRangeError(
        "ERR_NAPI_INVALID_DATAVIEW_ARGS",
        "byte_offset + byte_length should be less than or "
        "equal to the size in bytes of the array passed in");
    return SetLastError(napi_pending_exception);
  }

  JsValueRef jsDataView;
  CHECK_JSRT(JsCreateDataView(
      jsArrayBuffer, static_cast<unsigned int>(byteOffset), static_cast<unsigned int>(byteLength), &jsDataView));

  auto dataViewInfo = new DataViewInfo{jsDataView, jsArrayBuffer, byteOffset, byteLength};
  CHECK_JSRT(JsCreateExternalObject(dataViewInfo, DataViewInfo::Finalize, reinterpret_cast<JsValueRef *>(result)));

  return napi_ok;
}

napi_status Environment::IsDataView(napi_value value, bool *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType;
  CHECK_JSRT(JsGetValueType(jsValue, &valueType));

  *result = (valueType == JsDataView);
  return napi_ok;
}

napi_status Environment::GetDataViewInfo(
    napi_value dataview,
    size_t *byteLength,
    void **data,
    napi_value *arrayBuffer,
    size_t *byteOffset) noexcept {
  CHECK_ARG(dataview);

  BYTE *bufferData = nullptr;
  unsigned int bufferLength = 0;

  JsValueRef jsExternalObject = reinterpret_cast<JsValueRef>(dataview);

  DataViewInfo *dataViewInfo;
  CHECK_JSRT(JsGetExternalData(jsExternalObject, reinterpret_cast<void **>(&dataViewInfo)));

  CHECK_JSRT(JsGetDataViewStorage(dataViewInfo->dataView, &bufferData, &bufferLength));

  if (byteLength != nullptr) {
    *byteLength = dataViewInfo->byteLength;
  }

  if (data != nullptr) {
    *data = static_cast<uint8_t *>(bufferData);
  }

  if (arrayBuffer != nullptr) {
    *arrayBuffer = reinterpret_cast<napi_value>(dataViewInfo->arrayBuffer);
  }

  if (byteOffset != nullptr) {
    *byteOffset = dataViewInfo->byteOffset;
  }

  return napi_ok;
}

napi_status Environment::GetVersion(uint32_t *result) noexcept {
  CHECK_ARG(result);
  *result = NAPI_VERSION;
  return napi_ok;
}

napi_status Environment::CreatePromise(napi_deferred *deferred, napi_value *promise) noexcept {
  CHECK_ARG(deferred);
  CHECK_ARG(promise);

  JsValueRef jsPromise, jsResolve, jsReject, jsDeferred;
  napi_ref deferredRef;

  CHECK_JSRT(ChakraCreatePromise(&jsPromise, &jsResolve, &jsReject));
  CHECK_JSRT(JsCreateObject(&jsDeferred));
  CHECK_JSRT(ChakraSetProperty(jsDeferred, m_propertyId.resolve, jsResolve));
  CHECK_JSRT(ChakraSetProperty(jsDeferred, m_propertyId.reject, jsReject));

  CHECK_NAPI(Reference::New(static_cast<napi_env>(this), reinterpret_cast<napi_value>(jsDeferred), 1, &deferredRef));

  *deferred = reinterpret_cast<napi_deferred>(deferredRef);
  *promise = reinterpret_cast<napi_value>(jsPromise);

  return napi_ok;
}

napi_status Environment::ResolveDeferred(napi_deferred deferred, napi_value resolution) noexcept {
  return ConcludeDeferred(deferred, m_propertyId.resolve, resolution);
}

napi_status Environment::RejectDeferred(napi_deferred deferred, napi_value rejection) noexcept {
  return ConcludeDeferred(deferred, m_propertyId.reject, rejection);
}

napi_status
Environment::ConcludeDeferred(napi_deferred deferred, CachedPropertyId propertyId, napi_value result) noexcept {
  CHECK_ARG(deferred);
  CHECK_ARG(result);

  JsValueRef resolver;
  JsValueRef jsDeferred;
  napi_ref ref = reinterpret_cast<napi_ref>(deferred);

  CHECK_NAPI(GetReferenceValue(ref, reinterpret_cast<napi_value *>(&jsDeferred)));
  CHECK_JSRT(ChakraGetProperty(jsDeferred, propertyId, &resolver));
  CHECK_JSRT(ChakraCallFunction(resolver, nullptr, m_value.Null, result));
  return DeleteReference(ref);
}

napi_status Environment::IsPromise(napi_value value, bool *is_promise) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(is_promise);

  JsValueRef promiseConstructor{};
  CHECK_JSRT(ChakraGetProperty(m_value.Global, m_propertyId.Promise, &promiseConstructor));
  CHECK_JSRT(JsInstanceOf(reinterpret_cast<JsValueRef>(value), promiseConstructor, is_promise));

  return napi_ok;
}

napi_status Environment::RunScript(napi_value script, napi_value *result) noexcept {
  CHECK_ARG(script);
  CHECK_ARG(result);

  JsValueRef scriptVar = reinterpret_cast<JsValueRef>(script);

  const wchar_t *scriptStr;
  size_t scriptStrLen;
  CHECK_JSRT(JsStringToPointer(scriptVar, &scriptStr, &scriptStrLen));
  CHECK_JSRT_EXPECTED(
      JsRunScript(scriptStr, ++m_sourceContext, L"Unknown", reinterpret_cast<JsValueRef *>(result)),
      napi_string_expected);

  return napi_ok;
}

napi_status Environment::AdjustExternalMemory(int64_t changeInBytes, int64_t *adjustedValue) noexcept {
  CHECK_ARG(adjustedValue);

  // TODO(jackhorton): Determine if Chakra needs or is able to do anything here
  // For now, we can lie and say that we always adjusted more memory
  *adjustedValue = changeInBytes;

  return napi_ok;
}

#if NAPI_VERSION >= 5

napi_status Environment::CreateDate(double time, napi_value *result) noexcept {
  CHECK_ARG(result);

  JsValueRef dateConstructor{};
  CHECK_JSRT(ChakraGetProperty(m_value.Global, m_propertyId.Date, &dateConstructor));

  JsValueRef args[2] = {};
  CHECK_JSRT(JsGetUndefinedValue(&args[0]));
  CHECK_JSRT(JsDoubleToNumber(time, &args[1]));
  CHECK_JSRT(JsConstructObject(dateConstructor, args, 2, reinterpret_cast<JsValueRef *>(result)));

  return napi_status::napi_ok;
}

napi_status Environment::IsDate(napi_value value, bool *isDate) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(isDate);

  JsValueRef dateConstructor{};
  CHECK_JSRT(ChakraGetProperty(m_value.Global, m_propertyId.Date, &dateConstructor));

  JsValueRef obj{reinterpret_cast<JsValueRef>(value)};
  CHECK_JSRT(JsInstanceOf(obj, dateConstructor, isDate));

  return napi_status::napi_ok;
}

napi_status Environment::GetDateValue(napi_value value, double *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(result);

  bool isDate{false};
  CHECK_NAPI(IsDate(value, &isDate));
  RETURN_STATUS_IF_FALSE(isDate, napi_status::napi_date_expected);

  JsPropertyIdRef valueOfId{JS_INVALID_REFERENCE};
  CHECK_JSRT(JsGetPropertyIdFromName(L"valueOf", &valueOfId));

  JsValueRef jsValue{reinterpret_cast<JsValueRef>(value)};
  JsValueRef valueOf{};
  CHECK_JSRT(ChakraGetProperty(jsValue, m_propertyId.valueOf, &valueOf));

  JsValueRef dateValue{};
  CHECK_JSRT(JsCallFunction(valueOf, &jsValue, 1, &dateValue));
  CHECK_JSRT(JsNumberToDouble(dateValue, result));

  return napi_status::napi_ok;
}

napi_status Environment::AddFinalizer(
    napi_value jsObject,
    void *nativeObject,
    napi_finalize finalizeCallback,
    void *finalizeHint,
    napi_ref *result) noexcept {
  return chakra::FinalizingReference::New(
      this, jsObject, /*shouldDeleteSelf:*/ result == nullptr, finalizeCallback, nativeObject, finalizeHint, result);
}

#endif // NAPI_VERSION >= 5

#if NAPI_VERSION >= 6

napi_status Environment::CreateBigIntInt64(int64_t value, napi_value *result) noexcept {
  // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::CreateBigIntUInt64(uint64_t value, napi_value *result) noexcept {
  // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::CreateBigIntWords(
    int sign_bit,
    size_t word_count,
    const uint64_t *words,
    napi_value *result) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::GetValueBigIntInt64(
    napi_value value,
    int64_t *result,
    bool *isLossless) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::GetValueBigIntUInt64(
    napi_value value,
    uint64_t *result,
    bool *isLossless) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::GetValueBigIntWords(
    napi_value value,
    int *signBit,
    size_t *wordCount,
    uint64_t *words) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::GetAllPropertyNames(
    napi_value object,
    napi_key_collection_mode keyMode,
    napi_key_filter keyFilter,
    napi_key_conversion /*keyConversion*/,
    napi_value *result) noexcept {
  // We currently do not handle the keyConversion
  // Chakra API seems not be able to provide numeric property names.
  CHECK_ARG(object);
  CHECK_ARG(result);
  ClearLastError();

  JsValueRef jsObj = reinterpret_cast<JsValueRef>(object);
  std::vector<JsValueRef> allPropertyNames;

  auto checkDescriptorFilter = [&](JsValueRef descriptor,
                                   napi_key_filter checkFilter,
                                   CachedPropertyId propertyId,
                                   bool *result) -> napi_status {
    if (*result && (keyFilter & checkFilter)) {
      bool isTrue{};
      CHECK_JSRT(ChakraGetBoolProperty(descriptor, propertyId, &isTrue));
      if (!isTrue) {
        *result = false;
      }
    }

    return napi_ok;
  };

  auto isPropertyDescriptorAccepted = [&](JsPropertyIdRef propId, bool *result) -> napi_status {
    JsValueRef descriptor{};
    CHECK_JSRT(JsGetOwnPropertyDescriptor(jsObj, propId, &descriptor));
    *result = true;
    CHECK_NAPI(checkDescriptorFilter(descriptor, napi_key_writable, m_propertyId.writable, result));
    CHECK_NAPI(checkDescriptorFilter(descriptor, napi_key_enumerable, m_propertyId.enumerable, result));
    CHECK_NAPI(checkDescriptorFilter(descriptor, napi_key_configurable, m_propertyId.configurable, result));
    return napi_ok;
  };

  bool useDescriptorFilter = (keyFilter & (napi_key_writable | napi_key_enumerable | napi_key_configurable)) != 0;

  for (;;) {
    if ((keyFilter & napi_key_skip_strings) == 0) {
      JsValueRef propertyNames{};
      uint32_t propertyNamesSize{};
      CHECK_JSRT(JsGetOwnPropertyNames(jsObj, &propertyNames));
      CHECK_NAPI(GetArrayLength(reinterpret_cast<napi_value>(propertyNames), &propertyNamesSize));
      size_t requiredCapacity = allPropertyNames.size() + propertyNamesSize;
      if (requiredCapacity > allPropertyNames.capacity() + allPropertyNames.capacity() / 2) {
        // vector is usually grown by 50%. We reserve here only if we need to grow more.
        allPropertyNames.reserve(requiredCapacity);
      }
      for (uint32_t i = 0; i < propertyNamesSize; ++i) {
        JsValueRef propName{}, index{};
        CHECK_JSRT(JsIntToNumber(i, &index));
        CHECK_JSRT(JsGetIndexedProperty(propertyNames, index, &propName));
        if (useDescriptorFilter) {
          JsPropertyIdRef propId{};
          const wchar_t *strValue{};
          size_t strLength{};
          bool isAccepted{};
          CHECK_JSRT(JsStringToPointer(propName, &strValue, &strLength));
          CHECK_JSRT(JsGetPropertyIdFromName(strValue, &propId));
          CHECK_NAPI(isPropertyDescriptorAccepted(propId, &isAccepted));
          if (!isAccepted) {
            continue;
          }
        }
        allPropertyNames.push_back(propName);
      }
    }

    if ((keyFilter & napi_key_skip_symbols) == 0) {
      JsValueRef propertySymbols{};
      uint32_t propertySymbolsSize{};
      CHECK_JSRT(JsGetOwnPropertySymbols(jsObj, &propertySymbols));
      CHECK_NAPI(GetArrayLength(reinterpret_cast<napi_value>(propertySymbols), &propertySymbolsSize));
      if (propertySymbolsSize > allPropertyNames.size() / 2) {
        allPropertyNames.reserve(allPropertyNames.size() + propertySymbolsSize);
      }
      for (uint32_t i = 0; i < propertySymbolsSize; ++i) {
        JsValueRef propSymbol{}, index{};
        CHECK_JSRT(JsIntToNumber(i, &index));
        CHECK_JSRT(JsGetIndexedProperty(propertySymbols, index, &propSymbol));
        if (useDescriptorFilter) {
          JsPropertyIdRef propId{};
          bool isAccepted{};
          CHECK_JSRT(JsGetPropertyIdFromSymbol(propSymbol, &propId));
          CHECK_NAPI(isPropertyDescriptorAccepted(propId, &isAccepted));
          if (!isAccepted) {
            continue;
          }
        }
        allPropertyNames.push_back(propSymbol);
      }
    }

    JsValueRef jsPrototype{};
    CHECK_JSRT(JsGetPrototype(jsObj, &jsPrototype));

    jsObj = jsPrototype;
    JsValueType objType{};
    CHECK_JSRT(JsGetValueType(jsObj, &objType));

    if (keyMode == napi_key_collection_mode::napi_key_own_only || objType < JsValueType::JsObject) {
      break;
    }
  }

  JsValueRef resultArray{};
  uint32_t resultSize = static_cast<unsigned int>(allPropertyNames.size());
  CHECK_JSRT(JsCreateArray(resultSize, &resultArray));
  for (uint32_t i = 0; i < resultSize; ++i) {
    JsValueRef index{};
    CHECK_JSRT(JsIntToNumber(i, &index));
    CHECK_JSRT(JsSetIndexedProperty(resultArray, index, allPropertyNames[i]));
  }

  *result = reinterpret_cast<napi_value>(resultArray);
  return napi_status::napi_ok;
}

napi_status Environment::SetInstanceData(
    void *data,
    napi_finalize finalizeCallback,
    void *finalizeHint) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::GetInstanceData(void **data) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

#endif // NAPI_VERSION >= 6

#if NAPI_VERSION >= 7

napi_status Environment::DetachArrayBuffer(napi_value arrayBuffer) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

napi_status Environment::IsDetachedArrayBuffer(napi_value value, bool *result) noexcept { // TODO: [vmoroz] Implement
  CRASH_IF_FALSE(false);
}

#endif // NAPI_VERSION >= 7

#ifdef NAPI_EXPERIMENTAL

napi_status Environment::TypeTagObject(napi_value value, const napi_type_tag *typeTag) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(typeTag);
  JsValueRef external{};
  CHECK_JSRT(JsCreateExternalObject((void *)(typeTag), nullptr, &external));
  CHECK_JSRT(ChakraSetPrivateProperty(value, m_propertyId.tag, external));
  return napi_status::napi_ok;
}

napi_status Environment::CheckObjectTypeTag(napi_value value, const napi_type_tag *typeTag, bool *result) noexcept {
  CHECK_ARG(value);
  CHECK_ARG(typeTag);
  CHECK_ARG(result);
  JsValueRef external{};
  JsValueType externalType{};
  napi_type_tag *objectTypeTag;
  CHECK_JSRT(ChakraGetPrivateProperty(value, m_propertyId.tag, &external));
  CHECK_JSRT(JsGetValueType(external, &externalType));
  if (externalType == JsValueType::JsUndefined) {
    *result = false;
  } else {
    CHECK_JSRT(JsGetExternalData(external, reinterpret_cast<void **>(&objectTypeTag)));
    *result =
        objectTypeTag != nullptr && typeTag->lower == objectTypeTag->lower && typeTag->upper == objectTypeTag->upper;
  }
  return napi_status::napi_ok;
}

napi_status Environment::ObjectFreeze(napi_value object) noexcept {
  CHECK_JSRT(
      ChakraCallFunction(m_value.ObjectFreeze, nullptr, m_value.Undefined, reinterpret_cast<JsValueRef>(object)));
  return napi_status::napi_ok;
}

napi_status Environment::ObjectSeal(napi_value object) noexcept {
  CHECK_JSRT(ChakraCallFunction(m_value.ObjectSeal, nullptr, m_value.Undefined, reinterpret_cast<JsValueRef>(object)));
  return napi_status::napi_ok;
}

#endif // NAPI_EXPERIMENTAL

napi_status Environment::SerializeScript(const char *script, uint8_t *buffer, size_t *bufferSize) noexcept {
  const std::wstring utf16Script = NarrowToWide({script});

  unsigned long bytecodeSize{};
  CHECK_JSRT(JsSerializeScript(utf16Script.c_str(), nullptr, &bytecodeSize));
  if (buffer) {
    RETURN_STATUS_IF_FALSE(*bufferSize >= bytecodeSize, napi_status::napi_invalid_arg);
    CHECK_JSRT(JsSerializeScript(utf16Script.c_str(), buffer, &bytecodeSize));
  }

  *bufferSize = static_cast<size_t>(bytecodeSize);
  return napi_status::napi_ok;
}

napi_status Environment::RunSerializedScript(
    const char *script,
    uint8_t *buffer,
    const char *sourceUrl,
    napi_value *result) noexcept {
  const std::wstring utf16Script = NarrowToWide({script});
  const std::wstring utf16SourceUrl = NarrowToWide({sourceUrl});

  CHECK_JSRT(JsRunSerializedScript(
      utf16Script.c_str(), buffer, ++m_sourceContext, utf16SourceUrl.c_str(), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

} // namespace chakra

//=============================================================================
// N-API implementation of C-based functions.
//=============================================================================

napi_status napi_get_last_error_info(napi_env env, const napi_extended_error_info **result) {
  return CHECKED_ENV(env)->GetLastErrorInfo(result);
}

napi_status napi_get_undefined(napi_env env, napi_value *result) {
  return CHECKED_ENV(env)->GetUndefined(result);
}

napi_status napi_get_null(napi_env env, napi_value *result) {
  return CHECKED_ENV(env)->GetNull(result);
}

napi_status napi_get_global(napi_env env, napi_value *result) {
  return CHECKED_ENV(env)->GetGlobal(result);
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value *result) {
  return CHECKED_ENV(env)->GetBoolean(value, result);
}

napi_status napi_create_object(napi_env env, napi_value *result) {
  return CHECKED_ENV(env)->CreateObject(result);
}

napi_status napi_create_array(napi_env env, napi_value *result) {
  return CHECKED_ENV(env)->CreateArray(result);
}

napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value *result) {
  return CHECKED_ENV(env)->CreateArrayWithLength(length, result);
}

napi_status napi_create_double(napi_env env, double value, napi_value *result) {
  return CHECKED_ENV(env)->CreateDouble(value, result);
}

napi_status napi_create_int32(napi_env env, int32_t value, napi_value *result) {
  return CHECKED_ENV(env)->CreateInt32(value, result);
}

napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value *result) {
  return CHECKED_ENV(env)->CreateUInt32(value, result);
}

napi_status napi_create_int64(napi_env env, int64_t value, napi_value *result) {
  return CHECKED_ENV(env)->CreateInt64(value, result);
}

napi_status napi_create_string_latin1(napi_env env, const char *str, size_t length, napi_value *result) {
  return CHECKED_ENV(env)->CreateStringLatin1(str, length, result);
}

napi_status napi_create_string_utf8(napi_env env, const char *str, size_t length, napi_value *result) {
  return CHECKED_ENV(env)->CreateStringUtf8(str, length, result);
}

napi_status napi_create_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result) {
  return CHECKED_ENV(env)->CreateStringUtf16(str, length, result);
}

napi_status napi_create_symbol(napi_env env, napi_value description, napi_value *result) {
  return CHECKED_ENV(env)->CreateSymbol(description, result);
}

napi_status napi_create_function(
    napi_env env,
    const char *utf8name,
    size_t length,
    napi_callback cb,
    void *data,
    napi_value *result) {
  return CHECKED_ENV(env)->CreateFunction(utf8name, length, cb, data, result);
}

napi_status napi_create_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  return CHECKED_ENV(env)->CreateError(code, msg, result);
}

napi_status napi_create_type_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  return CHECKED_ENV(env)->CreateTypeError(code, msg, result);
}

napi_status napi_create_range_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  return CHECKED_ENV(env)->CreateRangeError(code, msg, result);
}

napi_status napi_typeof(napi_env env, napi_value value, napi_valuetype *result) {
  return CHECKED_ENV(env)->TypeOf(value, result);
}

napi_status napi_get_value_double(napi_env env, napi_value value, double *result) {
  return CHECKED_ENV(env)->GetValueDouble(value, result);
}

napi_status napi_get_value_int32(napi_env env, napi_value value, int32_t *result) {
  return CHECKED_ENV(env)->GetValueInt32(value, result);
}

napi_status napi_get_value_uint32(napi_env env, napi_value value, uint32_t *result) {
  return CHECKED_ENV(env)->GetValueUInt32(value, result);
}

napi_status napi_get_value_int64(napi_env env, napi_value value, int64_t *result) {
  return CHECKED_ENV(env)->GetValueInt64(value, result);
}

napi_status napi_get_value_bool(napi_env env, napi_value value, bool *result) {
  return CHECKED_ENV(env)->GetValueBool(value, result);
}

napi_status napi_get_value_string_latin1(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
  return CHECKED_ENV(env)->GetValueStringLatin1(value, buf, bufsize, result);
}

napi_status napi_get_value_string_utf8(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
  return CHECKED_ENV(env)->GetValueStringUtf8(value, buf, bufsize, result);
}

napi_status napi_get_value_string_utf16(napi_env env, napi_value value, char16_t *buf, size_t bufsize, size_t *result) {
  return CHECKED_ENV(env)->GetValueStringUtf16(value, buf, bufsize, result);
}

napi_status napi_coerce_to_bool(napi_env env, napi_value value, napi_value *result) {
  return CHECKED_ENV(env)->CoerceToBool(value, result);
}

napi_status napi_coerce_to_number(napi_env env, napi_value value, napi_value *result) {
  return CHECKED_ENV(env)->CoerceToNumber(value, result);
}

napi_status napi_coerce_to_object(napi_env env, napi_value value, napi_value *result) {
  return CHECKED_ENV(env)->CoerceToObject(value, result);
}

napi_status napi_coerce_to_string(napi_env env, napi_value value, napi_value *result) {
  return CHECKED_ENV(env)->CoerceToString(value, result);
}

napi_status napi_get_prototype(napi_env env, napi_value object, napi_value *result) {
  return CHECKED_ENV(env)->GetPrototype(object, result);
}

napi_status napi_get_property_names(napi_env env, napi_value object, napi_value *result) {
  return CHECKED_ENV(env)->GetPropertyNames(object, result);
}

napi_status napi_set_property(napi_env env, napi_value object, napi_value key, napi_value value) {
  return CHECKED_ENV(env)->SetProperty(object, key, value);
}

napi_status napi_has_property(napi_env env, napi_value object, napi_value key, bool *result) {
  return CHECKED_ENV(env)->HasProperty(object, key, result);
}

napi_status napi_get_property(napi_env env, napi_value object, napi_value key, napi_value *result) {
  return CHECKED_ENV(env)->GetProperty(object, key, result);
}

napi_status napi_delete_property(napi_env env, napi_value object, napi_value key, bool *result) {
  return CHECKED_ENV(env)->DeleteProperty(object, key, result);
}

napi_status napi_has_own_property(napi_env env, napi_value object, napi_value key, bool *result) {
  return CHECKED_ENV(env)->HasOwnProperty(object, key, result);
}

napi_status napi_set_named_property(napi_env env, napi_value object, const char *utf8name, napi_value value) {
  return CHECKED_ENV(env)->SetNamedProperty(object, utf8name, value);
}

napi_status napi_has_named_property(napi_env env, napi_value object, const char *utf8name, bool *result) {
  return CHECKED_ENV(env)->HasNamedProperty(object, utf8name, result);
}

napi_status napi_get_named_property(napi_env env, napi_value object, const char *utf8name, napi_value *result) {
  return CHECKED_ENV(env)->GetNamedProperty(object, utf8name, result);
}

napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value) {
  return CHECKED_ENV(env)->SetElement(object, index, value);
}

napi_status napi_has_element(napi_env env, napi_value object, uint32_t index, bool *result) {
  return CHECKED_ENV(env)->HasElement(object, index, result);
}

napi_status napi_get_element(napi_env env, napi_value object, uint32_t index, napi_value *result) {
  return CHECKED_ENV(env)->GetElement(object, index, result);
}

napi_status napi_delete_element(napi_env env, napi_value object, uint32_t index, bool *result) {
  return CHECKED_ENV(env)->DeleteElement(object, index, result);
}

napi_status napi_define_properties(
    napi_env env,
    napi_value object,
    size_t property_count,
    const napi_property_descriptor *properties) {
  return CHECKED_ENV(env)->DefineProperties(object, property_count, properties);
}

napi_status napi_is_array(napi_env env, napi_value value, bool *result) {
  return CHECKED_ENV(env)->IsArray(value, result);
}

napi_status napi_get_array_length(napi_env env, napi_value value, uint32_t *result) {
  return CHECKED_ENV(env)->GetArrayLength(value, result);
}

napi_status napi_strict_equals(napi_env env, napi_value lhs, napi_value rhs, bool *result) {
  return CHECKED_ENV(env)->StrictEquals(lhs, rhs, result);
}

napi_status napi_call_function(
    napi_env env,
    napi_value recv,
    napi_value func,
    size_t argc,
    const napi_value *argv,
    napi_value *result) {
  return CHECKED_ENV(env)->CallFunction(recv, func, argc, argv, result);
}

napi_status
napi_new_instance(napi_env env, napi_value constructor, size_t argc, const napi_value *argv, napi_value *result) {
  return CHECKED_ENV(env)->NewInstance(constructor, argc, argv, result);
}

napi_status napi_instanceof(napi_env env, napi_value object, napi_value constructor, bool *result) {
  return CHECKED_ENV(env)->InstanceOf(object, constructor, result);
}

napi_status napi_get_cb_info(
    napi_env env, // [in] NAPI environment handle
    napi_callback_info cbinfo, // [in] Opaque callback-info handle
    size_t *argc, // [in-out] Specifies the size of the provided argv array
                  // and receives the actual count of args.
    napi_value *argv, // [out] Array of values
    napi_value *this_arg, // [out] Receives the JS 'this' arg for the call
    void **data) // [out] Receives the data pointer for the callback.
{
  return CHECKED_ENV(env)->GetCallbackInfo(cbinfo, argc, argv, this_arg, data);
}

napi_status napi_get_new_target(napi_env env, napi_callback_info cbinfo, napi_value *result) {
  return CHECKED_ENV(env)->GetNewTarget(cbinfo, result);
}

napi_status napi_define_class(
    napi_env env,
    const char *utf8name,
    size_t length,
    napi_callback constructor,
    void *data,
    size_t property_count,
    const napi_property_descriptor *properties,
    napi_value *result) {
  return CHECKED_ENV(env)->DefineClass(utf8name, length, constructor, data, property_count, properties, result);
}

napi_status napi_wrap(
    napi_env env,
    napi_value js_object,
    void *native_object,
    napi_finalize finalize_cb,
    void *finalize_hint,
    napi_ref *result) {
  return CHECKED_ENV(env)->Wrap(js_object, native_object, finalize_cb, finalize_hint, result);
}

napi_status napi_unwrap(napi_env env, napi_value js_object, void **result) {
  return CHECKED_ENV(env)->Unwrap(js_object, result);
}

napi_status napi_remove_wrap(napi_env env, napi_value js_object, void **result) {
  return CHECKED_ENV(env)->RemoveWrap(js_object, result);
}

napi_status
napi_create_external(napi_env env, void *data, napi_finalize finalize_cb, void *finalize_hint, napi_value *result) {
  return CHECKED_ENV(env)->CreateExternal(data, finalize_cb, finalize_hint, result);
}

napi_status napi_get_value_external(napi_env env, napi_value value, void **result) {
  return CHECKED_ENV(env)->GetValueExternal(value, result);
}

napi_status napi_create_reference(napi_env env, napi_value value, uint32_t initial_refcount, napi_ref *result) {
  return CHECKED_ENV(env)->CreateReference(value, initial_refcount, result);
}

napi_status napi_delete_reference(napi_env env, napi_ref ref) {
  return CHECKED_ENV(env)->DeleteReference(ref);
}

napi_status napi_reference_ref(napi_env env, napi_ref ref, uint32_t *result) {
  return CHECKED_ENV(env)->ReferenceRef(ref, result);
}

napi_status napi_reference_unref(napi_env env, napi_ref ref, uint32_t *result) {
  return CHECKED_ENV(env)->ReferenceUnref(ref, result);
}

napi_status napi_get_reference_value(napi_env env, napi_ref ref, napi_value *result) {
  return CHECKED_ENV(env)->GetReferenceValue(ref, result);
}

napi_status napi_open_handle_scope(napi_env env, napi_handle_scope *result) {
  return CHECKED_ENV(env)->OpenHandleScope(result);
}

napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope) {
  return CHECKED_ENV(env)->CloseHandleScope(scope);
}

napi_status napi_open_escapable_handle_scope(napi_env env, napi_escapable_handle_scope *result) {
  return CHECKED_ENV(env)->OpenEscapableHandleScope(result);
}

// Stub implementation of handle scope apis for JSRT.
napi_status napi_close_escapable_handle_scope(napi_env env, napi_escapable_handle_scope scope) {
  return CHECKED_ENV(env)->CloseEscapableHandleScope(scope);
}

// Stub implementation of handle scope apis for JSRT.
napi_status
napi_escape_handle(napi_env env, napi_escapable_handle_scope scope, napi_value escapee, napi_value *result) {
  return CHECKED_ENV(env)->EscapeHandle(scope, escapee, result);
}

napi_status napi_throw(napi_env env, napi_value error) {
  return CHECKED_ENV(env)->Throw(error);
}

napi_status napi_throw_error(napi_env env, const char *code, const char *msg) {
  return CHECKED_ENV(env)->ThrowError(code, msg);
}

napi_status napi_throw_type_error(napi_env env, const char *code, const char *msg) {
  return CHECKED_ENV(env)->ThrowTypeError(code, msg);
}

napi_status napi_throw_range_error(napi_env env, const char *code, const char *msg) {
  return CHECKED_ENV(env)->ThrowRangeError(code, msg);
}

napi_status napi_is_error(napi_env env, napi_value value, bool *result) {
  return CHECKED_ENV(env)->IsError(value, result);
}

napi_status napi_is_exception_pending(napi_env env, bool *result) {
  return CHECKED_ENV(env)->IsExceptionPending(result);
}

napi_status napi_get_and_clear_last_exception(napi_env env, napi_value *result) {
  return CHECKED_ENV(env)->GetAndClearLastException(result);
}

napi_status napi_is_arraybuffer(napi_env env, napi_value value, bool *result) {
  return CHECKED_ENV(env)->IsArrayBuffer(value, result);
}

napi_status napi_create_arraybuffer(napi_env env, size_t byte_length, void **data, napi_value *result) {
  return CHECKED_ENV(env)->CreateArrayBuffer(byte_length, data, result);
}

napi_status napi_create_external_arraybuffer(
    napi_env env,
    void *external_data,
    size_t byte_length,
    napi_finalize finalize_cb,
    void *finalize_hint,
    napi_value *result) {
  return CHECKED_ENV(env)->CreateExternalArrayBuffer(external_data, byte_length, finalize_cb, finalize_hint, result);
}

napi_status napi_get_arraybuffer_info(napi_env env, napi_value arraybuffer, void **data, size_t *byte_length) {
  return CHECKED_ENV(env)->GetArrayBufferInfo(arraybuffer, data, byte_length);
}

napi_status napi_is_typedarray(napi_env env, napi_value value, bool *result) {
  return CHECKED_ENV(env)->IsTypedArray(value, result);
}

napi_status napi_create_typedarray(
    napi_env env,
    napi_typedarray_type type,
    size_t length,
    napi_value arraybuffer,
    size_t byte_offset,
    napi_value *result) {
  return CHECKED_ENV(env)->CreateTypedArray(type, length, arraybuffer, byte_offset, result);
}

napi_status napi_get_typedarray_info(
    napi_env env,
    napi_value typedarray,
    napi_typedarray_type *type,
    size_t *length,
    void **data,
    napi_value *arraybuffer,
    size_t *byte_offset) {
  return CHECKED_ENV(env)->GetTypedArrayInfo(typedarray, type, length, data, arraybuffer, byte_offset);
}

napi_status
napi_create_dataview(napi_env env, size_t byte_length, napi_value arraybuffer, size_t byte_offset, napi_value *result) {
  return CHECKED_ENV(env)->CreateDataView(byte_length, arraybuffer, byte_offset, result);
}

napi_status napi_is_dataview(napi_env env, napi_value value, bool *result) {
  return CHECKED_ENV(env)->IsDataView(value, result);
}

napi_status napi_get_dataview_info(
    napi_env env,
    napi_value dataview,
    size_t *byte_length,
    void **data,
    napi_value *arraybuffer,
    size_t *byte_offset) {
  return CHECKED_ENV(env)->GetDataViewInfo(dataview, byte_length, data, arraybuffer, byte_offset);
}

napi_status napi_get_version(napi_env env, uint32_t *result) {
  return CHECKED_ENV(env)->GetVersion(result);
}

napi_status napi_create_promise(napi_env env, napi_deferred *deferred, napi_value *promise) {
  return CHECKED_ENV(env)->CreatePromise(deferred, promise);
}

napi_status napi_resolve_deferred(napi_env env, napi_deferred deferred, napi_value resolution) {
  return CHECKED_ENV(env)->ResolveDeferred(deferred, resolution);
}

napi_status napi_reject_deferred(napi_env env, napi_deferred deferred, napi_value rejection) {
  return CHECKED_ENV(env)->RejectDeferred(deferred, rejection);
}

napi_status napi_is_promise(napi_env env, napi_value value, bool *is_promise) {
  return CHECKED_ENV(env)->IsPromise(value, is_promise);
}

napi_status napi_run_script(napi_env env, napi_value script, napi_value *result) {
  return CHECKED_ENV(env)->RunScript(script, result);
}

napi_status napi_adjust_external_memory(napi_env env, int64_t change_in_bytes, int64_t *adjusted_value) {
  return CHECKED_ENV(env)->AdjustExternalMemory(change_in_bytes, adjusted_value);
}

#if NAPI_VERSION >= 5

napi_status napi_create_date(napi_env env, double time, napi_value *result) {
  return CHECKED_ENV(env)->CreateDate(time, result);
}

napi_status napi_is_date(napi_env env, napi_value value, bool *is_date) {
  return CHECKED_ENV(env)->IsDate(value, is_date);
}

napi_status napi_get_date_value(napi_env env, napi_value value, double *result) {
  return CHECKED_ENV(env)->GetDateValue(value, result);
}

napi_status napi_add_finalizer(
    napi_env env,
    napi_value js_object,
    void *native_object,
    napi_finalize finalize_cb,
    void *finalize_hint,
    napi_ref *result) {
  return CHECKED_ENV(env)->AddFinalizer(js_object, native_object, finalize_cb, finalize_hint, result);
}

#endif // NAPI_VERSION >= 5

#if NAPI_VERSION >= 6

napi_status napi_create_bigint_int64(napi_env env, int64_t value, napi_value *result) {
  return CHECKED_ENV(env)->CreateBigIntInt64(value, result);
}

napi_status napi_create_bigint_uint64(napi_env env, uint64_t value, napi_value *result) {
  return CHECKED_ENV(env)->CreateBigIntUInt64(value, result);
}

napi_status
napi_create_bigint_words(napi_env env, int sign_bit, size_t word_count, const uint64_t *words, napi_value *result) {
  return CHECKED_ENV(env)->CreateBigIntWords(sign_bit, word_count, words, result);
}

napi_status napi_get_value_bigint_int64(napi_env env, napi_value value, int64_t *result, bool *lossless) {
  return CHECKED_ENV(env)->GetValueBigIntInt64(value, result, lossless);
}

napi_status napi_get_value_bigint_uint64(napi_env env, napi_value value, uint64_t *result, bool *lossless) {
  return CHECKED_ENV(env)->GetValueBigIntUInt64(value, result, lossless);
}

napi_status
napi_get_value_bigint_words(napi_env env, napi_value value, int *sign_bit, size_t *word_count, uint64_t *words) {
  return CHECKED_ENV(env)->GetValueBigIntWords(value, sign_bit, word_count, words);
}

napi_status napi_get_all_property_names(
    napi_env env,
    napi_value object,
    napi_key_collection_mode key_mode,
    napi_key_filter key_filter,
    napi_key_conversion key_conversion,
    napi_value *result) {
  return CHECKED_ENV(env)->GetAllPropertyNames(object, key_mode, key_filter, key_conversion, result);
}

napi_status napi_set_instance_data(napi_env env, void *data, napi_finalize finalize_cb, void *finalize_hint) {
  return CHECKED_ENV(env)->SetInstanceData(data, finalize_cb, finalize_hint);
}

napi_status napi_get_instance_data(napi_env env, void **data) {
  return CHECKED_ENV(env)->GetInstanceData(data);
}

#endif // NAPI_VERSION >= 6

#if NAPI_VERSION >= 7

napi_status napi_detach_arraybuffer(napi_env env, napi_value arraybuffer) {
  return CHECKED_ENV(env)->DetachArrayBuffer(arraybuffer);
}

napi_status napi_is_detached_arraybuffer(napi_env env, napi_value value, bool *result) {
  return CHECKED_ENV(env)->IsDetachedArrayBuffer(value, result);
}

#endif // NAPI_VERSION >= 7

#ifdef NAPI_EXPERIMENTAL

napi_status napi_type_tag_object(napi_env env, napi_value value, const napi_type_tag *type_tag) {
  return CHECKED_ENV(env)->TypeTagObject(value, type_tag);
}

napi_status napi_check_object_type_tag(napi_env env, napi_value value, const napi_type_tag *type_tag, bool *result) {
  return CHECKED_ENV(env)->CheckObjectTypeTag(value, type_tag, result);
}

napi_status napi_object_freeze(napi_env env, napi_value object) {
  return CHECKED_ENV(env)->ObjectFreeze(object);
}

napi_status napi_object_seal(napi_env env, napi_value object) {
  return CHECKED_ENV(env)->ObjectSeal(object);
}

#endif // NAPI_EXPERIMENTAL

#pragma region NAPI extensions

NAPI_EXTERN napi_status napiext_get_unique_string(napi_env env, napi_value str, napi_value *result) {
  return CHECKED_ENV(env)->GetUniqueString(str, result);
}

NAPI_EXTERN napi_status
napiext_get_unique_string_latin1(napi_env env, const char *str, size_t length, napi_value *result) {
  return CHECKED_ENV(env)->GetUniqueStringLatin1(str, length, result);
}

NAPI_EXTERN napi_status
napiext_get_unique_string_utf8(napi_env env, const char *str, size_t length, napi_value *result) {
  return CHECKED_ENV(env)->GetUniqueStringUtf8(str, length, result);
}

NAPI_EXTERN napi_status
napiext_get_unique_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result) {
  return CHECKED_ENV(env)->GetUniqueStringUtf16(str, length, result);
}

NAPI_EXTERN napi_status
napiext_serialize_script(napi_env env, const char *script, uint8_t *buffer, size_t *buffer_size) {
  return CHECKED_ENV(env)->SerializeScript(script, buffer, buffer_size);
}

NAPI_EXTERN napi_status napiext_run_serialized_script(
    napi_env env,
    const char *script,
    uint8_t *buffer,
    const char *source_url,
    napi_value *result) {
  return CHECKED_ENV(env)->RunSerializedScript(script, buffer, source_url, result);
}

#pragma endregion

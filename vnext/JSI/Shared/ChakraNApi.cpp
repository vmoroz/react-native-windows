// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define NAPI_EXPERIMENTAL
#include "js_native_api.h"
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

// Check condition and crash process if it fails.
#define CHASH_IF_FALSE(condition)             \
  do {                                        \
    if (!(condition)) {                       \
      assert(false && "Failed: " #condition); \
      *((int *)0) = 1;                        \
    }                                         \
  } while (false)

#define RETURN_STATUS_IF_FALSE2(condition, status) \
  do {                                             \
    if (!(condition)) {                            \
      return SetLastError(status);                 \
    }                                              \
  } while (0)

#define CHECK_ARG2(arg) RETURN_STATUS_IF_FALSE2(((arg) != nullptr), napi_invalid_arg)

#define CHECK_ARGS_2(arg1, arg2) \
  CHECK_ARG2(arg1);              \
  CHECK_ARG2(arg2)

#define CHECK_JSRT2(expr)       \
  do {                          \
    JsErrorCode err = (expr);   \
    if (err != JsNoError) {     \
      return SetLastError(err); \
    }                           \
  } while (0)

#define CHECK_JSRT_EXPECTED2(expr, expected) \
  do {                                       \
    JsErrorCode err = (expr);                \
    if (err == JsErrorInvalidArgument)       \
      return SetLastError(expected);         \
    if (err != JsNoError)                    \
      return SetLastError(err);              \
  } while (0)

#define RETURN_STATUS_IF_FALSE(env, condition, status) \
  do {                                                 \
    if (!(condition)) {                                \
      return (env)->SetLastError(status);              \
    }                                                  \
  } while (0)

#define CHECK_ENV(env)                      \
  do {                                      \
    if ((env) == nullptr) {                 \
      return napi_status::napi_invalid_arg; \
    }                                       \
  } while (0)

#define CHECKED_ENV(env) ((env) == nullptr) ? napi_invalid_arg : (env)

#define CHECKED_REF(ref) ((ref) == nullptr) ? napi_invalid_arg : reinterpret_cast<chakra::Reference *>(ref)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE((env), ((arg) != nullptr), napi_invalid_arg)

#define CHECK_ENV_AND_ARG(env, arg) \
  CHECK_ENV((env));                 \
  CHECK_ARG((env), (arg))

#define CHECK_ENV_AND_ARG2(env, arg1, arg2) \
  CHECK_ENV_AND_ARG((env), (arg1));         \
  CHECK_ARG((env), (arg2))

#define CHECK_ENV_AND_ARG3(env, arg1, arg2, arg3) \
  CHECK_ENV_AND_ARG2((env), (arg1), (arg2));      \
  CHECK_ARG((env), (arg3))

#define CHECK_JSRT(env, expr)          \
  do {                                 \
    JsErrorCode err = (expr);          \
    if (err != JsNoError)              \
      return (env)->SetLastError(err); \
  } while (0)

#define CHECK_JSRT_EXPECTED(env, expr, expected) \
  do {                                           \
    JsErrorCode err = (expr);                    \
    if (err == JsErrorInvalidArgument)           \
      return (env)->SetLastError(expected);      \
    if (err != JsNoError)                        \
      return (env)->SetLastError(err);           \
  } while (0)

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
        CHASH_IF_FALSE(m_propertyIdType == JsPropertyIdTypeSymbol);
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

struct CachedValue {
  using GetSimpleValue = decltype(JsGetTrueValue);
  CachedValue(GetSimpleValue *init) noexcept : m_init{init} {}

  JsErrorCode Get(JsValueRef *result) noexcept {
    if (m_value == JS_INVALID_REFERENCE) {
      JsValueRef propertyStr{JS_INVALID_REFERENCE};
      JsValueRef propertySymbol{JS_INVALID_REFERENCE};
      CHECK_JSRT_ERROR_CODE(m_init(&m_value));
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
  JsValueRef m_value{JS_INVALID_REFERENCE};
  GetSimpleValue *m_init;
};

struct Environment {
  explicit Environment(JsContextRef context) noexcept;
  ~Environment() noexcept;

  JsContextRef Context() const noexcept;

  void Ref() noexcept;
  void Unref() noexcept;

  void LinkReference(RefTracker::RefList *reference) noexcept;
  void LinkFinalizingReference(RefTracker::RefList *reference) noexcept;

  void ClearLastError() noexcept;
  napi_status
  SetLastError(napi_status errorCode, uint32_t engineErrorCode = 0, void *engineReserved = nullptr) noexcept;
  napi_status SetLastError(JsErrorCode jsError, void *engineReserved = nullptr) noexcept;
  napi_status GetLastErrorInfo(const napi_extended_error_info **result) noexcept;

  napi_status HasOwnProperty(napi_value object, napi_value key, bool *result) noexcept;

  napi_status CreateReference(napi_value value, uint32_t initialRefCount, napi_ref *result) noexcept;
  napi_status DeleteReference(napi_ref ref) noexcept;
  napi_status ReferenceRef(napi_ref ref, uint32_t *result) noexcept;
  napi_status ReferenceUnref(napi_ref ref, uint32_t *result) noexcept;
  napi_status GetReferenceValue(napi_ref ref, napi_value *result) noexcept;

  napi_status
  Wrap(napi_value obj, void *nativeObj, napi_finalize finalizeCallback, void *finalizeHint, napi_ref *result) noexcept;

  napi_status CreatePromise(napi_deferred *deferred, napi_value *promise) noexcept;
  napi_status ResolveDeferred(napi_deferred deferred, napi_value resolution) noexcept;
  napi_status RejectDeferred(napi_deferred deferred, napi_value rejection) noexcept;
  napi_status ConcludeDeferred(napi_deferred deferred, CachedPropertyId propertyId, napi_value result) noexcept;
  napi_status IsPromise(napi_value value, bool *is_promise) noexcept;

  napi_status RunScript(napi_value script, napi_value *result) noexcept;

  napi_status CreateDate(double time, napi_value *result) noexcept;
  napi_status IsDate(napi_value value, bool *is_date) noexcept;
  napi_status GetDateValue(napi_value value, double *result) noexcept;

 private:
  static JsErrorCode PointerToString(std::wstring_view value, JsValueRef *result) noexcept;

  template <class TObject, class TPropertyId>
  static JsErrorCode GetProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept;

  template <class TObject, class TPropertyId, class TValue>
  static JsErrorCode SetProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept;

  template <typename TValue>
  JsErrorCode CreatePropertyDescriptor(TValue &&value, PropertyAttibutes attrs, JsValueRef *result) noexcept;

  template <typename TObject, typename TPropertyId>
  static JsErrorCode
  DefineProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef propertyDescriptor, bool *isSucceeded) noexcept;

  template <typename TObject, typename TPropertyId, typename TValue>
  JsErrorCode DefineProperty(
      TObject &&object,
      TPropertyId &&propertyId,
      TValue &&value,
      PropertyAttibutes attrs,
      bool *isSucceeded) noexcept;

  template <class TObject, class TPropertyId>
  JsErrorCode HasPrivateProperty(TObject &&object, TPropertyId &&propertyId, bool *result) noexcept;

  template <class TObject, class TPropertyId>
  JsErrorCode GetPrivateProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept;

  template <class TObject, class TPropertyId, class TValue>
  JsErrorCode SetPrivateProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept;

  template <typename TFunction, typename... TArgs>
  static JsErrorCode CallFunction(TFunction &&function, JsValueRef *result, TArgs &&... args) noexcept;

  template <typename TConstructor, typename... TArgs>
  static JsErrorCode ChakraConstructObject(TConstructor &&constructor, JsValueRef *result, TArgs &&... args) noexcept;

  static JsErrorCode GetHasOwnPropertyFunction(JsValueRef *result) noexcept;

  JsErrorCode ChakraCreatePromise(
      _Out_ JsValueRef *promise,
      _Out_ JsValueRef *resolveFunction,
      _Out_ JsValueRef *rejectFunction) noexcept;

 private:
  JsRefHolder m_context;
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
    CachedPropertyId hasOwnProperty{L"hasOwnProperty"};
    CachedPropertyId hostObject{L"hostObject", JsPropertyIdTypeSymbol};
    CachedPropertyId reject{L"reject"};
    CachedPropertyId resolve{L"resolve"};
    CachedPropertyId value{L"value"};
    CachedPropertyId valueOf{L"valueOf"};
    CachedPropertyId writable{L"writable"};
  } m_propertyId;

  struct Value final {
    CachedValue False{JsGetTrueValue};
    CachedValue Global{JsGetGlobalObject};
    CachedValue Null{JsGetNullValue};
    CachedValue Undefined{JsGetUndefinedValue};
    CachedValue True{JsGetTrueValue};
    CachedValue HasOwnProperty{GetHasOwnPropertyFunction};
  } m_value;
};

} // namespace chakra

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

// TODO: [vmoroz] Separate implementation form the declaration.
struct Reference : RefTracker {
  // We use std::unique_ptr to avoid memory leaks after allocation.
  friend std::default_delete<Reference>;

  static napi_status New(napi_env env, napi_value value, uint32_t initialRefCount, napi_ref *result) noexcept {
    CHECK_ENV_AND_ARG2(env, value, result);

    JsValueRef jsValue{reinterpret_cast<JsValueRef>(value)};

    JsValueType jsValueType{JsValueType::JsUndefined};
    CHECK_JSRT(env, JsGetValueType(jsValue, &jsValueType));
    RETURN_STATUS_IF_FALSE(env, jsValueType >= JsValueType::JsObject, napi_status::napi_object_expected);

    // Allocate new Reference and make sure that it is not null.
    auto ref = std::unique_ptr<Reference>{new (std::nothrow) Reference{
        jsValue, initialRefCount, /*hasBeforeCollectCallback:*/ initialRefCount == 0, /*shouldDeleteSelf:*/ false}};
    RETURN_STATUS_IF_FALSE(env, ref, napi_status::napi_generic_failure);

    if (initialRefCount == 0) {
      CHECK_JSRT(env, JsSetObjectBeforeCollectCallback(jsValue, ref.get(), BeforeCollectCallback));
    } else {
      CHECK_JSRT(env, JsAddRef(jsValue, nullptr));
    }

    env->LinkReference(ref.get());
    *result = reinterpret_cast<napi_ref>(ref.release());
    return napi_status::napi_ok;
  }

  napi_status Delete(napi_env env) noexcept {
    CHECK_ENV(env);
    // Delete must not be called if we expect it to be deleted by Finalizer
    RETURN_STATUS_IF_FALSE(env, !m_shouldDeleteSelf, napi_status::napi_generic_failure);

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
    CHECK_ENV(env);

    if (m_value) {
      if (m_refCount == 0) {
        CHECK_JSRT(env, JsAddRef(m_value, nullptr));
      }
      ++m_refCount;
    }

    if (result) {
      *result = m_refCount;
    }

    return napi_status::napi_ok;
  }

  napi_status Unref(napi_env env, uint32_t *result) noexcept {
    CHECK_ENV(env);
    RETURN_STATUS_IF_FALSE(env, m_refCount > 0, napi_status::napi_generic_failure);

    --m_refCount;
    if (m_value) {
      if (m_refCount == 0) {
        if (!m_hasBeforeCollectCallback) {
          CHECK_JSRT(env, JsSetObjectBeforeCollectCallback(m_value, this, BeforeCollectCallback));
          m_hasBeforeCollectCallback = true;
        }

        CHECK_JSRT(env, JsRelease(m_value, nullptr));
      }
    }

    if (result) {
      *result = m_refCount;
    }

    return napi_status::napi_ok;
  }

  napi_status Value(napi_env env, napi_value *result) noexcept {
    CHECK_ENV_AND_ARG(env, result);

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
      napi_env env,
      napi_value value,
      bool shouldDeleteSelf,
      napi_finalize finalizeCallback,
      void *finalizeData,
      void *finalizeHint,
      napi_ref *result) noexcept {
    CHECK_ENV_AND_ARG2(env, value, finalizeCallback);

    JsValueRef jsValue{reinterpret_cast<JsValueRef>(value)};

    JsValueType jsValueType{JsValueType::JsUndefined};
    CHECK_JSRT(env, JsGetValueType(jsValue, &jsValueType));
    if (jsValueType < JsValueType::JsObject) {
      return env->SetLastError(napi_status::napi_object_expected);
    }

    // Allocate new Reference and make sure that it is not null.
    auto ref = std::unique_ptr<FinalizingReference>{new (std::nothrow) FinalizingReference{
        env, jsValue, shouldDeleteSelf, finalizeCallback, finalizeData, finalizeHint}};
    RETURN_STATUS_IF_FALSE(env, ref, napi_status::napi_generic_failure);

    CHECK_JSRT(env, JsSetObjectBeforeCollectCallback(jsValue, ref.get(), BeforeCollectCallback));

    env->LinkFinalizingReference(ref.get());
    if (result) {
      *result = reinterpret_cast<napi_ref>(ref.get());
    }

    ref.release();
    return napi_status::napi_ok;
  }

 protected:
  using Super = Reference;

  FinalizingReference(
      napi_env env,
      JsValueRef value,
      bool shouldDeleteSelf,
      napi_finalize finalizeCallback,
      void *finalizeData,
      void *finalizeHint) noexcept
      : Super{value, /*refCount:*/ 0, /*hasBeforeCollectCallback:*/ true, shouldDeleteSelf},
        m_env(env),
        m_finalizeCallback{finalizeCallback},
        m_finalizeData{finalizeData},
        m_finalizeHint{finalizeHint} {}

  void Finalize(bool isEnvTeardown) noexcept override {
    m_finalizeCallback(m_env, m_finalizeData, m_finalizeHint);
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

//=============================================================================
// Environment implementation
//=============================================================================

Environment::Environment(JsContextRef context) noexcept : m_context{context} {}

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
  CHECK_ARG2(result);

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

JsErrorCode Environment::PointerToString(std::wstring_view value, JsValueRef *result) noexcept {
  return JsPointerToString(value.data(), value.size(), result);
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

napi_status Environment::Wrap(
    napi_value obj,
    void *nativeObj,
    napi_finalize finalizeCallback,
    void *finalizeHint,
    napi_ref *result) noexcept {
  CHECK_ARG(this, obj);

  JsValueRef jsValue{reinterpret_cast<JsValueRef>(obj)};

  JsValueType jsValueType{JsValueType::JsUndefined};
  CHECK_JSRT(this, JsGetValueType(jsValue, &jsValueType));
  RETURN_STATUS_IF_FALSE(this, jsValueType == JsValueType::JsObject, napi_status::napi_object_expected);

  // If we've already wrapped this object, we error out.
  bool hasHostObjectProperty{};
  CHECK_JSRT(this, HasPrivateProperty(jsValue, m_propertyId.hostObject, &hasHostObjectProperty));
  RETURN_STATUS_IF_FALSE(this, !hasHostObjectProperty, napi_invalid_arg);

  napi_ref reference{};
  if (result != nullptr) {
    // The returned reference should be deleted via napi_delete_reference()
    // ONLY in response to the finalize callback invocation. (If it is deleted
    // before then, then the finalize callback will never be invoked.)
    // Therefore a finalize callback is required when returning a reference.
    CHECK_ARG(this, finalizeCallback);
    CHECK_NAPI(FinalizingReference::New(
        static_cast<napi_env>(this),
        obj,
        /*shouldDeleteSelf:*/ false,
        finalizeCallback,
        nativeObj,
        finalizeHint,
        &reference));
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
  CHECK_JSRT(this, JsCreateExternalObject(reference, nullptr, &external));
  CHECK_JSRT(this, SetPrivateProperty(jsValue, m_propertyId.hostObject, external));

  return napi_status::napi_ok;
}

napi_status Environment::HasOwnProperty(napi_value object, napi_value key, bool *result) noexcept {
  CHECK_ARG(this, object);
  CHECK_ARG(this, key);
  CHECK_ARG(this, result);
  JsValueRef jsResult{};

  CHECK_JSRT(
      this,
      CallFunction(
          m_value.HasOwnProperty, &jsResult, reinterpret_cast<JsValueRef>(object), reinterpret_cast<JsValueRef>(key)));
  CHECK_JSRT(this, JsBooleanToBool(jsResult, result));
  return napi_status::napi_ok;
}

napi_status Environment::CreatePromise(napi_deferred *deferred, napi_value *promise) noexcept {
  CHECK_ARG(this, deferred);
  CHECK_ARG(this, promise);

  JsValueRef jsPromise, jsResolve, jsReject, jsDeferred;
  napi_ref deferredRef;

  CHECK_JSRT(this, ChakraCreatePromise(&jsPromise, &jsResolve, &jsReject));
  CHECK_JSRT(this, JsCreateObject(&jsDeferred));
  CHECK_JSRT(this, SetProperty(jsDeferred, m_propertyId.resolve, jsResolve));
  CHECK_JSRT(this, SetProperty(jsDeferred, m_propertyId.reject, jsReject));

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
  CHECK_ARG(this, deferred);
  CHECK_ARG(this, result);

  JsValueRef resolver;
  napi_value container, js_null;
  napi_ref ref = reinterpret_cast<napi_ref>(deferred);

  CHECK_NAPI(GetReferenceValue(ref, &container));
  CHECK_JSRT(this, GetProperty(container, propertyId, &resolver));
  CHECK_JSRT(this, CallFunction(resolver, nullptr, m_value.Null, result));
  return DeleteReference(ref);
}

napi_status Environment::IsPromise(napi_value value, bool *is_promise) noexcept {
  CHECK_ARG(this, value);
  CHECK_ARG(this, is_promise);

  JsValueRef promiseConstructor{};
  CHECK_JSRT(this, GetProperty(m_value.Global, m_propertyId.Promise, &promiseConstructor));
  CHECK_JSRT(this, JsInstanceOf(reinterpret_cast<JsValueRef>(value), promiseConstructor, is_promise));

  return napi_ok;
}

napi_status Environment::RunScript(napi_value script, napi_value *result) noexcept {
  CHECK_ARG2(script);
  CHECK_ARG2(result);

  JsValueRef scriptVar = reinterpret_cast<JsValueRef>(script);

  const wchar_t *scriptStr;
  size_t scriptStrLen;
  CHECK_JSRT2(JsStringToPointer(scriptVar, &scriptStr, &scriptStrLen));
  CHECK_JSRT_EXPECTED2(
      JsRunScript(scriptStr, ++m_sourceContext, L"Unknown", reinterpret_cast<JsValueRef *>(result)),
      napi_string_expected);

  return napi_ok;
}

napi_status Environment::CreateDate(double time, napi_value *result) noexcept {
  CHECK_ARG(this, result);

  JsValueRef dateConstructor{};
  CHECK_JSRT(this, GetProperty(m_value.Global, m_propertyId.Date, &dateConstructor));

  JsValueRef args[2] = {};
  CHECK_JSRT(this, JsGetUndefinedValue(&args[0]));
  CHECK_JSRT(this, JsDoubleToNumber(time, &args[1]));
  CHECK_JSRT(this, JsConstructObject(dateConstructor, args, 2, reinterpret_cast<JsValueRef *>(result)));

  return napi_status::napi_ok;
}

napi_status Environment::IsDate(napi_value value, bool *is_date) noexcept {
  CHECK_ARG(this, value);
  CHECK_ARG(this, is_date);

  JsValueRef dateConstructor{};
  CHECK_JSRT(this, GetProperty(m_value.Global, m_propertyId.Date, &dateConstructor));

  JsValueRef obj{reinterpret_cast<JsValueRef>(value)};
  CHECK_JSRT(this, JsInstanceOf(obj, dateConstructor, is_date));

  return napi_status::napi_ok;
}

napi_status Environment::GetDateValue(napi_value value, double *result) noexcept {
  CHECK_ARG(this, value);
  CHECK_ARG(this, result);

  bool isDate{false};
  CHECK_NAPI(IsDate(value, &isDate));
  RETURN_STATUS_IF_FALSE(this, isDate, napi_status::napi_date_expected);

  JsPropertyIdRef valueOfId{JS_INVALID_REFERENCE};
  CHECK_JSRT(this, JsGetPropertyIdFromName(L"valueOf", &valueOfId));

  JsValueRef jsValue{reinterpret_cast<JsValueRef>(value)};
  JsValueRef valueOf{};
  CHECK_JSRT(this, GetProperty(jsValue, m_propertyId.valueOf, &valueOf));

  JsValueRef dateValue{};
  CHECK_JSRT(this, JsCallFunction(valueOf, &jsValue, 1, &dateValue));
  CHECK_JSRT(this, JsNumberToDouble(dateValue, result));

  return napi_status::napi_ok;
}

template <class TObject, class TPropertyId>
JsErrorCode Environment::GetProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept {
  JsValueRef jsObject{};
  JsPropertyIdRef jsPropertyId{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  return JsGetProperty(jsObject, jsPropertyId, result);
}

template <class TObject, class TPropertyId, class TValue>
JsErrorCode Environment::SetProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept {
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
Environment::CreatePropertyDescriptor(TValue &&value, PropertyAttibutes attrs, JsValueRef *result) noexcept {
  JsValueRef descriptor{};
  CHECK_JSRT_ERROR_CODE(JsCreateObject(&descriptor));
  CHECK_JSRT_ERROR_CODE(SetProperty(descriptor, m_propertyId.value, std::forward<TValue>(value)));
  if (!(attrs & PropertyAttibutes::ReadOnly)) {
    CHECK_JSRT_ERROR_CODE(SetProperty(descriptor, m_propertyId.writable, m_value.True));
  }
  if (!(attrs & PropertyAttibutes::DontEnum)) {
    CHECK_JSRT_ERROR_CODE(SetProperty(descriptor, m_propertyId.enumerable, m_value.True));
  }
  if (!(attrs & PropertyAttibutes::DontDelete)) {
    CHECK_JSRT_ERROR_CODE(SetProperty(descriptor, m_propertyId.configurable, m_value.True));
  }
  return JsErrorCode::JsNoError;
}

template <typename TObject, typename TPropertyId>
JsErrorCode Environment::DefineProperty(
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
JsErrorCode Environment::DefineProperty(
    TObject &&object,
    TPropertyId &&propertyId,
    TValue &&value,
    PropertyAttibutes attrs,
    bool *isSucceeded) noexcept {
  JsValueRef descriptor{};
  CHECK_JSRT_ERROR_CODE(CreatePropertyDescriptor(std::forward<TValue>(value), attrs, &descriptor));
  return DefineProperty(std::forward<TObject>(object), std::forward<TPropertyId>(propertyId), descriptor, isSucceeded);
}

/*static*/ JsErrorCode Environment::GetHasOwnPropertyFunction(JsValueRef *result) noexcept {
  JsValueRef global{};
  JsPropertyIdRef objectPropertyId{};
  JsValueRef objectCtor{};
  JsValueRef objectPrototype{};
  JsPropertyIdRef hasOwnPropertyId{};
  CHECK_JSRT_ERROR_CODE(JsGetGlobalObject(&global));
  CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromName(L"Object", &objectPropertyId));
  CHECK_JSRT_ERROR_CODE(JsGetProperty(global, objectPropertyId, &objectCtor));
  CHECK_JSRT_ERROR_CODE(JsGetPrototype(objectCtor, &objectPrototype));
  CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromName(L"hasOwnProperty", &hasOwnPropertyId));
  return JsGetProperty(objectPrototype, hasOwnPropertyId, result);
}

template <class TObject, class TPropertyId>
JsErrorCode Environment::HasPrivateProperty(TObject &&object, TPropertyId &&propertyId, bool *result) noexcept {
  JsValueRef jsObject{};
  JsPropertyIdRef jsPropertyId{};
  JsValueRef descriptor{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  JsErrorCode err = JsGetOwnPropertyDescriptor(jsObject, jsPropertyId, &descriptor);
  *result = (err == JsNoError);
  if (!*result) {
    // Discard the last error in case if we cannot retrieve the property descriptor.
    JsValueRef exception{};
    return JsGetAndClearException(&exception);
  }

  return JsErrorCode::JsNoError;
}

template <class TObject, class TPropertyId>
JsErrorCode Environment::GetPrivateProperty(TObject &&object, TPropertyId &&propertyId, JsValueRef *result) noexcept {
  JsValueRef jsObject{};
  JsPropertyIdRef jsPropertyId{};
  JsValueRef descriptor{};
  CHECK_JSRT_ERROR_CODE(CachedValue::GetValue(std::forward<TObject>(object), &jsObject));
  CHECK_JSRT_ERROR_CODE(CachedPropertyId::GetPropertyId(std::forward<TPropertyId>(propertyId), &jsPropertyId));
  CHECK_JSRT_ERROR_CODE(JsGetOwnPropertyDescriptor(jsObject, jsPropertyId, &descriptor));
  return GetProperty(descriptor, m_propertyId.value, result);
}

template <class TObject, class TPropertyId, class TValue>
JsErrorCode Environment::SetPrivateProperty(TObject &&object, TPropertyId &&propertyId, TValue &&value) noexcept {
  JsValueRef descriptor{};
  bool isSucceeded{false};
  CHECK_JSRT_ERROR_CODE(DefineProperty(
      std::forward<TObject>(object),
      std::forward<TPropertyId>(propertyId),
      std::forward<TValue>(value),
      PropertyAttibutes::DontEnum,
      &isSucceeded));
  if (isSucceeded) {
    return JsErrorCode::JsNoError;
  } else {
    return SetProperty(
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
/*static*/ JsErrorCode Environment::CallFunction(TFunction &&function, JsValueRef *result, TArgs &&... args) noexcept {
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
  CHECK_JSRT_ERROR_CODE(GetProperty(m_value.Global, m_propertyId.Promise, &promiseConstructor));

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

} // namespace chakra

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

namespace {
constexpr UINT CP_LATIN1 = 28591;

std::wstring NarrowToWide(std::string_view value, UINT codePage = CP_UTF8) {
  if (value.size() == 0) {
    return {};
  }

  int requiredSize = ::MultiByteToWideChar(codePage, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  assert(requiredSize != 0);
  std::wstring wstr(requiredSize, 0);
  int result = ::MultiByteToWideChar(codePage, 0, value.data(), static_cast<int>(value.size()), &wstr[0], requiredSize);
  assert(result != 0);
  return std::move(wstr);
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
    UINT codePage = CP_UTF8) {
  const wchar_t *stringValue;
  size_t stringLength;
  CHECK_JSRT_ERROR_CODE(JsStringToPointer(value, &stringValue, &stringLength));

  if (length != nullptr) {
    *length = stringLength;
  }

  if (buffer != nullptr) {
    int result = ::WideCharToMultiByte(
        codePage,
        0,
        stringValue,
        static_cast<int>(stringLength),
        buffer,
        static_cast<int>(bufferSize),
        nullptr,
        nullptr);
    assert(result != 0);
  }

  return JsErrorCode::JsNoError;
}

JsErrorCode
JsCopyStringUtf16(_In_ JsValueRef value, _Out_opt_ char16_t *buffer, _In_ size_t bufferSize, _Out_opt_ size_t *length) {
  const wchar_t *stringValue;
  size_t stringLength;
  CHECK_JSRT_ERROR_CODE(JsStringToPointer(value, &stringValue, &stringLength));

  if (length != nullptr) {
    *length = stringLength;
  }

  if (buffer != nullptr) {
    static_assert(sizeof(char16_t) == sizeof(wchar_t));
    memcpy_s(buffer, bufferSize, stringValue, stringLength * sizeof(wchar_t));
  }

  return JsErrorCode::JsNoError;
}

JsErrorCode JsCreatePropertyId(_In_z_ const char *name, _In_ size_t length, _Out_ JsPropertyIdRef *propertyId) {
  auto str = (length == NAPI_AUTO_LENGTH ? NarrowToWide({name}) : NarrowToWide({name, length}));
  return JsGetPropertyIdFromName(str.data(), propertyId);
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
  ExternalData(napi_env env, void *data, napi_finalize finalize_cb, void *hint)
      : _env(env), _data(data), _cb(finalize_cb), _hint(hint) {}

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
  ExternalCallback(napi_env env, napi_callback cb, void *data) : _env(env), _cb(cb), _data(data) {}

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

napi_status FindWrapper(napi_env env, JsValueRef obj, JsValueRef *wrapper, JsValueRef *parent = nullptr) {
  // Search the object's prototype chain for the wrapper with external data.
  // Usually the wrapper would be the first in the chain, but it is OK for
  // other objects to be inserted in the prototype chain.
  JsValueRef candidate = obj;
  JsValueRef current = JS_INVALID_REFERENCE;
  bool hasExternalData = false;

  JsValueRef nullValue = JS_INVALID_REFERENCE;
  CHECK_JSRT(env, JsGetNullValue(&nullValue));

  do {
    current = candidate;

    CHECK_JSRT(env, JsGetPrototype(current, &candidate));
    if (candidate == JS_INVALID_REFERENCE || candidate == nullValue) {
      if (parent != nullptr) {
        *parent = JS_INVALID_REFERENCE;
      }

      *wrapper = JS_INVALID_REFERENCE;
      return napi_ok;
    }

    CHECK_JSRT(env, JsHasExternalData(candidate, &hasExternalData));
  } while (!hasExternalData);

  if (parent != nullptr) {
    *parent = current;
  }

  *wrapper = candidate;

  return napi_ok;
}

napi_status Unwrap(
    napi_env env,
    JsValueRef obj,
    ExternalData **externalData,
    JsValueRef *wrapper = nullptr,
    JsValueRef *parent = nullptr) {
  JsValueRef candidate = JS_INVALID_REFERENCE;
  JsValueRef candidateParent = JS_INVALID_REFERENCE;
  CHECK_NAPI(FindWrapper(env, obj, &candidate, &candidateParent));
  RETURN_STATUS_IF_FALSE(env, candidate != JS_INVALID_REFERENCE, napi_invalid_arg);

  CHECK_JSRT(env, JsGetExternalData(candidate, reinterpret_cast<void **>(externalData)));

  if (wrapper != nullptr) {
    *wrapper = candidate;
  }

  if (parent != nullptr) {
    *parent = candidateParent;
  }

  return napi_ok;
}

napi_status SetErrorCode(napi_env env, JsValueRef error, napi_value code, const char *codeString) {
  if ((code != nullptr) || (codeString != nullptr)) {
    JsValueRef codeValue = reinterpret_cast<JsValueRef>(code);
    if (codeValue != JS_INVALID_REFERENCE) {
      JsValueType valueType = JsUndefined;
      CHECK_JSRT(env, JsGetValueType(codeValue, &valueType));
      RETURN_STATUS_IF_FALSE(env, valueType == JsString, napi_string_expected);
    } else {
      CHECK_JSRT(env, JsCreateString(codeString, NAPI_AUTO_LENGTH, &codeValue));
    }

    JsPropertyIdRef codePropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"code", &codePropId));

    CHECK_JSRT(env, JsSetProperty(error, codePropId, codeValue, true));

    JsValueRef nameArray{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsCreateArray(0, &nameArray));

    JsPropertyIdRef pushPropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"push", &pushPropId));

    JsValueRef pushFunction{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetProperty(nameArray, pushPropId, &pushFunction));

    JsPropertyIdRef namePropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"name", &namePropId));

    bool hasProp = false;
    CHECK_JSRT(env, JsHasProperty(error, namePropId, &hasProp));

    JsValueRef nameValue{JS_INVALID_REFERENCE};
    std::array<JsValueRef, 2> args = {nameArray, JS_INVALID_REFERENCE};

    if (hasProp) {
      CHECK_JSRT(env, JsGetProperty(error, namePropId, &nameValue));

      args[1] = nameValue;
      CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));
    }

    JsValueRef openBracketValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsPointerToString(STR_AND_LENGTH(L" ["), &openBracketValue));

    args[1] = openBracketValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    args[1] = codeValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    JsValueRef closeBracketValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsPointerToString(STR_AND_LENGTH(L"]"), &closeBracketValue));

    args[1] = closeBracketValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    JsValueRef emptyValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsPointerToString(L"", 0, &emptyValue));

    JsPropertyIdRef joinPropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"join", &joinPropId));

    JsValueRef joinFunction = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsGetProperty(nameArray, joinPropId, &joinFunction));

    args[1] = emptyValue;
    CHECK_JSRT(env, JsCallFunction(joinFunction, args.data(), static_cast<unsigned short>(args.size()), &nameValue));

    CHECK_JSRT(env, JsSetProperty(error, namePropId, nameValue, true));
  }
  return napi_ok;
}

napi_status CreatePropertyFunction(
    napi_env env,
    napi_value property_name,
    napi_callback cb,
    void *callback_data,
    napi_value *result) try {
  CHECK_ENV_AND_ARG2(env, property_name, result);

  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(env, cb, callback_data)};

  napi_valuetype nameType;
  CHECK_NAPI(napi_typeof(env, property_name, &nameType));

  JsValueRef function;
  if (nameType == napi_string) {
    JsValueRef name{JS_INVALID_REFERENCE};
    name = property_name;
    CHECK_JSRT(env, JsCreateNamedFunction(name, ExternalCallback::Callback, externalCallback.get(), &function));
  } else {
    CHECK_JSRT(env, JsCreateFunction(ExternalCallback::Callback, externalCallback.get(), &function));
  }

  externalCallback->newTarget = function;

  CHECK_JSRT(env, JsSetObjectBeforeCollectCallback(function, externalCallback.get(), ExternalCallback::Finalize));
  externalCallback.release();

  *result = reinterpret_cast<napi_value>(function);
  return napi_ok;
} catch (...) {
  return env->SetLastError(napi_generic_failure);
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

} // namespace

namespace chakra {
namespace {
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
    CHECK_ARG(env, finalize_cb);
  }

  v8impl::Reference *reference = nullptr;
  if (result != nullptr) {
    // The returned reference should be deleted via napi_delete_reference()
    // ONLY in response to the finalize callback invocation. (If it is deleted
    // before then, then the finalize callback will never be invoked.)
    // Therefore a finalize callback is required when returning a reference.
    CHECK_ARG(env, finalize_cb);
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
} // namespace
} // namespace chakra

//==============================================================================
// NAPI: Getting last error.
//==============================================================================

napi_status napi_get_last_error_info(napi_env env, const napi_extended_error_info **result) {
  CHECK_ENV(env);
  return env->GetLastErrorInfo(result);
}

//==============================================================================
// NAPI: Getters for defined singletons
//==============================================================================

napi_status napi_get_undefined(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsGetUndefinedValue(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_null(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsGetNullValue(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_global(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsGetGlobalObject(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsBoolToBoolean(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to create Primitive types/Objects
//==============================================================================

napi_status napi_create_object(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateObject(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_array(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateArray(0, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateArray(static_cast<unsigned int>(length), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_double(napi_env env, double value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_int32(napi_env env, int32_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsIntToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_int64(napi_env env, int64_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_string_latin1(napi_env env, const char *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  std::wstring wstr =
      (length == NAPI_AUTO_LENGTH) ? NarrowToWide({str}, CP_LATIN1) : NarrowToWide({str, length}, CP_LATIN1);
  CHECK_JSRT(env, JsPointerToString(wstr.data(), wstr.size(), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_string_utf8(napi_env env, const char *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateString(str, length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  static_assert(sizeof(char16_t) == sizeof(wchar_t));
  CHECK_JSRT(
      env, JsPointerToString(reinterpret_cast<const wchar_t *>(str), length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_symbol(napi_env env, napi_value description, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef js_description = reinterpret_cast<JsValueRef>(description);
  CHECK_JSRT(env, JsCreateSymbol(js_description, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_function(
    napi_env env,
    const char *utf8name,
    size_t length,
    napi_callback cb,
    void *data,
    napi_value *result) try {
  CHECK_ENV_AND_ARG(env, result);

  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(env, cb, data)};

  JsValueRef function;
  if (utf8name != nullptr) {
    JsValueRef name{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsCreateString(utf8name, length, &name));
    CHECK_JSRT(env, JsCreateNamedFunction(name, ExternalCallback::Callback, externalCallback.get(), &function));
  } else {
    CHECK_JSRT(env, JsCreateFunction(ExternalCallback::Callback, externalCallback.get(), &function));
  }

  externalCallback->newTarget = function;

  CHECK_JSRT(env, JsSetObjectBeforeCollectCallback(function, externalCallback.get(), ExternalCallback::Finalize));
  externalCallback.release();

  *result = reinterpret_cast<napi_value>(function);
  return napi_ok;
} catch (...) {
  return env->SetLastError(napi_generic_failure);
}

napi_status napi_create_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreateError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

napi_status napi_create_type_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreateTypeError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

napi_status napi_create_range_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreateRangeError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to get the native napi_value from Primitive type
//==============================================================================

napi_status napi_typeof(napi_env env, napi_value value, napi_valuetype *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType = JsUndefined;
  CHECK_JSRT(env, JsGetValueType(jsValue, &valueType));

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

napi_status napi_get_value_double(napi_env env, napi_value value, double *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT_EXPECTED(env, JsNumberToDouble(jsValue, result), napi_number_expected);
  return napi_ok;
}

napi_status napi_get_value_int32(napi_env env, napi_value value, int32_t *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  int valueInt;
  CHECK_JSRT_EXPECTED(env, JsNumberToInt(jsValue, &valueInt), napi_number_expected);
  *result = static_cast<int32_t>(valueInt);
  return napi_ok;
}

napi_status napi_get_value_uint32(napi_env env, napi_value value, uint32_t *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  double valueDouble;
  CHECK_JSRT_EXPECTED(env, JsNumberToDouble(jsValue, &valueDouble), napi_number_expected);
  if (std::isfinite(valueDouble)) {
    *result = static_cast<int32_t>(valueDouble);
  } else {
    *result = 0;
  }
  return napi_ok;
}

napi_status napi_get_value_int64(napi_env env, napi_value value, int64_t *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  double valueDouble;
  CHECK_JSRT_EXPECTED(env, JsNumberToDouble(jsValue, &valueDouble), napi_number_expected);

  if (std::isfinite(valueDouble)) {
    *result = static_cast<int64_t>(valueDouble);
  } else {
    *result = 0;
  }

  return napi_ok;
}

napi_status napi_get_value_bool(napi_env env, napi_value value, bool *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT_EXPECTED(env, JsBooleanToBool(jsValue, result), napi_boolean_expected);
  return napi_ok;
}

// Copies a JavaScript string into a LATIN-1 string buffer. The result is the
// number of bytes (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in bytes)
// via the result parameter.
// The result argument is optional unless buf is NULL.
napi_status napi_get_value_string_latin1(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
  CHECK_ENV_AND_ARG(env, value);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(env, result);
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, result, CP_LATIN1), napi_string_expected);
  } else {
    size_t count = 0;
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, &count), napi_string_expected);

    if (bufsize <= count) {
      // if bufsize == count there is no space for null terminator
      // Slow path: must implement truncation here.
      std::unique_ptr<char[]> fullBuffer{new char[count]};
      // CHAKRA_VERIFY(fullBuffer != nullptr);

      CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, fullBuffer.get(), count, nullptr), napi_string_expected);
      memmove(buf, fullBuffer.get(), sizeof(char) * bufsize);
      fullBuffer.reset();

      // Truncate string to the start of the last codepoint
      if (bufsize > 0 && (((buf[bufsize - 1] & 0x80) == 0) || UTF8_MULTIBYTE_START(buf[bufsize - 1]))) {
        // Last byte is a single byte codepoint or
        // starts a multibyte codepoint
        bufsize -= 1;
      } else if (bufsize > 1 && UTF8_MULTIBYTE_START(buf[bufsize - 2])) {
        // Second last byte starts a multibyte codepoint,
        bufsize -= 2;
      } else if (bufsize > 2 && UTF8_MULTIBYTE_START(buf[bufsize - 3])) {
        // Third last byte starts a multibyte codepoint
        bufsize -= 3;
      } else if (bufsize > 3 && UTF8_MULTIBYTE_START(buf[bufsize - 4])) {
        // Fourth last byte starts a multibyte codepoint
        bufsize -= 4;
      }

      buf[bufsize] = '\0';

      if (result) {
        *result = bufsize;
      }

      return napi_ok;
    }

    // Fast path, result fits in the buffer
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, buf, bufsize - 1, &count), napi_string_expected);

    buf[count] = 0;

    if (result != nullptr) {
      *result = count;
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
napi_status napi_get_value_string_utf8(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
  CHECK_ENV_AND_ARG(env, value);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(env, result);
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, result), napi_string_expected);
  } else {
    size_t count = 0;
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, &count), napi_string_expected);

    if (bufsize <= count) {
      // if bufsize == count there is no space for null terminator
      // Slow path: must implement truncation here.
      std::unique_ptr<char[]> fullBuffer{new char[count]};
      // CHAKRA_VERIFY(fullBuffer != nullptr);

      CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, fullBuffer.get(), count, nullptr), napi_string_expected);
      memmove(buf, fullBuffer.get(), sizeof(char) * bufsize);
      fullBuffer.reset();

      // Truncate string to the start of the last codepoint
      if (bufsize > 0 && (((buf[bufsize - 1] & 0x80) == 0) || UTF8_MULTIBYTE_START(buf[bufsize - 1]))) {
        // Last byte is a single byte codepoint or
        // starts a multibyte codepoint
        bufsize -= 1;
      } else if (bufsize > 1 && UTF8_MULTIBYTE_START(buf[bufsize - 2])) {
        // Second last byte starts a multibyte codepoint,
        bufsize -= 2;
      } else if (bufsize > 2 && UTF8_MULTIBYTE_START(buf[bufsize - 3])) {
        // Third last byte starts a multibyte codepoint
        bufsize -= 3;
      } else if (bufsize > 3 && UTF8_MULTIBYTE_START(buf[bufsize - 4])) {
        // Fourth last byte starts a multibyte codepoint
        bufsize -= 4;
      }

      buf[bufsize] = '\0';

      if (result) {
        *result = bufsize;
      }

      return napi_ok;
    }

    // Fast path, result fits in the buffer
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, buf, bufsize - 1, &count), napi_string_expected);

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
napi_status napi_get_value_string_utf16(napi_env env, napi_value value, char16_t *buf, size_t bufsize, size_t *result) {
  CHECK_ENV_AND_ARG(env, value);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(env, result);

    CHECK_JSRT_EXPECTED(env, JsCopyStringUtf16(jsValue, nullptr, 0, result), napi_string_expected);
  } else {
    size_t copied = 0;
    CHECK_JSRT_EXPECTED(env, JsCopyStringUtf16(jsValue, buf, bufsize - 1, &copied), napi_string_expected);

    if (copied < bufsize - 1) {
      buf[copied] = 0;
    } else {
      buf[bufsize - 1] = 0;
    }

    if (result != nullptr) {
      *result = copied;
    }
  }

  return napi_ok;
}

//==============================================================================
// NAPI: Methods to coerce values
// These APIs may execute user scripts
//==============================================================================

napi_status napi_coerce_to_bool(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToBoolean(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_coerce_to_number(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToNumber(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_coerce_to_object(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToObject(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_coerce_to_string(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToString(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to work with Objects
//==============================================================================

napi_status napi_get_prototype(napi_env env, napi_value object, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsGetPrototype(obj, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_property_names(napi_env env, napi_value object, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef propertyNames{JS_INVALID_REFERENCE};
  // TODO: [vmoroz] Check the V8 implementation to make sure that this implementation is correct.
  CHECK_JSRT(env, JsGetOwnPropertyNames(obj, &propertyNames));
  *result = reinterpret_cast<napi_value>(propertyNames);
  return napi_ok;
}

napi_status napi_set_property(napi_env env, napi_value object, napi_value key, napi_value value) {
  CHECK_ENV_AND_ARG2(env, key, value);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  JsValueRef js_value = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsSetProperty(obj, propertyId, js_value, /*useStrictRules:*/ true));
  return napi_ok;
}

napi_status napi_has_property(napi_env env, napi_value object, napi_value key, bool *result) {
  CHECK_ENV_AND_ARG2(env, key, result);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsHasProperty(obj, propertyId, result));
  return napi_ok;
}

napi_status napi_get_property(napi_env env, napi_value object, napi_value key, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, key, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  CHECK_JSRT(env, JsGetProperty(obj, propertyId, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_delete_property(napi_env env, napi_value object, napi_value key, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  *result = false;

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  JsValueRef deletePropertyResult{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsDeleteProperty(obj, propertyId, false /* isStrictMode */, &deletePropertyResult));
  CHECK_JSRT(env, JsBooleanToBool(deletePropertyResult, result));
  return napi_ok;
}

napi_status napi_has_own_property(napi_env env, napi_value object, napi_value key, bool *result) {
  return CHECKED_ENV(env)->HasOwnProperty(object, key, result);
}

napi_status napi_set_named_property(napi_env env, napi_value object, const char *utf8name, napi_value value) {
  CHECK_ENV_AND_ARG(env, value);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreatePropertyId(utf8name, NAPI_AUTO_LENGTH, &propertyId));
  JsValueRef js_value = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsSetProperty(obj, propertyId, js_value, true));
  return napi_ok;
}

napi_status napi_has_named_property(napi_env env, napi_value object, const char *utf8name, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreatePropertyId(utf8name, strlen(utf8name), &propertyId));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsHasProperty(obj, propertyId, result));
  return napi_ok;
}

napi_status napi_get_named_property(napi_env env, napi_value object, const char *utf8name, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreatePropertyId(utf8name, strlen(utf8name), &propertyId));
  CHECK_JSRT(env, JsGetProperty(obj, propertyId, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value) {
  CHECK_ENV_AND_ARG(env, value);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(index, &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsSetIndexedProperty(obj, jsIndex, jsValue));
  return napi_ok;
}

napi_status napi_has_element(napi_env env, napi_value object, uint32_t index, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(static_cast<int>(index), &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsHasIndexedProperty(obj, jsIndex, result));
  return napi_ok;
}

napi_status napi_get_element(napi_env env, napi_value object, uint32_t index, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(static_cast<int>(index), &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsGetIndexedProperty(obj, jsIndex, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_delete_element(napi_env env, napi_value object, uint32_t index, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(index, &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsDeleteIndexedProperty(obj, jsIndex));
  // TODO: [vmoroz] Check the result value
  *result = true;
  return napi_ok;
}

napi_status napi_define_properties(
    napi_env env,
    napi_value object,
    size_t property_count,
    const napi_property_descriptor *properties) {
  CHECK_ENV_AND_ARG(env, object);
  if (property_count > 0) {
    CHECK_ARG(env, properties);
  }

  JsPropertyIdRef configurableProperty{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsGetPropertyIdFromName(L"configurable", &configurableProperty));

  // TODO: [vmoroz] Add cached property ID
  JsPropertyIdRef enumerableProperty{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsGetPropertyIdFromName(L"enumerable", &enumerableProperty));

  for (size_t i = 0; i < property_count; i++) {
    const napi_property_descriptor *p = properties + i;

    JsValueRef descriptor{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsCreateObject(&descriptor));

    if (p->attributes & napi_configurable) {
      // TODO: [vmoroz] Add cached true/false JsValue
      JsValueRef configurable{JS_INVALID_REFERENCE};
      CHECK_JSRT(env, JsBoolToBoolean(true, &configurable));
      CHECK_JSRT(env, JsSetProperty(descriptor, configurableProperty, configurable, true));
    }

    if (p->attributes & napi_enumerable) {
      JsValueRef enumerable{JS_INVALID_REFERENCE};
      CHECK_JSRT(env, JsBoolToBoolean(true, &enumerable));
      CHECK_JSRT(env, JsSetProperty(descriptor, enumerableProperty, enumerable, true));
    }

    if (p->getter != nullptr || p->setter != nullptr) {
      napi_value property_name;
      CHECK_JSRT(env, JsNameValueFromPropertyDescriptor(p, &property_name));

      if (p->getter != nullptr) {
        JsPropertyIdRef getProperty;
        CHECK_JSRT(env, JsGetPropertyIdFromName(L"get", &getProperty));
        JsValueRef getter;
        CHECK_NAPI(
            CreatePropertyFunction(env, property_name, p->getter, p->data, reinterpret_cast<napi_value *>(&getter)));
        CHECK_JSRT(env, JsSetProperty(descriptor, getProperty, getter, true));
      }

      if (p->setter != nullptr) {
        JsPropertyIdRef setProperty;
        CHECK_JSRT(env, JsGetPropertyIdFromName(L"set", &setProperty));
        JsValueRef setter;
        CHECK_NAPI(
            CreatePropertyFunction(env, property_name, p->setter, p->data, reinterpret_cast<napi_value *>(&setter)));
        CHECK_JSRT(env, JsSetProperty(descriptor, setProperty, setter, true));
      }
    } else if (p->method != nullptr) {
      napi_value property_name;
      CHECK_JSRT(env, JsNameValueFromPropertyDescriptor(p, &property_name));

      JsPropertyIdRef valueProperty;
      CHECK_JSRT(env, JsGetPropertyIdFromName(L"value", &valueProperty));
      JsValueRef method;
      CHECK_NAPI(
          CreatePropertyFunction(env, property_name, p->method, p->data, reinterpret_cast<napi_value *>(&method)));
      CHECK_JSRT(env, JsSetProperty(descriptor, valueProperty, method, true));
    } else {
      RETURN_STATUS_IF_FALSE(env, p->value != nullptr, napi_invalid_arg);

      if (p->attributes & napi_writable) {
        JsPropertyIdRef writableProperty;
        CHECK_JSRT(env, JsGetPropertyIdFromName(L"writable", &writableProperty));
        JsValueRef writable;
        CHECK_JSRT(env, JsBoolToBoolean(true, &writable));
        CHECK_JSRT(env, JsSetProperty(descriptor, writableProperty, writable, true));
      }

      JsPropertyIdRef valueProperty;
      CHECK_JSRT(env, JsGetPropertyIdFromName(L"value", &valueProperty));
      CHECK_JSRT(env, JsSetProperty(descriptor, valueProperty, reinterpret_cast<JsValueRef>(p->value), true));
    }

    JsPropertyIdRef nameProperty;
    CHECK_JSRT(env, JsPropertyIdFromPropertyDescriptor(p, &nameProperty));
    bool result;
    CHECK_JSRT(
        env,
        JsDefineProperty(
            reinterpret_cast<JsValueRef>(object),
            reinterpret_cast<JsPropertyIdRef>(nameProperty),
            reinterpret_cast<JsValueRef>(descriptor),
            &result));
  }

  return napi_ok;
}

//==============================================================================
// NAPI: Methods to work with Arrays
//==============================================================================
napi_status napi_is_array(napi_env env, napi_value value, bool *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType type = JsUndefined;
  CHECK_JSRT(env, JsGetValueType(jsValue, &type));
  *result = (type == JsArray);
  return napi_ok;
}

napi_status napi_get_array_length(napi_env env, napi_value value, uint32_t *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsPropertyIdRef propertyIdRef;
  CHECK_JSRT(env, JsGetPropertyIdFromName(L"length", &propertyIdRef));
  JsValueRef lengthRef;
  JsValueRef arrayRef = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsGetProperty(arrayRef, propertyIdRef, &lengthRef));
  double sizeInDouble;
  CHECK_JSRT(env, JsNumberToDouble(lengthRef, &sizeInDouble));
  *result = static_cast<uint32_t>(sizeInDouble);
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to compare values
//==============================================================================
napi_status napi_strict_equals(napi_env env, napi_value lhs, napi_value rhs, bool *result) {
  CHECK_ENV_AND_ARG3(env, lhs, rhs, result);
  JsValueRef object1 = reinterpret_cast<JsValueRef>(lhs);
  JsValueRef object2 = reinterpret_cast<JsValueRef>(rhs);
  CHECK_JSRT(env, JsStrictEquals(object1, object2, result));
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to work with Functions
//==============================================================================

// The number of arguments that we keep on stack.
// We use heap if we have more argument.
constexpr static size_t MaxStackArgCount = 8;

napi_status napi_call_function(
    napi_env env,
    napi_value recv,
    napi_value func,
    size_t argc,
    const napi_value *argv,
    napi_value *result) {
  CHECK_ENV_AND_ARG(env, recv);
  if (argc > 0) {
    CHECK_ARG(env, argv);
  }

  JsValueRef function = reinterpret_cast<JsValueRef>(func);
  JsValueArgs args{recv, Span<napi_value>{argv, argc}};
  JsValueRef returnValue;
  CHECK_JSRT(env, JsCallFunction(function, args.Data(), static_cast<uint16_t>(args.Size()), &returnValue));
  if (result != nullptr) {
    *result = reinterpret_cast<napi_value>(returnValue);
  }
  return napi_ok;
}

napi_status
napi_new_instance(napi_env env, napi_value constructor, size_t argc, const napi_value *argv, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, constructor, result);
  if (argc > 0) {
    CHECK_ARG(env, argv);
  }
  JsValueRef function = reinterpret_cast<JsValueRef>(constructor);
  napi_value thisArg;
  CHECK_NAPI(napi_get_undefined(env, &thisArg));
  JsValueArgs args{thisArg, Span<napi_value>{argv, argc}};
  CHECK_JSRT(
      env,
      JsConstructObject(
          function, args.Data(), static_cast<uint16_t>(args.Size()), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_instanceof(napi_env env, napi_value object, napi_value constructor, bool *result) {
  CHECK_ENV_AND_ARG2(env, object, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef jsConstructor = reinterpret_cast<JsValueRef>(constructor);

  // FIXME: Remove this type check when we switch to a version of Chakracore
  // where passing an integer into JsInstanceOf as the constructor parameter
  // does not cause a segfault. The need for this if-statement is removed in at
  // least Chakracore 1.4.0, but maybe in an earlier version too.
  napi_valuetype valuetype;
  CHECK_NAPI(napi_typeof(env, constructor, &valuetype));
  if (valuetype != napi_function) {
    napi_throw_type_error(env, "ERR_NAPI_CONS_FUNCTION", "constructor must be a function");

    return env->SetLastError(napi_invalid_arg);
  }

  CHECK_JSRT(env, JsInstanceOf(obj, jsConstructor, result));
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to work with napi_callbacks
//==============================================================================

// Gets all callback info in a single call. (Ugly, but faster.)
napi_status napi_get_cb_info(
    napi_env env, // [in] NAPI environment handle
    napi_callback_info cbinfo, // [in] Opaque callback-info handle
    size_t *argc, // [in-out] Specifies the size of the provided argv array
                  // and receives the actual count of args.
    napi_value *argv, // [out] Array of values
    napi_value *this_arg, // [out] Receives the JS 'this' arg for the call
    void **data) // [out] Receives the data pointer for the callback.
{
  CHECK_ENV_AND_ARG(env, cbinfo);
  const CallbackInfo *info = reinterpret_cast<CallbackInfo *>(cbinfo);

  if (argv != nullptr) {
    CHECK_ARG(env, argc);

    size_t i = 0;
    size_t min = (std::min)(*argc, static_cast<size_t>(info->argc));

    for (; i < min; i++) {
      argv[i] = info->argv[i];
    }

    if (i < *argc) {
      napi_value undefined;
      CHECK_JSRT(env, JsGetUndefinedValue(reinterpret_cast<JsValueRef *>(&undefined)));
      for (; i < *argc; i++) {
        argv[i] = undefined;
      }
    }
  }

  if (argc != nullptr) {
    *argc = info->argc;
  }

  if (this_arg != nullptr) {
    *this_arg = info->thisArg;
  }

  if (data != nullptr) {
    *data = info->data;
  }

  return napi_ok;
}

napi_status napi_get_new_target(napi_env env, napi_callback_info cbinfo, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, cbinfo, result);

  const CallbackInfo *info = reinterpret_cast<CallbackInfo *>(cbinfo);
  if (info->isConstructCall) {
    *result = info->newTarget;
  } else {
    *result = nullptr;
  }

  return napi_ok;
}

//==============================================================================
// NAPI: Methods to work with external data objects
//==============================================================================

napi_status napi_define_class(
    napi_env env,
    const char *utf8name,
    size_t length,
    napi_callback constructor,
    void *data,
    size_t property_count,
    const napi_property_descriptor *properties,
    napi_value *result) try {
  CHECK_ENV_AND_ARG(env, result);

  napi_value namestring;
  CHECK_NAPI(napi_create_string_utf8(env, utf8name, length, &namestring));

  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(env, constructor, data)};

  JsValueRef jsConstructor;
  CHECK_JSRT(
      env, JsCreateNamedFunction(namestring, ExternalCallback::Callback, externalCallback.get(), &jsConstructor));

  externalCallback->newTarget = jsConstructor;

  CHECK_JSRT(env, JsSetObjectBeforeCollectCallback(jsConstructor, externalCallback.get(), ExternalCallback::Finalize));
  externalCallback.release();

  JsPropertyIdRef pid = nullptr;
  JsValueRef prototype = nullptr;
  CHECK_JSRT(env, JsGetPropertyIdFromName(L"prototype", &pid));
  CHECK_JSRT(env, JsGetProperty(jsConstructor, pid, &prototype));

  CHECK_JSRT(env, JsGetPropertyIdFromName(L"constructor", &pid));
  CHECK_JSRT(env, JsSetProperty(prototype, pid, jsConstructor, false));

  int instancePropertyCount = 0;
  int staticPropertyCount = 0;
  for (size_t i = 0; i < property_count; i++) {
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

  for (size_t i = 0; i < property_count; i++) {
    if ((properties[i].attributes & napi_static) != 0) {
      staticDescriptors.push_back(properties[i]);
    } else {
      instanceDescriptors.push_back(properties[i]);
    }
  }

  if (staticPropertyCount > 0) {
    CHECK_NAPI(napi_define_properties(
        env, reinterpret_cast<napi_value>(constructor), staticDescriptors.size(), staticDescriptors.data()));
  }

  if (instancePropertyCount > 0) {
    CHECK_NAPI(napi_define_properties(
        env, reinterpret_cast<napi_value>(prototype), instanceDescriptors.size(), instanceDescriptors.data()));
  }

  *result = reinterpret_cast<napi_value>(constructor);
  return napi_ok;
} catch (...) {
  return env->SetLastError(napi_generic_failure);
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
// try {
//  CHECK_ENV_AND_ARG(env, js_object);
//  // TODO: [vmoroz] change wrapping to be based on a symbol property
//  JsValueRef value = reinterpret_cast<JsValueRef>(js_object);
//
//  JsValueRef wrapper = JS_INVALID_REFERENCE;
//  CHECK_NAPI(FindWrapper(env, value, &wrapper));
//  RETURN_STATUS_IF_FALSE(env, wrapper == JS_INVALID_REFERENCE, napi_invalid_arg);
//
//  std::unique_ptr<ExternalData> externalData{new ExternalData(env, native_object, finalize_cb, finalize_hint)};
//
//  // Create an external object that will hold the external data pointer.
//  JsValueRef external = JS_INVALID_REFERENCE;
//  CHECK_JSRT(env, JsCreateExternalObject(externalData.get(), ExternalData::Finalize, &external));
//  externalData.release();
//
//  // Insert the external object into the value's prototype chain.
//  JsValueRef valuePrototype = JS_INVALID_REFERENCE;
//  CHECK_JSRT(env, JsGetPrototype(value, &valuePrototype));
//  CHECK_JSRT(env, JsSetPrototype(external, valuePrototype));
//  CHECK_JSRT(env, JsSetPrototype(value, external));
//
//  if (result != nullptr) {
//    CHECK_NAPI(napi_create_reference(env, js_object, 0, result));
//  }
//
//  return napi_ok;
//} catch (...) {
//  return env->SetLastError(napi_generic_failure);
//}

napi_status napi_unwrap(napi_env env, napi_value js_object, void **result) {
  CHECK_ENV_AND_ARG(env, js_object);

  JsValueRef value = reinterpret_cast<JsValueRef>(js_object);

  ExternalData *externalData = nullptr;
  CHECK_NAPI(Unwrap(env, value, &externalData));

  *result = (externalData != nullptr ? externalData->Data() : nullptr);

  return napi_ok;
}

napi_status napi_remove_wrap(napi_env env, napi_value js_object, void **result) {
  CHECK_ENV_AND_ARG(env, js_object);

  JsValueRef value = reinterpret_cast<JsValueRef>(js_object);

  ExternalData *externalData = nullptr;
  JsValueRef parent = JS_INVALID_REFERENCE;
  JsValueRef wrapper = JS_INVALID_REFERENCE;
  CHECK_NAPI(Unwrap(env, value, &externalData, &wrapper, &parent));
  RETURN_STATUS_IF_FALSE(env, parent != JS_INVALID_REFERENCE, napi_invalid_arg);
  RETURN_STATUS_IF_FALSE(env, wrapper != JS_INVALID_REFERENCE, napi_invalid_arg);

  // Remove the external from the prototype chain
  JsValueRef wrapperProto = JS_INVALID_REFERENCE;
  CHECK_JSRT(env, JsGetPrototype(wrapper, &wrapperProto));
  CHECK_JSRT(env, JsSetPrototype(parent, wrapperProto));

  // Clear the external data from the object
  CHECK_JSRT(env, JsSetExternalData(wrapper, nullptr));

  if (externalData != nullptr) {
    *result = externalData->Data();
    delete externalData;
  } else {
    *result = nullptr;
  }

  return napi_ok;
}

napi_status
napi_create_external(napi_env env, void *data, napi_finalize finalize_cb, void *finalize_hint, napi_value *result) try {
  CHECK_ENV_AND_ARG(env, result);
  std::unique_ptr<ExternalData> externalData{new ExternalData(env, data, finalize_cb, finalize_hint)};

  CHECK_JSRT(
      env, JsCreateExternalObject(externalData.get(), ExternalData::Finalize, reinterpret_cast<JsValueRef *>(result)));
  externalData.release();

  return napi_ok;
} catch (...) {
  return env->SetLastError(napi_generic_failure);
}

napi_status napi_get_value_external(napi_env env, napi_value value, void **result) {
  ExternalData *externalData;
  CHECK_JSRT(env, JsGetExternalData(reinterpret_cast<JsValueRef>(value), reinterpret_cast<void **>(&externalData)));

  *result = (externalData != nullptr ? externalData->Data() : nullptr);

  return napi_ok;
}

//==============================================================================
// NAPI: Methods to control object lifespan
//==============================================================================

napi_status napi_create_reference(napi_env env, napi_value value, uint32_t initial_refcount, napi_ref *result) {
  return chakra::Reference::New(env, value, initial_refcount, result);
}

napi_status napi_delete_reference(napi_env env, napi_ref ref) {
  return CHECKED_REF(ref)->Delete(env);
}

napi_status napi_reference_ref(napi_env env, napi_ref ref, uint32_t *result) {
  return CHECKED_REF(ref)->Ref(env, result);
}

napi_status napi_reference_unref(napi_env env, napi_ref ref, uint32_t *result) {
  return CHECKED_REF(ref)->Unref(env, result);
}

napi_status napi_get_reference_value(napi_env env, napi_ref ref, napi_value *result) {
  return CHECKED_REF(ref)->Value(env, result);
}

// Stub implementation of handle scope apis for JSRT.
napi_status napi_open_handle_scope(napi_env env, napi_handle_scope *result) {
  CHECK_ENV_AND_ARG(env, result);
  *result = reinterpret_cast<napi_handle_scope>(1);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status napi_close_handle_scope(napi_env env, napi_handle_scope scope) {
  CHECK_ENV_AND_ARG(env, scope);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status napi_open_escapable_handle_scope(napi_env env, napi_escapable_handle_scope *result) {
  CHECK_ENV_AND_ARG(env, result);
  *result = reinterpret_cast<napi_escapable_handle_scope>(1);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status napi_close_escapable_handle_scope(napi_env env, napi_escapable_handle_scope scope) {
  CHECK_ENV_AND_ARG(env, scope);
  return napi_ok;
}

// Stub implementation of handle scope apis for JSRT.
napi_status
napi_escape_handle(napi_env env, napi_escapable_handle_scope scope, napi_value escapee, napi_value *result) {
  CHECK_ENV_AND_ARG3(env, scope, escapee, result);
  *result = escapee;
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to support error handling
//==============================================================================

napi_status napi_throw(napi_env env, napi_value error) {
  CHECK_ENV(env);
  JsValueRef exception = reinterpret_cast<JsValueRef>(error);
  CHECK_JSRT(env, JsSetException(exception));
  return napi_ok;
}

napi_status napi_throw_error(napi_env env, const char *code, const char *msg) {
  CHECK_ENV(env);
  JsValueRef strRef;
  JsValueRef exception;
  size_t length = strlen(msg);
  CHECK_JSRT(env, JsCreateString(msg, length, &strRef));
  CHECK_JSRT(env, JsCreateError(strRef, &exception));
  CHECK_NAPI(SetErrorCode(env, exception, nullptr, code));
  CHECK_JSRT(env, JsSetException(exception));
  return napi_ok;
}

napi_status napi_throw_type_error(napi_env env, const char *code, const char *msg) {
  CHECK_ENV(env);
  JsValueRef strRef;
  JsValueRef exception;
  size_t length = strlen(msg);
  CHECK_JSRT(env, JsCreateString(msg, length, &strRef));
  CHECK_JSRT(env, JsCreateTypeError(strRef, &exception));
  CHECK_NAPI(SetErrorCode(env, exception, nullptr, code));
  CHECK_JSRT(env, JsSetException(exception));
  return napi_ok;
}

napi_status napi_throw_range_error(napi_env env, const char *code, const char *msg) {
  CHECK_ENV(env);
  JsValueRef strRef;
  JsValueRef exception;
  size_t length = strlen(msg);
  CHECK_JSRT(env, JsCreateString(msg, length, &strRef));
  CHECK_JSRT(env, JsCreateRangeError(strRef, &exception));
  CHECK_NAPI(SetErrorCode(env, exception, nullptr, code));
  CHECK_JSRT(env, JsSetException(exception));
  return napi_ok;
}

napi_status napi_is_error(napi_env env, napi_value value, bool *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueType valueType;
  CHECK_JSRT(env, JsGetValueType(value, &valueType));
  *result = (valueType == JsError);
  return napi_ok;
}

//==============================================================================
// NAPI: Methods to support catching exceptions
//==============================================================================

napi_status napi_is_exception_pending(napi_env env, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsHasException(result));
  return napi_ok;
}

napi_status napi_get_and_clear_last_exception(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);

  bool hasException;
  CHECK_JSRT(env, JsHasException(&hasException));
  if (hasException) {
    CHECK_JSRT(env, JsGetAndClearException(reinterpret_cast<JsValueRef *>(result)));
  } else {
    CHECK_NAPI(napi_get_undefined(env, result));
  }

  return napi_ok;
}

//==============================================================================
// NAPI: Methods to work with array buffers and typed arrays
//==============================================================================

napi_status napi_is_arraybuffer(napi_env env, napi_value value, bool *result) {
  CHECK_ENV_AND_ARG2(env, value, result);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType;
  CHECK_JSRT(env, JsGetValueType(jsValue, &valueType));

  *result = (valueType == JsArrayBuffer);
  return napi_ok;
}

napi_status napi_create_arraybuffer(napi_env env, size_t byte_length, void **data, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);

  JsValueRef arrayBuffer;
  CHECK_JSRT(env, JsCreateArrayBuffer(static_cast<unsigned int>(byte_length), &arrayBuffer));

  if (data != nullptr) {
    CHECK_JSRT(
        env,
        JsGetArrayBufferStorage(
            arrayBuffer, reinterpret_cast<BYTE **>(data), reinterpret_cast<unsigned int *>(&byte_length)));
  }

  *result = reinterpret_cast<napi_value>(arrayBuffer);
  return napi_ok;
}

napi_status napi_create_external_arraybuffer(
    napi_env env,
    void *external_data,
    size_t byte_length,
    napi_finalize finalize_cb,
    void *finalize_hint,
    napi_value *result) try {
  CHECK_ENV_AND_ARG(env, result);

  std::unique_ptr<ExternalData> externalData{new ExternalData(env, external_data, finalize_cb, finalize_hint)};

  JsValueRef arrayBuffer;
  CHECK_JSRT(
      env,
      JsCreateExternalArrayBuffer(
          external_data,
          static_cast<unsigned int>(byte_length),
          ExternalData::Finalize,
          externalData.get(),
          &arrayBuffer));
  externalData.release();

  *result = reinterpret_cast<napi_value>(arrayBuffer);
  return napi_ok;
} catch (...) {
  return env->SetLastError(napi_generic_failure);
}

napi_status napi_get_arraybuffer_info(napi_env env, napi_value arraybuffer, void **data, size_t *byte_length) {
  CHECK_ENV_AND_ARG(env, arraybuffer);

  BYTE *storageData;
  unsigned int storageLength;
  CHECK_JSRT(env, JsGetArrayBufferStorage(reinterpret_cast<JsValueRef>(arraybuffer), &storageData, &storageLength));

  if (data != nullptr) {
    *data = reinterpret_cast<void *>(storageData);
  }

  if (byte_length != nullptr) {
    *byte_length = static_cast<size_t>(storageLength);
  }

  return napi_ok;
}

napi_status napi_is_typedarray(napi_env env, napi_value value, bool *result) {
  CHECK_ENV_AND_ARG2(env, value, result);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType;
  CHECK_JSRT(env, JsGetValueType(jsValue, &valueType));

  *result = (valueType == JsTypedArray);
  return napi_ok;
}

napi_status napi_create_typedarray(
    napi_env env,
    napi_typedarray_type type,
    size_t length,
    napi_value arraybuffer,
    size_t byte_offset,
    napi_value *result) {
  CHECK_ENV_AND_ARG2(env, arraybuffer, result);

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
      return env->SetLastError(napi_invalid_arg);
  }

  JsValueRef jsArrayBuffer = reinterpret_cast<JsValueRef>(arraybuffer);

  CHECK_JSRT(
      env,
      JsCreateTypedArray(
          jsType,
          jsArrayBuffer,
          static_cast<unsigned int>(byte_offset),
          static_cast<unsigned int>(length),
          reinterpret_cast<JsValueRef *>(result)));

  return napi_ok;
}

napi_status napi_get_typedarray_info(
    napi_env env,
    napi_value typedarray,
    napi_typedarray_type *type,
    size_t *length,
    void **data,
    napi_value *arraybuffer,
    size_t *byte_offset) {
  CHECK_ENV_AND_ARG(env, typedarray);

  JsTypedArrayType jsType;
  JsValueRef jsArrayBuffer;
  unsigned int byteOffset;
  unsigned int byteLength;
  BYTE *bufferData;
  unsigned int bufferLength;
  int elementSize;

  CHECK_JSRT(
      env,
      JsGetTypedArrayInfo(reinterpret_cast<JsValueRef>(typedarray), &jsType, &jsArrayBuffer, &byteOffset, &byteLength));

  CHECK_JSRT(
      env,
      JsGetTypedArrayStorage(
          reinterpret_cast<JsValueRef>(typedarray), &bufferData, &bufferLength, &jsType, &elementSize));

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
        return env->SetLastError(napi_generic_failure);
    }
  }

  if (length != nullptr) {
    *length = static_cast<size_t>(byteLength / elementSize);
  }

  if (data != nullptr) {
    *data = static_cast<uint8_t *>(bufferData);
  }

  if (arraybuffer != nullptr) {
    *arraybuffer = reinterpret_cast<napi_value>(jsArrayBuffer);
  }

  if (byte_offset != nullptr) {
    *byte_offset = static_cast<size_t>(byteOffset);
  }

  return napi_ok;
}

napi_status
napi_create_dataview(napi_env env, size_t byte_length, napi_value arraybuffer, size_t byte_offset, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, arraybuffer, result);

  JsValueRef jsArrayBuffer = reinterpret_cast<JsValueRef>(arraybuffer);

  BYTE *unused = nullptr;
  unsigned int bufferLength = 0;

  CHECK_JSRT(env, JsGetArrayBufferStorage(jsArrayBuffer, &unused, &bufferLength));

  if (byte_length + byte_offset > bufferLength) {
    napi_throw_range_error(
        env,
        "ERR_NAPI_INVALID_DATAVIEW_ARGS",
        "byte_offset + byte_length should be less than or "
        "equal to the size in bytes of the array passed in");
    return env->SetLastError(napi_pending_exception);
  }

  JsValueRef jsDataView;
  CHECK_JSRT(
      env,
      JsCreateDataView(
          jsArrayBuffer, static_cast<unsigned int>(byte_offset), static_cast<unsigned int>(byte_length), &jsDataView));

  auto dataViewInfo = new DataViewInfo{jsDataView, jsArrayBuffer, byte_offset, byte_length};
  CHECK_JSRT(env, JsCreateExternalObject(dataViewInfo, DataViewInfo::Finalize, reinterpret_cast<JsValueRef *>(result)));

  return napi_ok;
}

napi_status napi_is_dataview(napi_env env, napi_value value, bool *result) {
  CHECK_ENV_AND_ARG2(env, value, result);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType;
  CHECK_JSRT(env, JsGetValueType(jsValue, &valueType));

  *result = (valueType == JsDataView);
  return napi_ok;
}

napi_status napi_get_dataview_info(
    napi_env env,
    napi_value dataview,
    size_t *byte_length,
    void **data,
    napi_value *arraybuffer,
    size_t *byte_offset) {
  CHECK_ENV_AND_ARG(env, dataview);

  BYTE *bufferData = nullptr;
  unsigned int bufferLength = 0;

  JsValueRef jsExternalObject = reinterpret_cast<JsValueRef>(dataview);

  DataViewInfo *dataViewInfo;
  CHECK_JSRT(env, JsGetExternalData(jsExternalObject, reinterpret_cast<void **>(&dataViewInfo)));

  CHECK_JSRT(env, JsGetDataViewStorage(dataViewInfo->dataView, &bufferData, &bufferLength));

  if (byte_length != nullptr) {
    *byte_length = dataViewInfo->byteLength;
  }

  if (data != nullptr) {
    *data = static_cast<uint8_t *>(bufferData);
  }

  if (arraybuffer != nullptr) {
    *arraybuffer = reinterpret_cast<napi_value>(dataViewInfo->arrayBuffer);
  }

  if (byte_offset != nullptr) {
    *byte_offset = dataViewInfo->byteOffset;
  }

  return napi_ok;
}

//==============================================================================
// NAPI: version management
//==============================================================================
napi_status napi_get_version(napi_env env, uint32_t *result) {
  CHECK_ENV(env);
  CHECK_ARG(env, result);
  *result = NAPI_VERSION;
  return napi_ok;
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
  CHECK_ARG(env, adjusted_value);

  // TODO(jackhorton): Determine if Chakra needs or is able to do anything here
  // For now, we can lie and say that we always adjusted more memory
  *adjusted_value = change_in_bytes;

  return napi_ok;
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
  return chakra::FinalizingReference::New(
      env, js_object, /*shouldDeleteSelf:*/ result == nullptr, finalize_cb, native_object, finalize_hint, result);
}

#endif // NAPI_VERSION >= 5
#if 0
#if NAPI_VERSION >= 6

// BigInt
napi_status napi_create_bigint_int64(napi_env env, int64_t value, napi_value *result) {}
napi_status napi_create_bigint_uint64(napi_env env, uint64_t value, napi_value *result) {}
napi_status
napi_create_bigint_words(napi_env env, int sign_bit, size_t word_count, const uint64_t *words, napi_value *result) {}
napi_status napi_get_value_bigint_int64(napi_env env, napi_value value, int64_t *result, bool *lossless) {}
napi_status napi_get_value_bigint_uint64(napi_env env, napi_value value, uint64_t *result, bool *lossless) {}
napi_status
napi_get_value_bigint_words(napi_env env, napi_value value, int *sign_bit, size_t *word_count, uint64_t *words) {}

// Object
napi_status napi_get_all_property_names(
    napi_env env,
    napi_value object,
    napi_key_collection_mode key_mode,
    napi_key_filter key_filter,
    napi_key_conversion key_conversion,
    napi_value *result) {}

// Instance data
napi_status napi_set_instance_data(napi_env env, void *data, napi_finalize finalize_cb, void *finalize_hint) {}

napi_status napi_get_instance_data(napi_env env, void **data) {}
#endif // NAPI_VERSION >= 6

#if NAPI_VERSION >= 7
// ArrayBuffer detaching
napi_status napi_detach_arraybuffer(napi_env env, napi_value arraybuffer) {}

napi_status napi_is_detached_arraybuffer(napi_env env, napi_value value, bool *result) {}
#endif // NAPI_VERSION >= 7

#ifdef NAPI_EXPERIMENTAL
// Type tagging
napi_status napi_type_tag_object(napi_env env, napi_value value, const napi_type_tag *type_tag) {}

napi_status napi_check_object_type_tag(napi_env env, napi_value value, const napi_type_tag *type_tag, bool *result) {}
napi_status napi_object_freeze(napi_env env, napi_value object) {}
napi_status napi_object_seal(napi_env env, napi_value object) {}
#endif // NAPI_EXPERIMENTAL
#endif
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "NapiApi.h"

#include <jsi/jsi.h>

#include <array>
#include <mutex>
#include <sstream>

namespace react::jsi {

struct NapiJsiRuntimeArgs {};

// Implementation of N-API JSI Runtime
class NapiJsiRuntime : public facebook::jsi::Runtime, NapiApi, NapiApi::IExceptionThrower {
 public:
  NapiJsiRuntime(NapiJsiRuntimeArgs &&args) noexcept;
  ~NapiJsiRuntime() noexcept;

#pragma region Functions_inherited_from_Runtime

  facebook::jsi::Value evaluateJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      const std::string &sourceURL) override;

  std::shared_ptr<const facebook::jsi::PreparedJavaScript> prepareJavaScript(
      const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
      std::string sourceURL) override;

  facebook::jsi::Value evaluatePreparedJavaScript(
      const std::shared_ptr<const facebook::jsi::PreparedJavaScript> &js) override;

  facebook::jsi::Object global() override;

  std::string description() override;

  bool isInspectable() override;

  // We use the default instrumentation() implementation that returns an
  // Instrumentation instance which returns no metrics.

 private:
  // Despite the name "clone" suggesting a deep copy, a return value of these
  // functions points to a new heap allocated ChakraPointerValue whose member
  // JsRefHolder refers to the same JavaScript object as the member
  // JsRefHolder of *pointerValue. This behavior is consistent with that of
  // HermesRuntime and JSCRuntime. Also, Like all ChakraPointerValues, the
  // return value must only be used as an argument to the constructor of
  // jsi::Pointer or one of its derived classes.
  PointerValue *cloneSymbol(const PointerValue *pointerValue) override;
  PointerValue *cloneString(const PointerValue *pointerValue) override;
  PointerValue *cloneObject(const PointerValue *pointerValue) override;
  PointerValue *clonePropNameID(const PointerValue *pointerValue) override;

  facebook::jsi::PropNameID createPropNameIDFromAscii(const char *str, size_t length) override;
  facebook::jsi::PropNameID createPropNameIDFromUtf8(const uint8_t *utf8, size_t length) override;
  facebook::jsi::PropNameID createPropNameIDFromString(const facebook::jsi::String &str) override;
  std::string utf8(const facebook::jsi::PropNameID &id) override;
  bool compare(const facebook::jsi::PropNameID &lhs, const facebook::jsi::PropNameID &rhs) override;

  std::string symbolToString(const facebook::jsi::Symbol &s) override;

  // Despite its name, createPropNameIDFromAscii is the same function as
  // createStringFromUtf8.
  facebook::jsi::String createStringFromAscii(const char *str, size_t length) override;
  facebook::jsi::String createStringFromUtf8(const uint8_t *utf8, size_t length) override;
  std::string utf8(const facebook::jsi::String &str) override;

  facebook::jsi::Object createObject() override;
  facebook::jsi::Object createObject(std::shared_ptr<facebook::jsi::HostObject> ho) override;
  std::shared_ptr<facebook::jsi::HostObject> getHostObject(const facebook::jsi::Object &) override;
  facebook::jsi::HostFunctionType &getHostFunction(const facebook::jsi::Function &) override;

  facebook::jsi::Value getProperty(const facebook::jsi::Object &obj, const facebook::jsi::PropNameID &name) override;
  facebook::jsi::Value getProperty(const facebook::jsi::Object &obj, const facebook::jsi::String &name) override;
  bool hasProperty(const facebook::jsi::Object &obj, const facebook::jsi::PropNameID &name) override;
  bool hasProperty(const facebook::jsi::Object &obj, const facebook::jsi::String &name) override;
  void setPropertyValue(
      facebook::jsi::Object &obj,
      const facebook::jsi::PropNameID &name,
      const facebook::jsi::Value &value) override;
  void setPropertyValue(
      facebook::jsi::Object &obj,
      const facebook::jsi::String &name,
      const facebook::jsi::Value &value) override;

  bool isArray(const facebook::jsi::Object &obj) const override;
  bool isArrayBuffer(const facebook::jsi::Object &obj) const override;
  bool isFunction(const facebook::jsi::Object &obj) const override;
  bool isHostObject(const facebook::jsi::Object &obj) const override;
  bool isHostFunction(const facebook::jsi::Function &func) const override;
  // Returns the names of all enumerable properties of an object. This
  // corresponds the properties iterated through by the JavaScript for..in loop.
  facebook::jsi::Array getPropertyNames(const facebook::jsi::Object &obj) override;

  facebook::jsi::WeakObject createWeakObject(const facebook::jsi::Object &obj) override;
  facebook::jsi::Value lockWeakObject(facebook::jsi::WeakObject &weakObj) override;

  facebook::jsi::Array createArray(size_t length) override;
  size_t size(const facebook::jsi::Array &arr) override;
  size_t size(const facebook::jsi::ArrayBuffer &arrBuf) override;
  // The lifetime of the buffer returned is the same as the lifetime of the
  // ArrayBuffer. The returned buffer pointer does not count as a reference to
  // the ArrayBuffer for the purpose of garbage collection.
  uint8_t *data(const facebook::jsi::ArrayBuffer &arrBuf) override;
  facebook::jsi::Value getValueAtIndex(const facebook::jsi::Array &arr, size_t index) override;
  void setValueAtIndexImpl(facebook::jsi::Array &arr, size_t index, const facebook::jsi::Value &value) override;

  facebook::jsi::Function createFunctionFromHostFunction(
      const facebook::jsi::PropNameID &name,
      unsigned int paramCount,
      facebook::jsi::HostFunctionType func) override;
  facebook::jsi::Value call(
      const facebook::jsi::Function &func,
      const facebook::jsi::Value &jsThis,
      const facebook::jsi::Value *args,
      size_t count) override;
  facebook::jsi::Value
  callAsConstructor(const facebook::jsi::Function &func, const facebook::jsi::Value *args, size_t count) override;

  // For now, pushing a scope does nothing, and popping a scope forces the
  // JavaScript garbage collector to run.
  ScopeState *pushScope() override;
  void popScope(ScopeState *) override;

  bool strictEquals(const facebook::jsi::Symbol &a, const facebook::jsi::Symbol &b) const override;
  bool strictEquals(const facebook::jsi::String &a, const facebook::jsi::String &b) const override;
  bool strictEquals(const facebook::jsi::Object &a, const facebook::jsi::Object &b) const override;

  bool instanceOf(const facebook::jsi::Object &obj, const facebook::jsi::Function &func) override;

#pragma endregion Functions_inherited_from_Runtime

 protected:
  NapiJsiRuntimeArgs &runtimeArgs() {
    return m_args;
  }

 private: //  ChakraApi::IExceptionThrower members
  [[noreturn]] void ThrowJsExceptionOverride(napi_status errorCode, napi_value jsError) override;
  [[noreturn]] void ThrowNativeExceptionOverride(char const *errorMessage) override;
  void RewriteErrorMessage(napi_value jsError);

 private:
  // NapiPointerValueView is the base class for NapiPointerValue.
  // It holds the napi_ref, but unlike the NapiPointerValue does nothing in the
  // invalidate() method.
  // It is used by the JsiValueView, JsiValueViewArray, and JsiPropNameIDView classes
  // to keep temporary PointerValues on the call stack to avoid extra memory allocations.
  struct NapiPointerValueView : PointerValue {
    NapiPointerValueView(NapiApi *napi, napi_ref ref) noexcept : m_napi{napi}, m_ref{ref} {}

    NapiPointerValueView(NapiPointerValueView const &) = delete;
    NapiPointerValueView &operator=(NapiPointerValueView const &) = delete;

    // Intentionally do nothing in the invalidate() method.
    void invalidate() noexcept override {}

    napi_ref GetRef() const noexcept {
      return m_ref;
    }

    napi_value GetValue() const {
      return m_napi->GetReferenceValue(m_ref);
    }

    NapiApi* GetNapi() const noexcept {
      return m_napi;
    }

   private:
    // TODO: [vmoroz] How to make it safe? Is there a way to use weak pointers?
    NapiApi *m_napi;
    napi_ref m_ref;
  };

  // NapiPointerValue is needed for working with Facebook's jsi::Pointer class
  // and must only be used for this purpose. Every instance of
  // NapiPointerValue should be allocated on the heap and be used as an
  // argument to the constructor of jsi::Pointer or one of its derived classes.
  // Pointer makes sure that invalidate(), which frees the heap allocated
  // NapiPointerValue, is called upon destruction. Since the constructor of
  // jsi::Pointer is protected, we usually have to invoke it through
  // jsi::Runtime::make. The code should look something like:
  //
  //     make<Pointer>(new NapiPointerValue(...));
  //
  // or you can use the helper function MakePointer(), as defined below.
  struct NapiPointerValue final : NapiPointerValueView {
    NapiPointerValue(NapiApi *napi, napi_ref ref) : NapiPointerValueView{napi, ref} {
    }

    NapiPointerValue(NapiApi *napi, napi_value value) noexcept : NapiPointerValueView{napi, napi->CreateReference(value)}{}

    void invalidate() noexcept override {
      delete this;
    }

   private:
    // ~NapiPointerValue() should only be invoked by invalidate().
    // Hence we make it private.
    ~NapiPointerValue() noexcept override {
      if (napi_ref ref = GetRef()) {
        GetNapi()->DeleteReference(ref);
      }
    }
  };

  template <typename T, std::enable_if_t<std::is_base_of_v<facebook::jsi::Pointer, T>, int> = 0>
  T MakePointer(napi_ref ref) {
    return make<T>(new NapiPointerValue(m_env, ref));
  }

  template <typename T, std::enable_if_t<std::is_base_of_v<facebook::jsi::Pointer, T>, int> = 0>
  T MakePointer(napi_value value) {
    return make<T>(new NapiPointerValue(m_env, value));
  }

  // The pointer passed to this function must point to a NapiPointerValue.
  static NapiPointerValue *CloneNapiPointerValue(const PointerValue *pointerValue) {
    const NapiPointerValue *napiPointerValue = static_cast<const NapiPointerValue *>(pointerValue);
    return new NapiPointerValue(napiPointerValue->GetNapi(), napiPointerValue->GetValue());
  }

  // The jsi::Pointer passed to this function must hold a NapiPointerValue.
  static napi_ref GetJsRef(const facebook::jsi::Pointer &p) {
    return static_cast<const NapiPointerValueView *>(getPointerValue(p))->GetRef();
  }

  // The jsi::Pointer passed to this function must hold a NapiPointerValue.
  napi_value GetJsValue(const facebook::jsi::Pointer &p) {
    return GetReferenceValue(static_cast<const NapiPointerValueView *>(getPointerValue(p))->GetRef());
  }

  // These three functions only performs shallow copies.
  facebook::jsi::Value ToJsiValue(napi_ref ref);
  napi_ref ToNapiRef(const facebook::jsi::Value &value);

  napi_ref
  CreateExternalFunction(napi_value name, int32_t paramCount, napi_callback nativeFunction, void *callbackState);

  // Host function helper
  static napi_value HostFunctionCall(napi_env env, napi_callback_info info) noexcept;

  // Host object helpers; runtime must be referring to a ChakraRuntime.
  static napi_value HostObjectGetTrap(napi_env env, napi_callback_info info) noexcept;

  static napi_value HostObjectSetTrap(napi_env env, napi_callback_info info) noexcept;

  static napi_value HostObjectOwnKeysTrap(napi_env env, napi_callback_info info) noexcept;

  napi_value GetHostObjectProxyHandler();

  // Evaluate lambda and augment exception messages with the methodName.
  template <typename TLambda>
  static auto RunInMethodContext(char const *methodName, TLambda lambda) {
    try {
      return lambda();
    } catch (facebook::jsi::JSError const &) {
      throw; // do not augment the JSError exceptions.
    } catch (std::exception const &ex) {
      ChakraThrow((std::string{"Exception in "} + methodName + ": " + ex.what()).c_str());
    } catch (...) {
      ChakraThrow((std::string{"Exception in "} + methodName + ": <unknown>").c_str());
    }
  }

  // Evaluate lambda and convert all exceptions to Chakra engine exceptions.
  template <typename TLambda>
  napi_value HandleCallbackExceptions(TLambda lambda) noexcept {
    try {
      try {
        return lambda();
      } catch (facebook::jsi::JSError const &jsError) {
        // This block may throw exceptions
        SetException(ToNapiRef(jsError.value()));
      }
    } catch (std::exception const &ex) {
      SetException(ex.what());
    } catch (...) {
      SetException(L"Unexpected error");
    }

    return m_undefinedValue;
  }

  // Promise Helpers
  // static void CALLBACK PromiseContinuationCallback(JsValueRef funcRef, void *callbackState) noexcept;
  // static void CALLBACK
  // PromiseRejectionTrackerCallback(JsValueRef promise, JsValueRef reason, bool handled, void *callbackState);

  // void PromiseContinuation(JsValueRef value) noexcept;
  // void PromiseRejectionTracker(JsValueRef promise, JsValueRef reason, bool handled);

  // void setupNativePromiseContinuation() noexcept;

  // Memory tracker helpers
  // void setupMemoryTracker() noexcept;

  // In-proc debugging helpers
  // void startDebuggingIfNeeded();
  // void stopDebuggingIfNeeded();

  // JsErrorCode enableDebugging(
  //    JsRuntimeHandle runtime,
  //    std::string const &runtimeName,
  //    bool breakOnNextLine,
  //    uint16_t port,
  //    std::unique_ptr<DebugProtocolHandler> &debugProtocolHandler,
  //    std::unique_ptr<DebugService> &debugService);
  // void ProcessDebuggerCommandQueue();

  // static void CALLBACK ProcessDebuggerCommandQueueCallback(void *callbackState);

  // Version related helpers
  // static void initRuntimeVersion() noexcept;
  // static uint64_t getRuntimeVersion() {
  //  return s_runtimeVersion;
  //}

  // Miscellaneous
  // std::unique_ptr<const facebook::jsi::Buffer> generatePreparedScript(
  //    const std::string &sourceURL,
  //    const facebook::jsi::Buffer &sourceBuffer) noexcept;
  // facebook::jsi::Value evaluateJavaScriptSimple(const facebook::jsi::Buffer &buffer, const std::string &sourceURL);
  // bool evaluateSerializedScript(
  //    const facebook::jsi::Buffer &scriptBuffer,
  //    const facebook::jsi::Buffer &serializedScriptBuffer,
  //    const std::string &sourceURL,
  //    JsValueRef *result);

  enum class PropertyAttibutes {
    None = 0,
    ReadOnly = 1 << 1,
    DontEnum = 1 << 2,
    DontDelete = 1 << 3,
    Frozen = ReadOnly | DontDelete,
    DontEnumAndFrozen = DontEnum | Frozen,
  };

  friend constexpr PropertyAttibutes operator&(PropertyAttibutes left, PropertyAttibutes right) {
    return (PropertyAttibutes)((int)left & (int)right);
  }

  friend constexpr bool operator!(PropertyAttibutes attrs) {
    return attrs == PropertyAttibutes::None;
  }

  napi_value CreatePropertyDescriptor(napi_value value, PropertyAttibutes attrs);

  // The number of arguments that we keep on stack.
  // We use heap if we have more argument.
  constexpr static size_t MaxStackArgCount = 8;

  // NapiValueArgs helps to optimize passing arguments to NAPI function.
  // If number of arguments is below or equal to MaxStackArgCount,
  // then they are kept on call stack, otherwise arguments are allocated on heap.
  struct NapiValueArgs final {
    NapiValueArgs(NapiJsiRuntime &rt, facebook::jsi::Value const &firstArg, Span<facebook::jsi::Value const> args);
    operator NapiApi::Span<napi_value>();

   private:
    size_t const m_count{};
    std::array<napi_value, MaxStackArgCount> m_stackArgs{{nullptr}};
    std::unique_ptr<napi_value[]> const m_heapArgs;
  };

  // This type represents a view to Value based on JsValueRef.
  // It avoids extra memory allocation by using an in-place storage.
  // It uses ChakraPointerValueView that does nothing in the invalidate() method.
  struct JsiValueView final {
    JsiValueView(napi_value jsValue);
    ~JsiValueView() noexcept;
    operator facebook::jsi::Value const &() const noexcept;

    using StoreType = std::aligned_storage_t<sizeof(NapiPointerValueView)>;
    static facebook::jsi::Value InitValue(napi_value jsValue, StoreType *store);

   private:
    StoreType m_pointerStore{};
    facebook::jsi::Value const m_value{};
  };

  // This class helps to use stack storage for passing arguments that must be temporary converted from
  // JsValueRef to facebook::jsi::Value.
  struct JsiValueViewArgs final {
    JsiValueViewArgs(napi_value *args, size_t argCount) noexcept;
    facebook::jsi::Value const *Data() const noexcept;
    size_t Size() const noexcept;

   private:
    size_t const m_size{};
    std::array<JsiValueView::StoreType, MaxStackArgCount> m_stackPointerStore{};
    std::array<facebook::jsi::Value, MaxStackArgCount> m_stackArgs{};
    std::unique_ptr<JsiValueView::StoreType[]> const m_heapPointerStore{};
    std::unique_ptr<facebook::jsi::Value[]> const m_heapArgs{};
  };

  // PropNameIDView helps to use the stack storage for temporary conversion from
  // JsPropertyIdRef to facebook::jsi::PropNameID.
  struct PropNameIDView final {
    PropNameIDView(napi_value propertyId) noexcept;
    ~PropNameIDView() noexcept;
    operator facebook::jsi::PropNameID const &() const noexcept;

    using StoreType = std::aligned_storage_t<sizeof(NapiPointerValueView)>;

   private:
    StoreType m_pointerStore{};
    facebook::jsi::PropNameID const m_propertyId;
  };

 private:
  // Property ID cache to improve execution speed
  struct PropertyId final {
    NapiRefHolder Object;
    NapiRefHolder Proxy;
    NapiRefHolder Symbol;
    NapiRefHolder byteLength;
    NapiRefHolder configurable;
    NapiRefHolder enumerable;
    NapiRefHolder get;
    NapiRefHolder hostFunctionSymbol;
    NapiRefHolder hostObjectSymbol;
    NapiRefHolder length;
    NapiRefHolder message;
    NapiRefHolder ownKeys;
    NapiRefHolder propertyIsEnumerable;
    NapiRefHolder prototype;
    NapiRefHolder set;
    NapiRefHolder toString;
    NapiRefHolder value;
    NapiRefHolder writable;
  } m_propertyId;

  NapiRefHolder m_undefinedValue;
  NapiRefHolder m_proxyConstructor;
  NapiRefHolder m_hostObjectProxyHandler;

  static std::once_flag s_runtimeVersionInitFlag;
  static uint64_t s_runtimeVersion;

  // Arguments shared by the specializations
  NapiJsiRuntimeArgs m_args;

  napi_env m_env;
  //JsRuntimeHandle m_runtime;
  //JsRefHolder m_context;
  //JsRefHolder m_prevContext;

  // Set the Chakra API exception thrower on this thread
  ExceptionThrowerHolder m_exceptionThrower{this};

  // Note: For simplicity, We are pinning the script and serialized script
  // buffers in the facebook::jsi::Runtime instance assuming as these buffers
  // are needed to stay alive for the lifetime of the facebook::jsi::Runtime
  // implementation. This approach doesn't make sense for other external buffers
  // which may get created during the execution as that will stop the backing
  // buffer from getting released when the JSValue gets collected.

  //// These buffers are kept to serve the source callbacks when evaluating
  //// serialized scripts.
  //std::vector<std::shared_ptr<const facebook::jsi::Buffer>> m_pinnedScripts;

  //// These buffers back the external array buffers that we handover to
  //// ChakraCore.
  //std::vector<std::shared_ptr<const facebook::jsi::Buffer>> m_pinnedPreparedScripts;

  //std::string m_debugRuntimeName;
  //int m_debugPort{0};
  //std::unique_ptr<DebugProtocolHandler> m_debugProtocolHandler;
  //std::unique_ptr<DebugService> m_debugService;
  //constexpr static char DebuggerDefaultRuntimeName[] = "runtime1";
  //constexpr static int DebuggerDefaultPort = 9229;
};

} // namespace react::jsi
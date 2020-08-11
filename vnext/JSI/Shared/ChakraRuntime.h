// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ChakraObjectRef.h"
#include "ChakraRuntimeArgs.h"

#include <jsi/jsi.h>

#ifdef CHAKRACORE
#include "ChakraCore.h"
#include "ChakraCoreDebugger.h"
#else
#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT
#endif
#include "jsrt.h"
#endif

#include <array>
#include <memory>
#include <mutex>
#include <sstream>

#if !defined(CHAKRACORE)
class DebugProtocolHandler {};
class DebugService {};
#endif

namespace Microsoft::JSI {

class ChakraRuntime : public facebook::jsi::Runtime {
 public:
  ChakraRuntime(ChakraRuntimeArgs &&args) noexcept;
  ~ChakraRuntime() noexcept;

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
  // functions points to a new heap allocated ChakraPointerValue whose memeber
  // ChakraObjectRef refers to the same JavaScript object as the member
  // ChakraObjectRef of *pointerValue. This behavior is consistent with that of
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
  facebook::jsi::Value lockWeakObject(const facebook::jsi::WeakObject &weakObj) override;

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
  ChakraRuntimeArgs &runtimeArgs() {
    return m_args;
  }

 private:
  void VerifyJsErrorElseThrow(JsErrorCode error);

  // ChakraPointerValue is needed for working with Facebook's jsi::Pointer class
  // and must only be used for this purpose. Every instance of
  // ChakraPointerValue should be allocated on the heap and be used as an
  // argument to the constructor of jsi::Pointer or one of its derived classes.
  // Pointer makes sure that invalidate(), which frees the heap allocated
  // ChakraPointerValue, is called upon destruction. Since the constructor of
  // jsi::Pointer is protected, we usually have to invoke it through
  // jsi::Runtime::make. The code should look something like:
  //
  //     make<Pointer>(new ChakraPointerValue(...));
  //
  // or you can use the helper function MakePointer(), as defined below.

  struct ChakraPointerValueView : PointerValue {
    ChakraPointerValueView(JsRef ref) noexcept : m_ref{ref} {}

    void invalidate() noexcept override {}

    JsRef GetRef() const noexcept {
      return m_ref;
    }

   private:
    JsRef m_ref;
  };

  struct ChakraPointerValue final : ChakraPointerValueView {
    ChakraPointerValue(JsRef ref) noexcept : ChakraPointerValueView{ref} {
      if (ref) {
        VerifyChakraErrorElseThrow(JsAddRef(ref, nullptr));
      }
    }

    void invalidate() noexcept override {
      delete this;
    }

   private:
    // ~ChakraPointerValue() should only be invoked by invalidate().
    // Hence we make it private.
    ~ChakraPointerValue() noexcept override {
      if (JsRef ref = GetRef()) {
        VerifyChakraErrorElseThrow(JsRelease(ref, nullptr));
      }
    }
  };

  template <typename T, std::enable_if_t<std::is_base_of_v<facebook::jsi::Pointer, T>, int> = 0>
  inline T MakePointer(JsRef ref) {
    return make<T>(new ChakraPointerValue(std::move(ref)));
  }

  // The pointer passed to this function must point to a ChakraPointerValue.
  static ChakraPointerValue *CloneChakraPointerValue(const PointerValue *pointerValue) {
    return new ChakraPointerValue(*(static_cast<const ChakraPointerValue *>(pointerValue)));
  }

  // The jsi::Pointer passed to this function must hold a ChakraPointerValue.
  static JsRef GetChakraObjectRef(const facebook::jsi::Pointer &p) {
    return static_cast<const ChakraPointerValueView *>(getPointerValue(p))->GetRef();
  }

  // These three functions only performs shallow copies.
  facebook::jsi::Value ToJsiValue(JsValueRef ref);
  JsValueRef ToChakraObjectRef(const facebook::jsi::Value &value);

  // Convenience functions for property access.
  JsValueRef GetProperty(JsValueRef obj, JsPropertyIdRef id);

  JsValueRef GetProperty(JsValueRef obj, const char *const name) {
    return GetProperty(obj, GetChakraObjectRef(createPropNameIDFromAscii(name, strlen(name))));
  }

  // Since the function
  //   Object::getProperty(Runtime& runtime, const char* name)
  // causes multiple copies of name, we do not want to use it when implementing
  // ChakraRuntime methods. This function does the same thing as
  // Object::getProperty, but without the extra overhead. This function is
  // declared as const so that it can be used when implementing
  // isHostFunction and isHostObject.
  inline facebook::jsi::Value GetProperty(const facebook::jsi::Object &obj, const char *const name) const {
    // We have to use const_casts here because ToJsiValue and GetProperty cannot
    // be marked as const.
    return const_cast<ChakraRuntime *>(this)->ToJsiValue(
        const_cast<ChakraRuntime *>(this)->GetProperty(GetChakraObjectRef(obj), name));
  }

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

  JsValueRef CallFunction(JsValueRef function, JsValueRefSpan args);
  JsValueRef ConstructObject(JsValueRef function, JsValueRefSpan args);

  JsValueRef CreateExternalFunction(
      JsPropertyIdRef name,
      int32_t paramCount,
      JsNativeFunction nativeFunction,
      void *callbackState);

  // Host function helper
  static JsValueRef CALLBACK HostFunctionCall(
      JsValueRef callee,
      bool isConstructCall,
      JsValueRef *args,
      unsigned short argCount,
      void *callbackState);

  // Host object helpers; runtime must be referring to a ChakraRuntime.
  static JsValueRef CALLBACK HostObjectGetTrap(
      JsValueRef callee,
      bool isConstructCall,
      JsValueRef *args,
      unsigned short argCount,
      void *callbackState) noexcept;
  static JsValueRef CALLBACK HostObjectSetTrap(
      JsValueRef callee,
      bool isConstructCall,
      JsValueRef *args,
      unsigned short argCount,
      void *callbackState) noexcept;
  static JsValueRef CALLBACK HostObjectOwnKeysTrap(
      JsValueRef callee,
      bool isConstructCall,
      JsValueRef *args,
      unsigned short argCount,
      void *callbackState) noexcept;
  JsValueRef GetHostObjectProxyHandler();

  // Evaluate code and catch any exceptions
  template <typename TCode>
  JsValueRef HandleCallbackExceptions(TCode &&code) noexcept {
    try {
      try {
        return code();
      } catch (facebook::jsi::JSError const &jsError) {
        // This block may throw exceptions
        SetException(ToChakraObjectRef(jsError.value()));
      }
    } catch (std::exception const &ex) {
      SetException(ex.what());
    } catch (...) {
      SetException(L"Unexpected error");
    }

    return m_undefinedValue;
  }

  // Promise Helpers
  static void CALLBACK PromiseContinuationCallback(JsValueRef funcRef, void *callbackState) noexcept;
  static void CALLBACK
  PromiseRejectionTrackerCallback(JsValueRef promise, JsValueRef reason, bool handled, void *callbackState);

  void PromiseContinuation(JsValueRef value) noexcept;
  void PromiseRejectionTracker(JsValueRef promise, JsValueRef reason, bool handled);

  void setupNativePromiseContinuation() noexcept;

  // Memory tracker helpers
  void setupMemoryTracker() noexcept;

  // In-proc debugging helpers
  void startDebuggingIfNeeded();
  void stopDebuggingIfNeeded();

  JsErrorCode enableDebugging(
      JsRuntimeHandle runtime,
      std::string const &runtimeName,
      bool breakOnNextLine,
      uint16_t port,
      std::unique_ptr<DebugProtocolHandler> &debugProtocolHandler,
      std::unique_ptr<DebugService> &debugService);
  void ProcessDebuggerCommandQueue();

  static void CALLBACK ProcessDebuggerCommandQueueCallback(void *callbackState);

  // Version related helpers
  static void initRuntimeVersion() noexcept;
  static uint64_t getRuntimeVersion() {
    return s_runtimeVersion;
  }

  // Miscellaneous
  std::unique_ptr<const facebook::jsi::Buffer> generatePreparedScript(
      const std::string &sourceURL,
      const facebook::jsi::Buffer &sourceBuffer) noexcept;
  facebook::jsi::Value evaluateJavaScriptSimple(const facebook::jsi::Buffer &buffer, const std::string &sourceURL);
  bool evaluateSerializedScript(
      const facebook::jsi::Buffer &scriptBuffer,
      const facebook::jsi::Buffer &serializedScriptBuffer,
      const std::string &sourceURL);

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

  JsValueRef CreatePropertyDescriptor(JsValueRef value, PropertyAttibutes attrs);
  void SetProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef value);

  // This type represents a view to Value based on JsValueRef.
  // It avoids extra memory allocation by using an in-place storage.
  // It uses ChakraPointerValueView that does nothing in the invalidate() method.
  struct JsiValueView {
    JsiValueView(JsValueRef jsValue) noexcept;
    ~JsiValueView() noexcept;
    operator facebook::jsi::Value const &() const noexcept;

    using StoreType = std::aligned_storage_t<sizeof(ChakraPointerValueView)>;
    static facebook::jsi::Value InitValue(JsValueRef jsValue, StoreType *store) noexcept;

   private:
    StoreType m_pointerStore{};
    facebook::jsi::Value m_value{};
  };

  constexpr static size_t MaxCallArgCount = 32;

  // This class helps to use stack storage for passing arguments that must be temporary converted from
  // JsValueRef to facebook::jsi::Value.
  struct JsiValueViewArray {
    JsiValueViewArray(JsValueRef *args, size_t argCount) noexcept;
    facebook::jsi::Value const *Data() const noexcept;
    size_t Size() const noexcept;

   private:
    std::array<JsiValueView::StoreType, MaxCallArgCount> m_pointerStoreArray{};
    std::array<facebook::jsi::Value, MaxCallArgCount> m_valueArray{};
    size_t m_size{};
  };

  // PropNameIDView helps to use the stack storage for temporary conversion from
  // JsPropertyIdRef to facebook::jsi::PropNameID.
  struct PropNameIDView {
    PropNameIDView(JsPropertyIdRef propertyId) noexcept;
    ~PropNameIDView() noexcept;
    operator facebook::jsi::PropNameID const &() const noexcept;

    using StoreType = std::aligned_storage_t<sizeof(ChakraPointerValueView)>;

   private:
    StoreType m_pointerStore{};
    facebook::jsi::PropNameID m_propertyId;
  };

 private:
  // Property ID cache to improve execution speed
  struct PropertyId {
    ChakraObjectRef Object;
    ChakraObjectRef Proxy;
    ChakraObjectRef Symbol;
    ChakraObjectRef byteLength;
    ChakraObjectRef configurable;
    ChakraObjectRef enumerable;
    ChakraObjectRef get;
    ChakraObjectRef hostFunctionSymbol;
    ChakraObjectRef hostObjectSymbol;
    ChakraObjectRef length;
    ChakraObjectRef ownKeys;
    ChakraObjectRef propertyIsEnumerable;
    ChakraObjectRef prototype;
    ChakraObjectRef set;
    ChakraObjectRef toString;
    ChakraObjectRef value;
    ChakraObjectRef writable;
  } m_propertyId;

  ChakraObjectRef m_undefinedValue;
  ChakraObjectRef m_hostObjectProxyHandler;

  static std::once_flag s_runtimeVersionInitFlag;
  static uint64_t s_runtimeVersion;

  // Arguments shared by the specializations
  ChakraRuntimeArgs m_args;

  JsRuntimeHandle m_runtime;
  ChakraObjectRef m_context;

  // Note: For simplicity, We are pinning the script and serialized script
  // buffers in the facebook::jsi::Runtime instance assuming as these buffers
  // are needed to stay alive for the lifetime of the facebook::jsi::Runtime
  // implementation. This approach doesn't make sense for other external buffers
  // which may get created during the execution as that will stop the backing
  // buffer from getting released when the JSValue gets collected.

  // These buffers are kept to serve the source callbacks when evaluating
  // serialized scripts.
  std::vector<std::shared_ptr<const facebook::jsi::Buffer>> m_pinnedScripts;

  // These buffers back the external array buffers that we handover to
  // ChakraCore.
  std::vector<std::shared_ptr<const facebook::jsi::Buffer>> m_pinnedPreparedScripts;

  std::string m_debugRuntimeName;
  int m_debugPort{0};
  std::unique_ptr<DebugProtocolHandler> m_debugProtocolHandler;
  std::unique_ptr<DebugService> m_debugService;
  constexpr static char DebuggerDefaultRuntimeName[] = "runtime1";
  constexpr static int DebuggerDefaultPort = 9229;
};

} // namespace Microsoft::JSI

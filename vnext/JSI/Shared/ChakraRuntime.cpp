// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ChakraRuntime.h"

#include "Unicode.h"
#include "Utilities.h"

#include <MemoryTracker.h>
#include <cxxreact/MessageQueueThread.h>

#include <cstring>
#include <limits>
#include <mutex>
#include <set>
#include <sstream>
#include <unordered_set>

#define ChakraVerifyElseThrow(cond, message)            \
  do {                                                  \
    if (!(cond)) {                                      \
      throw facebook::jsi::JSINativeException(message); \
    }                                                   \
  } while (false)

namespace Microsoft::JSI {

namespace {

struct HostFunctionWrapper {
  HostFunctionWrapper(facebook::jsi::HostFunctionType &&hostFunction, ChakraRuntime &runtime)
      : m_hostFunction(std::move(hostFunction)), m_runtime(runtime) {}

  facebook::jsi::HostFunctionType &GetHostFunction() {
    return m_hostFunction;
  }

  ChakraRuntime &GetRuntime() {
    return m_runtime;
  }

 private:
  facebook::jsi::HostFunctionType m_hostFunction;
  ChakraRuntime &m_runtime;
};

} // namespace

ChakraRuntime::ChakraRuntime(ChakraRuntimeArgs &&args) noexcept : m_args{std::move(args)} {
  JsRuntimeAttributes runtimeAttributes = JsRuntimeAttributeNone;

  if (!m_args.enableJITCompilation) {
    runtimeAttributes = static_cast<JsRuntimeAttributes>(
        runtimeAttributes | JsRuntimeAttributeDisableNativeCodeGeneration |
        JsRuntimeAttributeDisableExecutablePageAllocation);
  }

  VerifyChakraErrorElseThrow(JsCreateRuntime(runtimeAttributes, nullptr, &m_runtime));

  setupMemoryTracker();

  m_context = ChakraObjectRef(CreateContext(m_runtime));

  // Note :: We currently assume that the runtime will be created and
  // exclusively used in a single thread.
  VerifyChakraErrorElseThrow(JsSetCurrentContext(m_context));

  startDebuggingIfNeeded();

  setupNativePromiseContinuation();

  std::call_once(s_runtimeVersionInitFlag, initRuntimeVersion);

  m_propertyId.Object = ChakraObjectRef(GetPropertyId(L"Object"));
  m_propertyId.Proxy = ChakraObjectRef(GetPropertyId(L"Proxy"));
  m_propertyId.Symbol = ChakraObjectRef(GetPropertyId(L"Symbol"));
  m_propertyId.byteLength = ChakraObjectRef(GetPropertyId(L"byteLength"));
  m_propertyId.configurable = ChakraObjectRef(GetPropertyId(L"configurable"));
  m_propertyId.enumerable = ChakraObjectRef(GetPropertyId(L"enumerable"));
  m_propertyId.get = ChakraObjectRef(GetPropertyId(L"get"));
  m_propertyId.hostFunctionSymbol = ChakraObjectRef(GetSymbolPropertyId(L"hostFunctionSymbol"));
  m_propertyId.hostObjectSymbol = ChakraObjectRef(GetSymbolPropertyId(L"hostObjectSymbol"));
  m_propertyId.length = ChakraObjectRef(GetPropertyId(L"length"));
  m_propertyId.ownKeys = ChakraObjectRef(GetPropertyId(L"ownKeys"));
  m_propertyId.propertyIsEnumerable = ChakraObjectRef(GetPropertyId(L"propertyIsEnumerable"));
  m_propertyId.prototype = ChakraObjectRef(GetPropertyId(L"prototype"));
  m_propertyId.set = ChakraObjectRef(GetPropertyId(L"set"));
  m_propertyId.toString = ChakraObjectRef(GetPropertyId(L"toString"));
  m_propertyId.value = ChakraObjectRef(GetPropertyId(L"value"));
  m_propertyId.writable = ChakraObjectRef(GetPropertyId(L"writable"));

  m_undefinedValue = ChakraObjectRef{GetUndefinedValue()};
}

ChakraRuntime::~ChakraRuntime() noexcept {
  m_undefinedValue = {};
  m_propertyId = {};
  m_hostObjectProxyHandler = {};

  stopDebuggingIfNeeded();

  VerifyChakraErrorElseThrow(JsSetCurrentContext(JS_INVALID_REFERENCE));
  m_context = {};

  JsSetRuntimeMemoryAllocationCallback(m_runtime, nullptr, nullptr);

  VerifyChakraErrorElseThrow(JsDisposeRuntime(m_runtime));
}

JsValueRef ChakraRuntime::CreatePropertyDescriptor(JsValueRef value, PropertyAttibutes attrs) {
  JsValueRef descriptor = CreateObject();
  SetProperty(descriptor, m_propertyId.value, value);
  if (!(attrs & PropertyAttibutes::ReadOnly)) {
    SetProperty(descriptor, m_propertyId.writable, BoolToBoolean(true));
  }
  if (!(attrs & PropertyAttibutes::DontEnum)) {
    SetProperty(descriptor, m_propertyId.enumerable, BoolToBoolean(true));
  }
  if (!(attrs & PropertyAttibutes::DontDelete)) {
    SetProperty(descriptor, m_propertyId.configurable, BoolToBoolean(true));
  }
  return descriptor;
}

void ChakraRuntime::SetProperty(JsValueRef object, JsPropertyIdRef propertyId, JsValueRef value) {
  VerifyJsErrorElseThrow(JsSetProperty(object, propertyId, value, true));
}

#pragma region Functions_inherited_from_Runtime

facebook::jsi::Value ChakraRuntime::evaluateJavaScript(
    const std::shared_ptr<const facebook::jsi::Buffer> &buffer,
    const std::string &sourceURL) {
  // Simple evaluate if scriptStore not available as it's risky to utilize the
  // byte codes without checking the script version.
  if (!runtimeArgs().scriptStore) {
    if (!buffer)
      throw facebook::jsi::JSINativeException("Script buffer is empty!");
    return evaluateJavaScriptSimple(*buffer, sourceURL);
  }

  uint64_t scriptVersion = 0;
  std::shared_ptr<const facebook::jsi::Buffer> scriptBuffer;

  if (buffer) {
    scriptBuffer = std::move(buffer);
    scriptVersion = runtimeArgs().scriptStore->getScriptVersion(sourceURL);
  } else {
    auto versionedScript = runtimeArgs().scriptStore->getVersionedScript(sourceURL);
    scriptBuffer = std::move(versionedScript.buffer);
    scriptVersion = versionedScript.version;
  }

  if (!scriptBuffer) {
    throw facebook::jsi::JSINativeException("Script buffer is empty!");
  }

  // Simple evaluate if script version can't be computed.
  if (scriptVersion == 0) {
    return evaluateJavaScriptSimple(*scriptBuffer, sourceURL);
  }

  auto sharedScriptBuffer = std::shared_ptr<const facebook::jsi::Buffer>(std::move(scriptBuffer));

  facebook::jsi::ScriptSignature scriptSignature = {sourceURL, scriptVersion};
  facebook::jsi::JSRuntimeSignature runtimeSignature = {description().c_str(), getRuntimeVersion()};

  auto preparedScript =
      runtimeArgs().preparedScriptStore->tryGetPreparedScript(scriptSignature, runtimeSignature, nullptr);

  std::shared_ptr<const facebook::jsi::Buffer> sharedPreparedScript;
  if (preparedScript) {
    sharedPreparedScript = std::shared_ptr<const facebook::jsi::Buffer>(std::move(preparedScript));
  } else {
    auto genPreparedScript = generatePreparedScript(sourceURL, *sharedScriptBuffer);
    if (!genPreparedScript)
      std::terminate(); // Cache generation can't fail unless something really
                        // wrong. but we should get rid of this abort before
                        // shipping.

    sharedPreparedScript = std::shared_ptr<const facebook::jsi::Buffer>(std::move(genPreparedScript));
    runtimeArgs().preparedScriptStore->persistPreparedScript(
        sharedPreparedScript, scriptSignature, runtimeSignature, nullptr);
  }

  // We are pinning the buffers which are backing the external array buffers to
  // the duration of this. This is not good if the external array buffers have a
  // reduced lifetime compared to the runtime itself. But, it's ok for the script
  // and prepared script buffer as their lifetime is expected to be same as the
  // JSI runtime.
  m_pinnedPreparedScripts.push_back(sharedPreparedScript);
  m_pinnedScripts.push_back(sharedScriptBuffer);

  if (evaluateSerializedScript(*sharedScriptBuffer, *sharedPreparedScript, sourceURL)) {
    return facebook::jsi::Value::undefined();
  }

  // If we reach here, fall back to simple evaluation.
  return evaluateJavaScriptSimple(*sharedScriptBuffer, sourceURL);
}

std::shared_ptr<const facebook::jsi::PreparedJavaScript> ChakraRuntime::prepareJavaScript(
    const std::shared_ptr<const facebook::jsi::Buffer> &,
    std::string) {
  throw facebook::jsi::JSINativeException("Not implemented!");
}

facebook::jsi::Value ChakraRuntime::evaluatePreparedJavaScript(
    const std::shared_ptr<const facebook::jsi::PreparedJavaScript> &) {
  throw facebook::jsi::JSINativeException("Not implemented!");
}

facebook::jsi::Object ChakraRuntime::global() {
  return MakePointer<facebook::jsi::Object>(GetGlobalObject());
}

std::string ChakraRuntime::description() {
  return "ChakraRuntime";
}

bool ChakraRuntime::isInspectable() {
  return false;
}

facebook::jsi::Runtime::PointerValue *ChakraRuntime::cloneSymbol(
    const facebook::jsi::Runtime::PointerValue *pointerValue) {
  return CloneChakraPointerValue(pointerValue);
}

facebook::jsi::Runtime::PointerValue *ChakraRuntime::cloneString(
    const facebook::jsi::Runtime::PointerValue *pointerValue) {
  return CloneChakraPointerValue(pointerValue);
}

facebook::jsi::Runtime::PointerValue *ChakraRuntime::cloneObject(
    const facebook::jsi::Runtime::PointerValue *pointerValue) {
  return CloneChakraPointerValue(pointerValue);
}

facebook::jsi::Runtime::PointerValue *ChakraRuntime::clonePropNameID(
    const facebook::jsi::Runtime::PointerValue *pointerValue) {
  return CloneChakraPointerValue(pointerValue);
}

facebook::jsi::PropNameID ChakraRuntime::createPropNameIDFromAscii(const char *str, size_t length) {
  JsPropertyIdRef propertyId = GetPropertyId(std::string_view{str, length});
  return MakePointer<facebook::jsi::PropNameID>(propertyId);
}

facebook::jsi::PropNameID ChakraRuntime::createPropNameIDFromUtf8(const uint8_t *utf8, size_t length) {
  JsPropertyIdRef propertyId = GetPropertyId(std::string_view{reinterpret_cast<const char *>(utf8), length});
  return MakePointer<facebook::jsi::PropNameID>(propertyId);
}

facebook::jsi::PropNameID ChakraRuntime::createPropNameIDFromString(const facebook::jsi::String &str) {
  JsPropertyIdRef propertyId = GetPropertyId(StringToPointer(GetChakraObjectRef(str)));
  return MakePointer<facebook::jsi::PropNameID>(propertyId);
}

std::string ChakraRuntime::utf8(const facebook::jsi::PropNameID &id) {
  return Common::Unicode::Utf16ToUtf8(GetPropertyNameFromId(GetChakraObjectRef(id)));
}

bool ChakraRuntime::compare(const facebook::jsi::PropNameID &lhs, const facebook::jsi::PropNameID &rhs) {
  return GetChakraObjectRef(lhs) == GetChakraObjectRef(rhs);
}

std::string ChakraRuntime::symbolToString(const facebook::jsi::Symbol &s) {
  JsValueRef symbol = GetChakraObjectRef(s);
  JsValueRef symbolCtor = GetProperty(GetGlobalObject(), m_propertyId.Symbol);
  JsValueRef symbolPrototype = GetProperty(symbolCtor, m_propertyId.prototype);
  JsValueRef symbolToString = GetProperty(symbolPrototype, m_propertyId.toString);
  JsValueRef jsString = CallFunction(symbolToString, {symbol});
  return ToStdString(jsString);
}

facebook::jsi::String ChakraRuntime::createStringFromAscii(const char *str, size_t length) {
  return MakePointer<facebook::jsi::String>(PointerToString({str, length}));
}

facebook::jsi::String ChakraRuntime::createStringFromUtf8(const uint8_t *str, size_t length) {
  return MakePointer<facebook::jsi::String>(PointerToString({reinterpret_cast<char const *>(str), length}));
}

std::string ChakraRuntime::utf8(const facebook::jsi::String &str) {
  return ToStdString(GetChakraObjectRef(str));
}

facebook::jsi::Object ChakraRuntime::createObject() {
  return MakePointer<facebook::jsi::Object>(CreateObject());
}

facebook::jsi::Object ChakraRuntime::createObject(std::shared_ptr<facebook::jsi::HostObject> hostObject) {
  JsValueRef proxyCtor = GetProperty(GetGlobalObject(), m_propertyId.Proxy);
  JsValueRef proxy =
      ConstructObject(proxyCtor, {m_undefinedValue, ToJsObject(hostObject), GetHostObjectProxyHandler()});
  return MakePointer<facebook::jsi::Object>(proxy);
}

std::shared_ptr<facebook::jsi::HostObject> ChakraRuntime::getHostObject(const facebook::jsi::Object &obj) {
  JsValueRef hostObject = GetProperty(GetChakraObjectRef(obj), m_propertyId.hostObjectSymbol);
  if (GetValueType(hostObject) == JsValueType::JsObject) {
    return *static_cast<std::shared_ptr<facebook::jsi::HostObject> *>(GetExternalData(hostObject));
  } else {
    throw facebook::jsi::JSINativeException("getHostObject() can only be called with HostObjects.");
  }
}

facebook::jsi::HostFunctionType &ChakraRuntime::getHostFunction(const facebook::jsi::Function &func) {
  JsValueRef hostFunction = GetProperty(GetChakraObjectRef(func), m_propertyId.hostFunctionSymbol);
  if (GetValueType(hostFunction) == JsValueType::JsObject) {
    return static_cast<HostFunctionWrapper *>(GetExternalData(hostFunction))->GetHostFunction();
  } else {
    throw facebook::jsi::JSINativeException("getHostFunction() can only be called with HostFunction.");
  }
}

facebook::jsi::Value ChakraRuntime::getProperty(
    const facebook::jsi::Object &obj,
    const facebook::jsi::PropNameID &name) {
  return ToJsiValue(GetProperty(GetChakraObjectRef(obj), GetChakraObjectRef(name)));
}

facebook::jsi::Value ChakraRuntime::getProperty(const facebook::jsi::Object &obj, const facebook::jsi::String &name) {
  return ToJsiValue(
      GetProperty(GetChakraObjectRef(obj), GetPropertyIdFromName(StringToPointer(GetChakraObjectRef(name)).data())));
}

bool ChakraRuntime::hasProperty(const facebook::jsi::Object &obj, const facebook::jsi::PropNameID &name) {
  bool result{false};
  VerifyJsErrorElseThrow(JsHasProperty(GetChakraObjectRef(obj), GetChakraObjectRef(name), &result));
  return result;
}

bool ChakraRuntime::hasProperty(const facebook::jsi::Object &obj, const facebook::jsi::String &name) {
  bool result{false};
  VerifyJsErrorElseThrow(JsHasProperty(
      GetChakraObjectRef(obj), GetPropertyIdFromName(StringToPointer(GetChakraObjectRef(name)).data()), &result));
  return result;
}

void ChakraRuntime::setPropertyValue(
    facebook::jsi::Object &object,
    const facebook::jsi::PropNameID &name,
    const facebook::jsi::Value &value) {
  // We use strict rules for property assignments here, so any assignment that
  // silently fails in normal code (assignment to a non-writable global or
  // property, assignment to a getter-only property, assignment to a new
  // property on a non-extensible object) will throw.
  VerifyJsErrorElseThrow(JsSetProperty(
      GetChakraObjectRef(object), GetChakraObjectRef(name), ToChakraObjectRef(value), true /* useStrictRules */));
}

void ChakraRuntime::setPropertyValue(
    facebook::jsi::Object &object,
    const facebook::jsi::String &name,
    const facebook::jsi::Value &value) {
  VerifyJsErrorElseThrow(JsSetProperty(
      GetChakraObjectRef(object),
      GetPropertyIdFromName(StringToPointer(GetChakraObjectRef(name)).data()),
      ToChakraObjectRef(value),
      true /* useStrictRules */));
}

bool ChakraRuntime::isArray(const facebook::jsi::Object &object) const {
  return GetValueType(GetChakraObjectRef(object)) == JsArray;
}

bool ChakraRuntime::isArrayBuffer(const facebook::jsi::Object &object) const {
  return GetValueType(GetChakraObjectRef(object)) == JsArrayBuffer;
}

bool ChakraRuntime::isFunction(const facebook::jsi::Object &obj) const {
  return GetValueType(GetChakraObjectRef(obj)) == JsFunction;
}

bool ChakraRuntime::isHostObject(const facebook::jsi::Object &obj) const {
  auto self = const_cast<ChakraRuntime *>(this);
  JsValueRef hostObject = self->GetProperty(GetChakraObjectRef(obj), m_propertyId.hostObjectSymbol);
  if (GetValueType(hostObject) == JsValueType::JsObject) {
    return GetExternalData(hostObject) != nullptr;
  } else {
    return false;
  }
}

bool ChakraRuntime::isHostFunction(const facebook::jsi::Function &func) const {
  ChakraRuntime *self = const_cast<ChakraRuntime *>(this);
  JsValueRef hostFunction = self->GetProperty(GetChakraObjectRef(func), m_propertyId.hostFunctionSymbol);
  if (GetValueType(hostFunction) == JsValueType::JsObject) {
    return GetExternalData(hostFunction) != nullptr;
  } else {
    return false;
  }
}

facebook::jsi::Array ChakraRuntime::getPropertyNames(const facebook::jsi::Object &object) {
  // Handle to the null JS value.
  JsValueRef jsNull = GetNullValue();

  // Handle to the Object constructor.
  JsValueRef objectConstructor = GetProperty(GetGlobalObject(), m_propertyId.Object);

  // Handle to the Object.prototype Object.
  JsValueRef objectPrototype = GetProperty(objectConstructor, m_propertyId.prototype);

  // Handle to the Object.prototype.propertyIsEnumerable() Function.
  JsValueRef objectPrototypePropertyIsEnumerable = GetProperty(objectPrototype, m_propertyId.propertyIsEnumerable);

  // We now traverse the object's property chain and collect all enumerable property names.
  std::vector<JsValueRef> enumerablePropNames{};
  JsValueRef currentObjectOnPrototypeChain = GetChakraObjectRef(object);

  // We have a small optimization here where we stop traversing the prototype
  // chain as soon as we hit Object.prototype. However, we still need to check
  // for null here, as one can create an Object with no prototype through
  // Object.create(null).
  while (!StrictEquals(currentObjectOnPrototypeChain, objectPrototype) &&
         !StrictEquals(currentObjectOnPrototypeChain, jsNull)) {
    JsValueRef propNames = GetOwnPropertyNames(currentObjectOnPrototypeChain);
    int propNamesSize = NumberToInt(GetProperty(propNames, m_propertyId.length));

    enumerablePropNames.reserve(enumerablePropNames.size() + propNamesSize);
    for (int i = 0; i < propNamesSize; ++i) {
      JsValueRef propName = GetIndexedProperty(propNames, i);
      bool propIsEnumerable =
          BooleanToBool(CallFunction(objectPrototypePropertyIsEnumerable, {currentObjectOnPrototypeChain, propName}));
      if (propIsEnumerable) {
        enumerablePropNames.push_back(propName);
      }
    }

    currentObjectOnPrototypeChain = GetPrototype(currentObjectOnPrototypeChain);
  }

  size_t enumerablePropNamesSize = enumerablePropNames.size();
  JsValueRef result = CreateArray(enumerablePropNamesSize);

  for (size_t i = 0; i < enumerablePropNamesSize; ++i) {
    SetIndexedProperty(result, i, enumerablePropNames[i]);
  }

  return MakePointer<facebook::jsi::Object>(result).getArray(*this);
}

// Only ChakraCore supports weak reference semantics, so ChakraRuntime
// WeakObjects are in fact strong references.

facebook::jsi::WeakObject ChakraRuntime::createWeakObject(const facebook::jsi::Object &object) {
  return make<facebook::jsi::WeakObject>(CloneChakraPointerValue(getPointerValue(object)));
}

facebook::jsi::Value ChakraRuntime::lockWeakObject(const facebook::jsi::WeakObject &weakObject) {
  // We need to make a copy of the ChakraObjectRef held within weakObj's
  // member PointerValue for the returned jsi::Value here.
  return ToJsiValue(GetChakraObjectRef(weakObject));
}

facebook::jsi::Array ChakraRuntime::createArray(size_t length) {
  assert(length <= (std::numeric_limits<unsigned int>::max)());

  return MakePointer<facebook::jsi::Object>(CreateArray(length)).asArray(*this);
}

size_t ChakraRuntime::size(const facebook::jsi::Array &arr) {
  assert(isArray(arr));

  int result = ToInteger(GetProperty(GetChakraObjectRef(arr), m_propertyId.length));

  if (result < 0) {
    throw facebook::jsi::JSINativeException("Invalid JS array length detected.");
  }

  return static_cast<size_t>(result);
}

size_t ChakraRuntime::size(const facebook::jsi::ArrayBuffer &arrBuf) {
  assert(isArrayBuffer(arrBuf));

  int result = ToInteger(GetProperty(GetChakraObjectRef(arrBuf), m_propertyId.byteLength));

  if (result < 0) {
    throw facebook::jsi::JSINativeException("Invalid JS array buffer byteLength detected.");
  }

  return static_cast<size_t>(result);
}

uint8_t *ChakraRuntime::data(const facebook::jsi::ArrayBuffer &arrBuf) {
  assert(isArrayBuffer(arrBuf));

  uint8_t *buffer = nullptr;
  unsigned int size = 0;

  VerifyJsErrorElseThrow(JsGetArrayBufferStorage(GetChakraObjectRef(arrBuf), &buffer, &size));

  return buffer;
}

facebook::jsi::Value ChakraRuntime::getValueAtIndex(const facebook::jsi::Array &arr, size_t index) {
  assert(isArray(arr));
  assert(index <= static_cast<size_t>((std::numeric_limits<int>::max)()));

  JsValueRef result = JS_INVALID_REFERENCE;
  VerifyJsErrorElseThrow(JsGetIndexedProperty(GetChakraObjectRef(arr), ToJsNumber(static_cast<int>(index)), &result));
  return ToJsiValue(result);
}

void ChakraRuntime::setValueAtIndexImpl(facebook::jsi::Array &arr, size_t index, const facebook::jsi::Value &value) {
  assert(isArray(arr));
  assert(index <= static_cast<size_t>((std::numeric_limits<int>::max)()));

  VerifyJsErrorElseThrow(
      JsSetIndexedProperty(GetChakraObjectRef(arr), ToJsNumber(static_cast<int>(index)), ToChakraObjectRef(value)));
}

facebook::jsi::Function ChakraRuntime::createFunctionFromHostFunction(
    const facebook::jsi::PropNameID &name,
    unsigned int paramCount,
    facebook::jsi::HostFunctionType func) {
  auto hostFunctionWrapper = std::make_unique<HostFunctionWrapper>(std::move(func), *this);
  JsValueRef function = CreateExternalFunction(
      GetChakraObjectRef(name), (int32_t)paramCount, HostFunctionCall, hostFunctionWrapper.get());

  auto funcWrapper = CreateExternalObject(std::move(hostFunctionWrapper));
  DefineProperty(
      function,
      m_propertyId.hostFunctionSymbol,
      CreatePropertyDescriptor(funcWrapper, PropertyAttibutes::DontEnumAndFrozen));

  facebook::jsi::Object hostFuncObj = MakePointer<facebook::jsi::Object>(function);
  return hostFuncObj.getFunction(*this);
}

facebook::jsi::Value ChakraRuntime::call(
    const facebook::jsi::Function &func,
    const facebook::jsi::Value &jsThis,
    const facebook::jsi::Value *args,
    size_t count) {
  size_t jsArgCount = count + 1; // for 'this' argument.
  ChakraVerifyElseThrow(jsArgCount <= MaxCallArgCount, "Argument count exceeds MaxCallArgCount");
  std::array<JsValueRef, MaxCallArgCount> jsArgs;
  jsArgs[0] = ToChakraObjectRef(jsThis);
  for (size_t i = 0; i < count; ++i) {
    jsArgs[i + 1] = ToChakraObjectRef(args[i]);
  }

  return ToJsiValue(CallFunction(GetChakraObjectRef(func), JsValueRefSpan(jsArgs.data(), jsArgCount)));
}

facebook::jsi::Value
ChakraRuntime::callAsConstructor(const facebook::jsi::Function &func, const facebook::jsi::Value *args, size_t count) {
  size_t jsArgCount = count + 1; // for 'this' argument.
  ChakraVerifyElseThrow(jsArgCount <= MaxCallArgCount, "Argument count exceeds MaxCallArgCount");
  std::array<JsValueRef, MaxCallArgCount> jsArgs;
  jsArgs[0] = m_undefinedValue;
  for (size_t i = 0; i < count; ++i) {
    jsArgs[i + 1] = ToChakraObjectRef(args[i]);
  }

  return ToJsiValue(ConstructObject(GetChakraObjectRef(func), JsValueRefSpan(jsArgs.data(), jsArgCount)));
}

facebook::jsi::Runtime::ScopeState *ChakraRuntime::pushScope() {
  return nullptr;
}

void ChakraRuntime::popScope([[maybe_unused]] Runtime::ScopeState *state) {
  assert(state == nullptr);
  VerifyJsErrorElseThrow(JsCollectGarbage(m_runtime));
}

bool ChakraRuntime::strictEquals(const facebook::jsi::Symbol &a, const facebook::jsi::Symbol &b) const {
  return StrictEquals(GetChakraObjectRef(a), GetChakraObjectRef(b));
}

bool ChakraRuntime::strictEquals(const facebook::jsi::String &a, const facebook::jsi::String &b) const {
  return StrictEquals(GetChakraObjectRef(a), GetChakraObjectRef(b));
}

bool ChakraRuntime::strictEquals(const facebook::jsi::Object &a, const facebook::jsi::Object &b) const {
  return StrictEquals(GetChakraObjectRef(a), GetChakraObjectRef(b));
}

bool ChakraRuntime::instanceOf(const facebook::jsi::Object &obj, const facebook::jsi::Function &func) {
  return InstanceOf(GetChakraObjectRef(obj), GetChakraObjectRef(func));
}

#pragma endregion Functions_inherited_from_Runtime

void ChakraRuntime::VerifyJsErrorElseThrow(JsErrorCode error) {
  switch (error) {
    case JsNoError:
      return;

    case JsErrorScriptException: {
      JsValueRef jsError;
      VerifyChakraErrorElseThrow(JsGetAndClearException(&jsError));
      throw facebook::jsi::JSError("", *this, ToJsiValue(jsError));
    }

    default: { VerifyChakraErrorElseThrow(error); }
  }

  // We must never get here.
  throw facebook::jsi::JSINativeException("Did not handle the JsErrorCode");
}

facebook::jsi::Value ChakraRuntime::ToJsiValue(JsValueRef ref) {
  switch (GetValueType(ref)) {
    case JsUndefined:
      return facebook::jsi::Value::undefined();
    case JsNull:
      return facebook::jsi::Value::null();
    case JsNumber:
      return facebook::jsi::Value(NumberToDouble(ref));
    case JsString:
      return facebook::jsi::Value(*this, MakePointer<facebook::jsi::String>(ref));
    case JsBoolean:
      return facebook::jsi::Value(BooleanToBool(ref));
    case JsSymbol:
      return facebook::jsi::Value(*this, MakePointer<facebook::jsi::Symbol>(ref));
    case JsObject:
    case JsFunction:
    case JsError:
    case JsArray:
    case JsArrayBuffer:
    case JsTypedArray:
    case JsDataView:
      return facebook::jsi::Value(*this, MakePointer<facebook::jsi::Object>(ref));
    default:
      throw facebook::jsi::JSINativeException("Unexpected value type");
  }
}

JsValueRef ChakraRuntime::ToChakraObjectRef(const facebook::jsi::Value &value) {
  if (value.isUndefined()) {
    return m_undefinedValue;
  } else if (value.isNull()) {
    return GetNullValue();
  } else if (value.isBool()) {
    return BoolToBoolean(value.getBool());
  } else if (value.isNumber()) {
    return DoubleToNumber(value.getNumber());
  } else if (value.isSymbol()) {
    return GetChakraObjectRef(value.getSymbol(*this));
  } else if (value.isString()) {
    return GetChakraObjectRef(value.getString(*this));
  } else if (value.isObject()) {
    return GetChakraObjectRef(value.getObject(*this));
  } else {
    throw facebook::jsi::JSINativeException("Unexpected jsi::Value type");
  }
}

JsValueRef ChakraRuntime::GetProperty(JsValueRef obj, JsPropertyIdRef id) {
  JsValueRef result{JS_INVALID_REFERENCE};
  VerifyJsErrorElseThrow(JsGetProperty(obj, id, &result));
  return result;
}

JsValueRef ChakraRuntime::CreateExternalFunction(
    JsPropertyIdRef name,
    int32_t paramCount,
    JsNativeFunction nativeFunction,
    void *callbackState) {
  JsValueRef nameString = PropertyIdToString(name);
  JsValueRef function = CreateNamedFunction(nameString, nativeFunction, callbackState);
  DefineProperty(
      function,
      m_propertyId.length,
      CreatePropertyDescriptor(IntToNumber(paramCount), PropertyAttibutes::DontEnumAndFrozen));
  return function;
}

JsValueRef CALLBACK ChakraRuntime::HostFunctionCall(
    JsValueRef /*callee*/,
    bool isConstructCall,
    JsValueRef *args,
    unsigned short argCount,
    void *callbackState) {
  HostFunctionWrapper *hostFuncWraper = static_cast<HostFunctionWrapper *>(callbackState);
  ChakraRuntime &chakraRuntime = hostFuncWraper->GetRuntime();
  return chakraRuntime.HandleCallbackExceptions([&]() {
    ChakraVerifyElseThrow(!isConstructCall, "Constructor call for HostObjectGetTrap() is not supported.");

    ChakraVerifyElseThrow(argCount > 0, "There must be at least 'this' argument.");
    JsiValueView jsiThisArg{*args};
    JsiValueViewArray jsiArgs{args + 1, argCount - 1u};

    const facebook::jsi::HostFunctionType &hostFunc = hostFuncWraper->GetHostFunction();
    return chakraRuntime.ToChakraObjectRef(hostFunc(chakraRuntime, jsiThisArg, jsiArgs.Data(), jsiArgs.Size()));
  });
}

/*static*/ JsValueRef CALLBACK ChakraRuntime::HostObjectGetTrap(
    JsValueRef /*callee*/,
    bool isConstructCall,
    JsValueRef *args,
    unsigned short argCount,
    void *callbackState) noexcept {
  ChakraRuntime *chakraRuntime = static_cast<ChakraRuntime *>(callbackState);
  return chakraRuntime->HandleCallbackExceptions([&]() {
    ChakraVerifyElseThrow(!isConstructCall, "Constructor call for HostObjectGetTrap() is not supported.");

    // args[0] - the Proxy handler object (this) (unused).
    // args[1] - the Proxy target object.
    // args[2] - the name of the property to set.
    // args[3] - the Proxy object (unused).
    ChakraVerifyElseThrow(argCount == 4, "HostObjectGetTrap() requires 4 arguments.");
    JsValueRef target = args[1];
    JsValueRef propertyName = args[2];
    if (GetValueType(propertyName) == JsValueType::JsString) {
      auto const &hostObject = *static_cast<std::shared_ptr<facebook::jsi::HostObject> *>(GetExternalData(target));
      PropNameIDView propertyId{GetPropertyIdFromName(StringToPointer(propertyName).data())};
      return chakraRuntime->ToChakraObjectRef(hostObject->get(*chakraRuntime, propertyId));
    } else if (
        GetValueType(propertyName) == JsValueType::JsSymbol &&
        GetPropertyIdFromSymbol(propertyName) == chakraRuntime->m_propertyId.hostObjectSymbol) {
      // The special property to retrieve the target object.
      return target;
    }

    return static_cast<JsValueRef>(chakraRuntime->m_undefinedValue);
  });
}

/*static*/ JsValueRef CALLBACK ChakraRuntime::HostObjectSetTrap(
    JsValueRef /*callee*/,
    bool isConstructCall,
    JsValueRef *args,
    unsigned short argCount,
    void *callbackState) noexcept {
  ChakraRuntime *chakraRuntime = static_cast<ChakraRuntime *>(callbackState);
  return chakraRuntime->HandleCallbackExceptions([&]() {
    ChakraVerifyElseThrow(!isConstructCall, "Constructor call for HostObjectSetTrap() is not supported.");

    // args[0] - the Proxy handler object (this) (unused).
    // args[1] - the Proxy target object.
    // args[2] - the name of the property to set.
    // args[3] - the new value of the property to set.
    // args[4] - the Proxy object (unused).
    ChakraVerifyElseThrow(argCount == 5, "HostObjectSetTrap() requires 5 arguments.");

    JsValueRef target = args[1];
    JsValueRef propertyName = args[2];
    if (GetValueType(propertyName) == JsValueType::JsString) {
      auto const &hostObject = *static_cast<std::shared_ptr<facebook::jsi::HostObject> *>(GetExternalData(target));
      PropNameIDView propertyId{GetPropertyIdFromName(StringToPointer(propertyName).data())};
      JsiValueView value{args[3]};
      hostObject->set(*chakraRuntime, propertyId, value);
    }

    return static_cast<JsValueRef>(chakraRuntime->m_undefinedValue);
  });
}

/*static*/ JsValueRef CALLBACK ChakraRuntime::HostObjectOwnKeysTrap(
    JsValueRef /*callee*/,
    bool isConstructCall,
    JsValueRef *args,
    unsigned short argCount,
    void *callbackState) noexcept {
  ChakraRuntime *chakraRuntime = static_cast<ChakraRuntime *>(callbackState);
  return chakraRuntime->HandleCallbackExceptions([&]() {
    ChakraVerifyElseThrow(!isConstructCall, "Constructor call for HostObjectOwnKeysTrap() is not supported.");

    // args[0] - the Proxy handler object (this) (unused).
    // args[1] - the Proxy target object.
    ChakraVerifyElseThrow(argCount == 2, "HostObjectOwnKeysTrap() requires 2 arguments.");
    JsValueRef target = args[1];
    auto const &hostObject = *static_cast<std::shared_ptr<facebook::jsi::HostObject> *>(GetExternalData(target));

    auto ownKeys = hostObject->getPropertyNames(*chakraRuntime);

    std::unordered_set<JsPropertyIdRef> dedupedOwnKeys{};
    dedupedOwnKeys.reserve(ownKeys.size());
    for (facebook::jsi::PropNameID const &key : ownKeys) {
      dedupedOwnKeys.insert(GetChakraObjectRef(key));
    }

    JsValueRef result = CreateArray(dedupedOwnKeys.size());
    size_t index = 0;
    for (JsPropertyIdRef key : dedupedOwnKeys) {
      SetIndexedProperty(result, index, PropertyIdToString(key));
      ++index;
    }

    return result;
  });
}

JsValueRef ChakraRuntime::GetHostObjectProxyHandler() {
  if (!m_hostObjectProxyHandler) {
    JsValueRef handler = CreateObject();
    SetProperty(handler, m_propertyId.get, CreateExternalFunction(m_propertyId.get, 2, HostObjectGetTrap, this));
    SetProperty(handler, m_propertyId.set, CreateExternalFunction(m_propertyId.set, 3, HostObjectSetTrap, this));
    SetProperty(
        handler, m_propertyId.ownKeys, CreateExternalFunction(m_propertyId.ownKeys, 1, HostObjectOwnKeysTrap, this));
    m_hostObjectProxyHandler = ChakraObjectRef{handler};
  }

  return m_hostObjectProxyHandler;
}

// TODO: review
void ChakraRuntime::setupMemoryTracker() noexcept {
  if (runtimeArgs().memoryTracker) {
    size_t initialMemoryUsage = 0;
    JsGetRuntimeMemoryUsage(m_runtime, &initialMemoryUsage);
    runtimeArgs().memoryTracker->Initialize(initialMemoryUsage);

    if (runtimeArgs().runtimeMemoryLimit > 0)
      JsSetRuntimeMemoryLimit(m_runtime, runtimeArgs().runtimeMemoryLimit);

    JsSetRuntimeMemoryAllocationCallback(
        m_runtime,
        runtimeArgs().memoryTracker.get(),
        [](void *callbackState, JsMemoryEventType allocationEvent, size_t allocationSize) -> bool {
          auto memoryTrackerPtr = static_cast<facebook::react::MemoryTracker *>(callbackState);
          switch (allocationEvent) {
            case JsMemoryAllocate:
              memoryTrackerPtr->OnAllocation(allocationSize);
              break;

            case JsMemoryFree:
              memoryTrackerPtr->OnDeallocation(allocationSize);
              break;

            case JsMemoryFailure:
            default:
              break;
          }

          return true;
        });
  }
}

//===========================================================================
// ChakraRuntime::JsiValueView implementation
//===========================================================================

ChakraRuntime::JsiValueView::JsiValueView(JsValueRef jsValue) noexcept
    : m_value{InitValue(jsValue, std::addressof(m_pointerStore))} {}

ChakraRuntime::JsiValueView::~JsiValueView() noexcept {}

ChakraRuntime::JsiValueView::operator facebook::jsi::Value const &() const noexcept {
  return m_value;
}

/*static*/ facebook::jsi::Value ChakraRuntime::JsiValueView::InitValue(JsValueRef jsValue, StoreType *store) noexcept {
  switch (GetValueType(jsValue)) {
    case JsUndefined:
      return facebook::jsi::Value::undefined();
    case JsNull:
      return facebook::jsi::Value::null();
    case JsNumber:
      return facebook::jsi::Value(NumberToDouble(jsValue));
    case JsString:
      return make<facebook::jsi::String>(new (store) ChakraPointerValueView(jsValue));
    case JsBoolean:
      return facebook::jsi::Value(BooleanToBool(jsValue));
    case JsSymbol:
      return make<facebook::jsi::Symbol>(new (store) ChakraPointerValueView(jsValue));
    case JsObject:
    case JsFunction:
    case JsError:
    case JsArray:
    case JsArrayBuffer:
    case JsTypedArray:
    case JsDataView:
      return make<facebook::jsi::Object>(new (store) ChakraPointerValueView(jsValue));
    default:
      throw facebook::jsi::JSINativeException("Unexpected JsValueType");
  }
}

//===========================================================================
// ChakraRuntime::JsiValueViewArray implementation
//===========================================================================

ChakraRuntime::JsiValueViewArray::JsiValueViewArray(JsValueRef *args, size_t argCount) noexcept : m_size{argCount} {
  ChakraVerifyElseThrow(m_size <= MaxCallArgCount, "Argument count must not exceed the MaxCallArgCount");
  for (uint32_t i = 0; i < m_size; ++i) {
    m_valueArray[i] = JsiValueView::InitValue(args[i], std::addressof(m_pointerStoreArray[i]));
  }
}

facebook::jsi::Value const *ChakraRuntime::JsiValueViewArray::Data() const noexcept {
  return m_valueArray.data();
}

size_t ChakraRuntime::JsiValueViewArray::Size() const noexcept {
  return m_size;
}

//===========================================================================
// ChakraRuntime::PropNameIDView implementation
//===========================================================================

ChakraRuntime::PropNameIDView::PropNameIDView(JsPropertyIdRef propertyId) noexcept
    : m_propertyId{
          make<facebook::jsi::PropNameID>(new (std::addressof(m_pointerStore)) ChakraPointerValueView(propertyId))} {}

ChakraRuntime::PropNameIDView::~PropNameIDView() noexcept {}

ChakraRuntime::PropNameIDView::operator facebook::jsi::PropNameID const &() const noexcept {
  return m_propertyId;
}

std::once_flag ChakraRuntime::s_runtimeVersionInitFlag;
uint64_t ChakraRuntime::s_runtimeVersion = 0;

std::unique_ptr<facebook::jsi::Runtime> makeChakraRuntime(ChakraRuntimeArgs &&args) noexcept {
  return std::make_unique<ChakraRuntime>(std::move(args));
}

} // namespace Microsoft::JSI

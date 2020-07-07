// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "JsiAbiApi.h"

namespace winrt::Microsoft::ReactNative {

//===========================================================================
// JsiBufferWrapper implementation
//===========================================================================

JsiBufferWrapper::JsiBufferWrapper(std::shared_ptr<facebook::jsi::Buffer const> const &buffer) noexcept
    : m_buffer{buffer} {}

JsiBufferWrapper::~JsiBufferWrapper() = default;

uint32_t JsiBufferWrapper::Size() {
  return static_cast<uint32_t>(m_buffer->size());
}

void JsiBufferWrapper::GetData(JsiDataHandler const &handler) {
  handler(winrt::array_view<uint8_t const>{m_buffer->data(), m_buffer->data() + m_buffer->size()});
}

//===========================================================================
// JsiPreparedJavaScriptWrapper implementation
//===========================================================================

JsiPreparedJavaScriptWrapper::JsiPreparedJavaScriptWrapper(JsiPreparedJavaScript const &preparedScript) noexcept
    : m_preparedScript{preparedScript} {}

JsiPreparedJavaScriptWrapper::~JsiPreparedJavaScriptWrapper() noexcept = default;

JsiPreparedJavaScript const &JsiPreparedJavaScriptWrapper::Get() const noexcept {
  return m_preparedScript;
}

//===========================================================================
// JsiHostObjectWrapper implementation
//===========================================================================

JsiHostObjectWrapper::JsiHostObjectWrapper(std::shared_ptr<facebook::jsi::HostObject> &&hostObject) noexcept
    : m_hostObject(std::move(hostObject)) {}

JsiHostObjectWrapper::~JsiHostObjectWrapper() noexcept {
  if (m_objectHandle) {
    std::scoped_lock lock{s_mutex};
    s_objectHandleToObjectWrapper.erase(m_objectHandle);
  }
}

JsiValueData JsiHostObjectWrapper::GetProperty(IJsiRuntime const &runtime, JsiPointerHandle name) {
  JsiAbiRuntime rt{runtime};
  return ToJsiValueData(m_hostObject->get(rt, *reinterpret_cast<facebook::jsi::PropNameID *>(name)));
}

void JsiHostObjectWrapper::SetProperty(IJsiRuntime const &runtime, JsiPointerHandle name, JsiValueData const &value) {
  JsiAbiRuntime rt{runtime};
  m_hostObject->set(
      rt,
      *reinterpret_cast<facebook::jsi::PropNameID *>(name),
      *reinterpret_cast<facebook::jsi::Value const *>(&value));
}

Windows::Foundation::Collections::IVectorView<JsiPointerHandle> JsiHostObjectWrapper::GetPropertyNames(
    IJsiRuntime const &runtime) {
  JsiAbiRuntime rt{runtime};
  auto names = m_hostObject->getPropertyNames(rt);
  std::vector<JsiPointerHandle> result;
  result.reserve(names.size());
  for (auto &name : names) {
    result.push_back(*reinterpret_cast<JsiPointerHandle *>(&name));
  }

  return winrt::single_threaded_vector<JsiPointerHandle>(std::move(result)).GetView();
}

/*static*/ void JsiHostObjectWrapper::RegisterHostObject(
    JsiPointerHandle handle,
    JsiHostObjectWrapper *hostObject) noexcept {
  std::scoped_lock lock{s_mutex};
  s_objectHandleToObjectWrapper[handle] = hostObject;
  hostObject->m_objectHandle = handle;
}

/*static*/ bool JsiHostObjectWrapper::IsHostObject(JsiPointerHandle handle) noexcept {
  std::scoped_lock lock{s_mutex};
  auto it = s_objectHandleToObjectWrapper.find(handle);
  return it != s_objectHandleToObjectWrapper.end();
}

/*static*/ std::shared_ptr<facebook::jsi::HostObject> JsiHostObjectWrapper::GetHostObject(
    JsiPointerHandle handle) noexcept {
  auto it = s_objectHandleToObjectWrapper.find(handle);
  if (it != s_objectHandleToObjectWrapper.end()) {
    return it->second->m_hostObject;
  } else {
    return nullptr;
  }
}

//===========================================================================
// JsiHostFunctionWrapper implementation
//===========================================================================

JsiHostFunctionWrapper::JsiHostFunctionWrapper(
    facebook::jsi::HostFunctionType &&hostFunction,
    uint32_t functionId) noexcept
    : m_hostFunction{std::move(hostFunction)}, m_functionId{functionId} {
  VerifyElseCrashSz(functionId, "Function Id must be not zero");
  std::scoped_lock lock{s_functionMutex};
  s_functionIdToFunctionWrapper[functionId] = this;
}

JsiHostFunctionWrapper::JsiHostFunctionWrapper(JsiHostFunctionWrapper &&other) noexcept
    : m_hostFunction{std::move(other.m_hostFunction)},
      m_functionId{std::exchange(other.m_functionId, 0)},
      m_functionHandle{std::exchange(other.m_functionHandle, 0)} {
  std::scoped_lock lock{s_functionMutex};
  if (m_functionId) {
    s_functionIdToFunctionWrapper[m_functionId] = this;
  }
  if (m_functionHandle) {
    s_functionHandleToFunctionWrapper[m_functionHandle] = this;
  }
}

JsiHostFunctionWrapper &JsiHostFunctionWrapper::operator=(JsiHostFunctionWrapper &&other) noexcept {
  if (this != &other) {
    m_hostFunction = std::move(other.m_hostFunction);
    m_functionId = std::exchange(other.m_functionId, 0);
    m_functionHandle = std::exchange(other.m_functionHandle, 0);
    std::scoped_lock lock{s_functionMutex};
    if (m_functionId) {
      s_functionIdToFunctionWrapper[m_functionId] = this;
    }
    if (m_functionHandle) {
      s_functionHandleToFunctionWrapper[m_functionHandle] = this;
    }
  }
  return *this;
}

JsiHostFunctionWrapper::~JsiHostFunctionWrapper() noexcept {
  if (m_functionId || m_functionHandle) {
    std::scoped_lock lock{s_functionMutex};
    auto it1 = s_functionIdToFunctionWrapper.find(m_functionId);
    if (it1 != s_functionIdToFunctionWrapper.end()) {
      s_functionIdToFunctionWrapper.erase(it1);
    }
    auto it2 = s_functionHandleToFunctionWrapper.find(m_functionHandle);
    if (it2 != s_functionHandleToFunctionWrapper.end()) {
      s_functionHandleToFunctionWrapper.erase(it2);
    }
  }
}

JsiValueData JsiHostFunctionWrapper::operator()(
    IJsiRuntime const &runtime,
    JsiValueData const &thisValue,
    array_view<JsiValueData const> args) {
  auto rt{JsiAbiRuntime{runtime}};
  return ToJsiValueData(
      m_hostFunction(rt, ToValue(thisValue), reinterpret_cast<facebook::jsi::Value const *>(args.data()), args.size()));
}

/*static*/ uint32_t JsiHostFunctionWrapper::GetNextFunctionId() noexcept {
  return ++s_functionIdGenerator;
}

/*static*/ void JsiHostFunctionWrapper::RegisterHostFunction(uint32_t functionId, JsiPointerHandle handle) noexcept {
  std::scoped_lock lock{s_functionMutex};
  auto it = s_functionIdToFunctionWrapper.find(functionId);
  VerifyElseCrashSz(it != s_functionIdToFunctionWrapper.end(), "Function Id is not found.");
  JsiHostFunctionWrapper *functionWrapper = it->second;
  s_functionHandleToFunctionWrapper[handle] = functionWrapper;
  functionWrapper->m_functionHandle = handle;
}

/*static*/ bool JsiHostFunctionWrapper::IsHostFunction(JsiPointerHandle functionHandle) noexcept {
  std::scoped_lock lock{s_functionMutex};
  return s_functionHandleToFunctionWrapper.find(functionHandle) != s_functionHandleToFunctionWrapper.end();
}

/*static*/ facebook::jsi::HostFunctionType &JsiHostFunctionWrapper::GetHostFunction(
    JsiPointerHandle functionHandle) noexcept {
  std::scoped_lock lock{s_functionMutex};
  auto it = s_functionHandleToFunctionWrapper.find(functionHandle);
  if (it != s_functionHandleToFunctionWrapper.end()) {
    return it->second->m_hostFunction;
  }

  VerifyElseCrashSz(false, "Function is not a host function");
}

} // namespace winrt::Microsoft::ReactNative

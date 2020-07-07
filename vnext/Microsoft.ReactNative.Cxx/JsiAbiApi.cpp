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

} // namespace winrt::Microsoft::ReactNative

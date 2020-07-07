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

} // namespace winrt::Microsoft::ReactNative

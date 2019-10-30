// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#pragma once

#include <mutex>
#include "winrt/Microsoft.ReactNative.h"

namespace winrt::Microsoft::ReactNative {

struct UIDispatchQueue
    : winrt::implements<UIDispatchQueue, ISerialDispatchQueue> {
  // Associate the UIDispatchQueue with the 
  UIDispatchQueue(
      winrt::Windows::UI::Core::CoreDispatcher const &coreDispatcher) noexcept;

 public:
  void Shutdown() noexcept;

 public: // ISerialDispatchQueue implementation
  bool HasThreadAccess() noexcept;
  void Post(DispatchQueueCallback const &callback) noexcept;

 private:
  void InvokeCallback() noexcept;

 private:
  winrt::Windows::UI::Core::CoreDispatcher m_coreDispatcher{nullptr};
  winrt::Windows::UI::Core::DispatchedHandler m_dispatchedHandler{nullptr};
  std::recursive_mutex m_lock;
  bool m_isShutdown{false};
  bool m_workSubmitted{false};
  std::list<DispatchQueueCallback> m_callbacks;
};

} // namespace winrt::Microsoft::ReactNative

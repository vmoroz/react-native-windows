// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#pragma once

#include "SimpleEvent.h"
#include "winrt/Microsoft.ReactNative.h"

namespace winrt::Microsoft::ReactNative {

struct LooperDispatchQueue
    : winrt::implements<LooperDispatchQueue, ISerialDispatchQueue> {
  LooperDispatchQueue() noexcept;
  static void final_release(std::unique_ptr<LooperDispatchQueue> self);

 public:
  void RunLoop() noexcept;
  void Shutdown() noexcept;

 public: // ISerialDispatchQueue implementation
  bool HasThreadAccess() noexcept;
  void Post(DispatchQueueCallback const &callback) noexcept;

 private:
  void InvokeCallback() noexcept;

 private:
  std::recursive_mutex m_lock;
  bool m_isShutdown{false};
  bool m_workSubmitted{false};
  std::list<DispatchQueueCallback> m_callbacks;

  std::unique_ptr<LooperThreadCallback> m_threadCallback;
};

} // namespace winrt::Microsoft::ReactNative

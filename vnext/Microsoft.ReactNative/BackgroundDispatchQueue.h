// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#pragma once

#include <mutex>
#include "winrt/Microsoft.ReactNative.h"

namespace winrt::Microsoft::ReactNative {

struct BackgroundDispatchQueue
    : winrt::implements<BackgroundDispatchQueue, ISerialDispatchQueue> {
  BackgroundDispatchQueue() noexcept;

 public: // ISerialDispatchQueue implementation
  bool HasThreadAccess() noexcept;
  void Post(DispatchQueueCallback const &callback) noexcept;

 public:
  void Shutdown() noexcept;
  void ShutdownAndWait() noexcept;

 private:
  void InvokeCallback() noexcept;
  static void __stdcall WorkCallback(
      _In_ PTP_CALLBACK_INSTANCE pInstance,
      _In_ PVOID pContext,
      _In_ PTP_WORK pWork) noexcept;

 private:
  struct ThreadPoolWorkDeleter {
    void operator()(TP_WORK *tpWork) noexcept;
  };

 private:
  std::atomic<uint32_t> m_processingThreadId{0};
  std::recursive_mutex m_lock;
  bool m_isShutdown{false};
  bool m_workSubmitted{false};
  std::list<DispatchQueueCallback> m_callbacks;
  std::unique_ptr<TP_WORK, ThreadPoolWorkDeleter> m_tpWork;
};

} // namespace winrt::Microsoft::ReactNative

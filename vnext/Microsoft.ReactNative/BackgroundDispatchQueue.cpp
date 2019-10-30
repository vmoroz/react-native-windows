// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#include "pch.h"

#include "BackgroundDispatchQueue.h"
#include "Crash.h"

namespace winrt::Microsoft::ReactNative {

void BackgroundDispatchQueue::ThreadPoolWorkDeleter::operator()(
    TP_WORK *tpWork) noexcept {
  if (tpWork) {
    ::CloseThreadpoolWork(tpWork);
  }
}

BackgroundDispatchQueue::BackgroundDispatchQueue() noexcept {
  m_tpWork.reset(::CreateThreadpoolWork(WorkCallback, this, nullptr));
  VerifyElseCrash(m_tpWork);
}

bool BackgroundDispatchQueue::HasThreadAccess() noexcept {
  return m_processingThreadId == ::GetCurrentThreadId();
}

void BackgroundDispatchQueue::Post(
    DispatchQueueCallback const &callback) noexcept {
  bool shouldCancel{false};
  bool shouldSubmit{false};

  {
    std::lock_guard<std::recursive_mutex> lock{m_lock};

    if (m_isShutdown) {
      shouldCancel = true;
    } else {
      m_callbacks.push_back(callback);

      // Only 1 ThreadPoolWork item submitted at a time.
      if (!m_workSubmitted) {
        shouldSubmit = true;
        m_workSubmitted = true;
      }
    }
  }

  // Do all the 'external' work outside of lock
  if (shouldCancel) {
    callback(/*isCanceled*/ true);
  } else if (shouldSubmit) {
    // Make sure that the queue is alive while we invoke callbacks.
    // We release it in WorkCallback
    AddRef();
    ::SubmitThreadpoolWork(m_tpWork.get());
  }
}

void BackgroundDispatchQueue::InvokeCallback() noexcept {
  uint32_t expectedCurrentThreadId = 0;
  VerifyElseCrash(m_processingThreadId.compare_exchange_strong(
      expectedCurrentThreadId, ::GetCurrentThreadId()));

  std::list<DispatchQueueCallback> callbacksToCancel;
  DispatchQueueCallback callback{};

  // Process work items as long as they still exist in the queue
  for (;;) {
    // Attempt to pop one work item off the front of the queue. If there isn't
    // one then exit. The lock must be engaged for this entire process to ensure
    // protection of m_workItems and m_pProcessingWorkItem in tandem.
    {
      std::lock_guard<std::recursive_mutex> lock(m_lock);

      if (m_isShutdown || m_callbacks.empty()) {
        callbacksToCancel = std::move(m_callbacks);
        m_processingThreadId = 0;
        m_workSubmitted = false;
        break;
      }

      callback = std::move(m_callbacks.front());
      m_callbacks.pop_front();
    }

    callback(/*isCanceled:*/ false);
    callback = nullptr;
  }

  while (!callbacksToCancel.empty()) {
    callback = std::move(callbacksToCancel.front());
    callbacksToCancel.pop_front();
    callback(/*isCanceled:*/ true);
    callback = nullptr;
  }
}

void __stdcall BackgroundDispatchQueue::WorkCallback(
    _In_ PTP_CALLBACK_INSTANCE /*pInstance*/,
    _In_ PVOID pContext,
    _In_ PTP_WORK /*pWork*/) noexcept {
  BackgroundDispatchQueue *dispatchQueue = (BackgroundDispatchQueue *)pContext;
  VerifyElseCrash(dispatchQueue);

  dispatchQueue->InvokeCallback();

  // Release the reference to the queue that was previously acquired in Post
  // method.
  dispatchQueue->Release();
}

void BackgroundDispatchQueue::Shutdown() noexcept {
  std::lock_guard<std::recursive_mutex> lock{m_lock};
  m_isShutdown = true;
}

void BackgroundDispatchQueue::ShutdownAndWait() noexcept {
  Shutdown();

  // TODO: It is not clear how it works when we did not submit the work yet.
  ::WaitForThreadpoolWorkCallbacks(m_tpWork.get(), FALSE);
}

} // namespace winrt::Microsoft::ReactNative

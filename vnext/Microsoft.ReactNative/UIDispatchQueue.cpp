// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#include "pch.h"

#include "Crash.h"
#include "UIDispatchQueue.h"

#include <chrono>

using namespace winrt::Windows::UI::Core;
using namespace std::chrono;
using namespace std::chrono_literals;

namespace winrt::Microsoft::ReactNative {

UIDispatchQueue::UIDispatchQueue(CoreDispatcher const &coreDispatcher) noexcept
    : m_coreDispatcher{coreDispatcher} {
  m_dispatchedHandler = DispatchedHandler([this]() noexcept {
    InvokeCallback();
    Release();
  });
}

bool UIDispatchQueue::HasThreadAccess() noexcept {
  return m_coreDispatcher.HasThreadAccess();
}

void UIDispatchQueue::Post(DispatchQueueCallback const &callback) noexcept {
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
    m_coreDispatcher.RunAsync(
        CoreDispatcherPriority::Normal, m_dispatchedHandler);
  }
}

void UIDispatchQueue::InvokeCallback() noexcept {
  std::list<DispatchQueueCallback> callbacksToCancel;
  DispatchQueueCallback callback{};
  bool shouldContinue{false};

  // We want to handle as many items as possible, but we do not want to
  // decrease performance of UI. Thus, we choose half of the time needed to
  // render 60 frames per second.
  const uint32_t framesPerSecond = 60;
  const duration processingTime = 1s / framesPerSecond / 2;
  const steady_clock::time_point finishTime{steady_clock::now() +
                                            processingTime};

  // Process work items as long as they still exist in the queue or we ran out
  // of time
  for (;;) {
    // Attempt to pop one work item off the front of the queue. If there isn't
    // one then exit. The lock must be engaged for this entire process to ensure
    // protection of m_workItems and m_pProcessingWorkItem in tandem.
    {
      std::lock_guard<std::recursive_mutex> lock(m_lock);

      if (m_isShutdown || m_callbacks.empty()) {
        callbacksToCancel = std::move(m_callbacks);
        m_workSubmitted = false;
        break;
      }

      if (steady_clock::now() >= finishTime) {
        shouldContinue = true;
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

  if (shouldContinue) {
    // Make sure that the queue is alive while we invoke callbacks.
    // We release it in WorkCallback
    AddRef();
    m_coreDispatcher.RunAsync(
        CoreDispatcherPriority::Normal, m_dispatchedHandler);
  }
}

void UIDispatchQueue::Shutdown() noexcept {
  std::lock_guard<std::recursive_mutex> lock{m_lock};
  m_isShutdown = true;
}

} // namespace winrt::Microsoft::ReactNative

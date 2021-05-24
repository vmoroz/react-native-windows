// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include <errorCode/exceptionErrorProvider.h>
#include <eventWaitHandle/eventWaitHandle.h>
#include "MessageDispatchQueue.h"
#include "MsoUtils.h"

namespace Mso::React {

//=============================================================================================
// MessageDispatchQueue implementation.
//=============================================================================================

MessageDispatchQueue::MessageDispatchQueue(
    Mso::DispatchQueue const &dispatchQueue,
    MessageDispatchQueueCallbacks &&callbacks) noexcept
    : m_dispatchQueue{dispatchQueue}, m_stopped{false}, m_callbacks{std::move(callbacks)} {}

MessageDispatchQueue::~MessageDispatchQueue() noexcept {}

void MessageDispatchQueue::runOnQueue(std::function<void()> &&func) {
  if (m_stopped) {
    return;
  }

  m_dispatchQueue.Post([pThis = shared_from_this(), func = std::move(func)]() noexcept {
    if (!pThis->m_stopped) {
      pThis->tryFunc(func);
    }
  });
}

void MessageDispatchQueue::tryFunc(const std::function<void()> &func) noexcept {
  try {
    func();
  } catch (const std::exception & /*ex*/) {
    if (auto errorHandler = m_callbacks.OnError.Get()) {
      errorHandler->Invoke(Mso::ExceptionErrorProvider().MakeErrorCode(std::current_exception()));
    }
  }
}

void MessageDispatchQueue::runSync(const Mso::VoidFunctorRef &func) noexcept {
  Mso::ManualResetEvent callbackFinished{};

  m_dispatchQueue.InvokeElsePost(Mso::MakeDispatchTask(
      /*callback:*/
      [&func, &callbackFinished]() noexcept {
        func();
        callbackFinished.Set();
      },
      /*onCancel:*/ [&func, &callbackFinished]() noexcept { callbackFinished.Set(); }));

  callbackFinished.Wait();
}

// runOnQueueSync and quitSynchronous are dangerous.  They should only be
// used for initialization and cleanup.
void MessageDispatchQueue::runOnQueueSync(std::function<void()> &&func) {
  if (m_stopped) {
    return;
  }

  runSync([this, &func]() noexcept {
    if (!m_stopped) {
      tryFunc(func);
    }
  });
}

// Once quitSynchronous() returns, no further work should run on the queue.
void MessageDispatchQueue::quitSynchronous() {
  m_stopped = true;
  runSync(m_callbacks.OnShutdownStarting ? m_callbacks.OnShutdownStarting : Mso::VoidFunctor::DoNothing());

  if (m_callbacks.OnShutdownCompleted) {
    m_dispatchQueue.Post(Mso::Copy(m_callbacks.OnShutdownCompleted));
  }
}

} // namespace Mso::React

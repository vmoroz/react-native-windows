// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#include "pch.h"

#include "Crash.h"
#include "LooperDispatchQueue.h"

namespace winrt::Microsoft::ReactNative {

struct LooperDispatchQueue;

/// The callback submitted to a thread to run LooperDispatchQueue::Invoke()
/// method. This class is responsible for the lifetime of the
/// LooperDispatchQueue associated with it.
class LooperThreadCallback final {
  friend LooperDispatchQueue;

 public:
  LooperThreadCallback(_In_ LooperDispatchQueue *looperQueue) noexcept;

  LooperThreadCallback(LooperThreadCallback const &) = delete;
  LooperThreadCallback &operator=(LooperThreadCallback const &) = delete;

  ~LooperThreadCallback() noexcept;

  void OnDestroy() noexcept;
  void WakeUpLoop() noexcept;

 private:
  static void RunInThread(LooperThreadCallback *self) noexcept;
  void RunLoop() noexcept;
  void ShutdownLooperQueue() noexcept;
  void DestroyLooperQueue(
      std::unique_ptr<LooperDispatchQueue> looperQueue) noexcept;

 private:
  // The state of the looper thread.
  // It starts with the Initial state. From this state it can move to
  // Processing, Canceling, or Destroyed states.
  // **Processing** state is when we process callbacks in the thread. From here
  // we can only move to Destroyed state.
  // **Canceling** state is when we are canceling callbacks in the queue because
  // they cannot be executed.
  //    From Canceling we can move to Canceled or Destroyed states.
  // **Canceled** state is when we finished canceling callbacks. From here we
  // can only move to Destroyed state.
  // **Destroy** state is the final state when we should destroy the looper
  // queue. We cannot move to any other state from it.
  //    Being in Destroy state does not mean that looper dispatch queue is
  //    already destroyed, but we rather expect that it should happen as soon as
  //    any current work is done, e.g. when we finish RunLoop or queue shutdown.
  enum class State {
    Initial,
    Processing,
    Canceling,
    Canceled,
    Destroy,
  };

 private:
  LooperDispatchQueue *m_looperQueue;
  std::atomic<State> m_state{State::Initial};
  ::Microsoft::ReactNative::SimpleEvent m_wakeUpEvent;
};

LooperThreadCallback::LooperThreadCallback(
    _In_ LooperDispatchQueue *looperQueue) noexcept
    : m_looperQueue(looperQueue) {
  VerifyElseCrash(m_looperQueue);
}

LooperThreadCallback::~LooperThreadCallback() noexcept {
  VerifyElseCrash(m_state == State::Destroy);
  VerifyElseCrash(m_looperQueue == nullptr);
}

// Set the queue to the final destroyed state.
// If previous state was Processing then let the RunLoop() finish and it will
// destroy the LooperDispatchQueue after that. If previous state was Canceling
// then let the ShutdownLooperQueue finish shutdown and it will destroy the
// LooperDispatchQueue after that. Otherwise destroy the LooperDispatchQueue
// instance immediately.
void LooperThreadCallback::OnDestroy() noexcept {
  State previousState = m_state.exchange(State::Destroy);
  switch (previousState) {
    case State::Processing:
      WakeUpLoop(); // To exit the RunLoop() and destroy queue after that
      break;

    case State::Canceling:
      // Do nothing: ShutdownLooperQueue() will destroy the LooperDispatchQueue
      // instance when tries to move to Canceled state.
      break;

    case State::Initial:
    case State::Canceled:
      DestroyLooperQueue();
      break;

    default:
      VerifyElseCrash(false);
  }
}

void LooperThreadCallback::WakeUpLoop() noexcept {
  m_wakeUpEvent.Set();
}

void LooperThreadCallback::RunInThread(LooperThreadCallback *self) noexcept {
  self->RunLoop();
}

void LooperThreadCallback::RunLoop() noexcept {
  // Start the RunLoop unless the LooperDispatchQueue is already destroyed.
  State previousState = State::Initial; // Only move to Processing state if
                                        // previous state was Initial.
  if (!m_state.compare_exchange_strong(previousState, State::Processing)) {
    VerifyElseCrash(previousState == State::Destroy);
  }

  for (;;) {
    if (m_state.load(std::memory_order_acquire) == State::Destroy) {
      DestroyLooperQueue();
      break;
    }

    if (!m_looperQueue->Invoke()) {
      // The queue finished processing callbacks. Sleep until new callbacks
      // arrive.
      m_wakeUpEvent.Wait();
    }
  }
}

void LooperThreadCallback::ShutdownLooperQueue() noexcept {
  m_looperQueue->Shutdown();

  State previousState = State::Canceling; // Only move to Canceled state if
                                          // previous state was Canceling.
  if (!m_state.compare_exchange_strong(previousState, State::Canceled)) {
    // We could not move to the Canceled state. The only valid reason can be if
    // we are already in Destroy state.
    VerifyElseCrash(previousState == State::Destroy);
    DestroyLooperQueue();
  }
}

void LooperThreadCallback::DestroyLooperQueue(
    std::unique_ptr<LooperDispatchQueue> looperQueue) noexcept {
  m_looperQueue = nullptr;
  looperQueue.reset();
}

LooperDispatchQueue::LooperDispatchQueue() noexcept {
  // Don't wrap 'this' in a smart pointer (no need to add-ref since to avoid ref-count loop)
  m_threadCallback = std::make_unique<LooperThreadCallback>(this);

  std::thread t{LooperThreadCallback::RunInThread, m_threadCallback.get()};
  m_threadId = t.get_id();
  t.detach();
}

void LooperDispatchQueue::InitializeThis(
    _In_ IDispatchQueue &concurrentQueue,
    const char *threadName) noexcept {
  Super::InitializeThreadName(threadName);
  // Don't wrap 'this' in a smart pointer (no need to add-ref since because we
  // use m_disposeCount to control lifetime)
  m_threadPoolCallback = Mso::Make<LooperThreadPoolCallback>(this, threadName);
  concurrentQueue.Post(Mso::VoidFunctor(m_threadPoolCallback));
}

LooperDispatchQueue::~LooperDispatchQueue() noexcept {
  Super::ShutdownIdleThrottlerMixin();
}

void LooperDispatchQueue::InternalPost() noexcept {
  m_threadPoolCallback->WakeUpLoop();
}

void LooperDispatchQueue::InternalPostIdle() noexcept {
  InternalPost();
}

void LooperDispatchQueue::OnShutdown() noexcept {
  // While we let all callbacks with normal priority finish, we cannot do it for
  // the idle callbacks because they can be throttled and after this method they
  // will never be resumed because we unsubscribed from events. As a result the
  // queue instance will be never freed. We force shutdown of all idle items
  // which could be throttled.
  if (Super::ShutdownIdleOnly()) {
    InternalPost();
  }
}

// This method cancels all current callbacks in the queue, and stops any
// callbacks added to the queue.
void LooperDispatchQueue::Shutdown() noexcept {
  Super::Shutdown();
  Super::ShutdownIdleThrottlerMixin();
}

// This function runs the main thread loop
bool LooperDispatchQueue::Invoke() noexcept {
  EventWriteDQLooperQueueInvokeStart(this);

  bool shouldContinueInvoke = Super::InvokeCore(
      InvokeTaskPriority::Both, std::chrono::milliseconds::max());

  EventWriteDQLooperQueueInvokeEnd(this);
  return shouldContinueInvoke;
}

LIBLET_PUBLICAPI Mso::TCntPtr<IDispatchQueue> CreateLooperScheduler(
    _In_opt_ IDispatchQueue *concurrentQueue) noexcept {
  return Mso::Swarm::Make<LooperDispatchQueue, IDispatchQueue>(
      concurrentQueue == nullptr ? *Mso::Make<ThreadPoolQueue>()
                                 : *concurrentQueue);
}

LIBLET_PUBLICAPI Mso::TCntPtr<IDispatchQueue> MakeLooperDispatchQueue(
    const char *threadName) noexcept {
  return Mso::Swarm::Make<LooperDispatchQueue, IDispatchQueue>(
      *Mso::Make<ThreadPoolQueue>(), threadName);
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

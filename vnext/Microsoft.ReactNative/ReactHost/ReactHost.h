#pragma once

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#include <functional>
#include "winrt/Microsoft.ReactNative.Bridge.h"
#include "Threading/WorkerMessageQueueThread.h"
#include <ppltasks.h>

namespace Microsoft::ReactNative {

class ReactViewHost;
class ReactHost;

//! The async action is a Functor that returns void Future.
using AsyncAction = std::function<concurrency::task<void>()>;
#if 0
//! The queue that executes actions in the sequential order.
//! Each action returns Mso::Future<void> to indicate its completion.
//! The next action does not start until the previous action is completed.
struct AsyncActionQueue final : std::enable_shared_from_this<AsyncActionQueue> {

  //! Creates a new AsyncActionQueue that is based on the provided sequential
  //! queue.
  AsyncActionQueue(std::shared_ptr<react::uwp::WorkerMessageQueueThread>&& queue) noexcept;

  AsyncActionQueue() = delete;
  AsyncActionQueue(const AsyncActionQueue&) = delete;
  AsyncActionQueue& operator=(const AsyncActionQueue &) = delete;

  //! Posts a new action to the AsyncActionQueue and returns Future indicating
  //! when it is completed.
  concurrency::task<void> PostAction(
      AsyncAction &&action) noexcept;

  //! Posts a new action to the AsyncActionQueue and returns Future indicating
  //! when the last one is completed. For the empty list it returns succeeded
  //! Future immediately.
  concurrency::task<void> PostActions(
      std::initializer_list<AsyncAction> actions) noexcept;

  //! Returns the queue associated with the AsyncActionQueue.
  const std::shared_ptr<react::uwp::WorkerMessageQueueThread> Queue() noexcept;

 private:
  struct Entry {
    AsyncAction Action;
    concurrency::task_completion_event<void> Result;
  };

 private:
  //! Invokes action in the m_queue and observes its result.
  void InvokeAction(Entry &&entry) noexcept;

  //! Completes the action, and then starts the next one.
  void CompleteAction(Entry &&entry, std::exception_ptr &&result) noexcept;

 private:
  const std::shared_ptr<react::uwp::WorkerMessageQueueThread> m_queue;
  std::vector<Entry> m_actions;
  bool m_isInvoking{false};
  //const Mso::InvokeElsePostExecutor m_executor{m_queue.Get()};
};
#endif
//! ReactHost manages lifetime of ReactNative instance.
//! It is associated with a native queue that is used modify its state.
//! - It can reload or unload react instance using Reload and Unload.
//! - ReactInstance() returns current react native instance.
//! - NativeQueue() returns the associate native queue.
//! - RekaContext() returns Reka context proxy that can be used before Reka is
//! initialized.
//! - Options() - returns options used for creating ReactInstance.
//!
//! About the RNH closing sequence:
//! - Every time a new RNH is created it is added to the global registry.
//! - During registration we associate Mso::Future with each RNH
//! - The Mso::Future deletes the RNH from the registry.
//! - When the Liblet::Uninit starts, it turns the registry into the uninited
//! state.
//! - In the uninited state new RNH cannot be registered. They are created as
//! already closed.
//! - All operations in closed RNH are canceled.
//! - All registered RNHs are closed. No new work can be done.
//! - We wait in Liblet::Uninit when all pending Futures to indicate closed RNH
//! state are completed.
#if 0
class ReactHost final : public Mso::ActiveObject<IReactHost> {
  using Super = ActiveObjectType;

 public: // IReactHost
  ReactOptions Options() const noexcept override;
  Mso::TCntPtr<IReactInstance> Instance() const noexcept override;
  Mso::JSHost::IRekaContext &RekaContext() noexcept override;
  Mso::Async::IDispatchQueue &NativeQueue() const noexcept override;
  Mso::Future<void> ReloadInstance() noexcept override;
  Mso::Future<void> ReloadInstanceWithOptions(
      ReactOptions &&options) noexcept override;
  Mso::Future<void> UnloadInstance() noexcept override;
  Mso::TCntPtr<IReactViewHost> MakeViewHost(
      ReactViewOptions &&options) noexcept override;
  Mso::Future<std::vector<Mso::TCntPtr<IReactViewHost>>>
  GetViewHostList() noexcept override;

 public:
  void AttachViewHost(ReactViewHost &viewHost) noexcept;
  void DetachViewHost(ReactViewHost &viewHost) noexcept;

  Mso::TCntPtr<AsyncActionQueue> ActionQueue() const noexcept;

  Mso::Future<void> LoadInQueue(ReactOptions &&options) noexcept;
  Mso::Future<void> UnloadInQueue(size_t unloadActionId) noexcept;

  void Close() noexcept;
  bool IsClosed() const noexcept;

  size_t PendingUnloadActionId() const noexcept;
  bool IsInstanceLoaded() const noexcept;

  template <class TCallback>
  Mso::Future<void> PostInQueue(TCallback &&callback) noexcept;

 private:
  friend MakePolicy;
  ReactHost(
      ReactOptions &&options,
      Mso::Promise<void> &&onInstanceLoaded) noexcept;
  ~ReactHost() noexcept override;
  void Initialize() noexcept override;
  void Finalize() noexcept override;

  static Mso::TCntPtr<Mso::Async::IDispatchQueue> GetNativeQueue(
      const ReactOptions &options) noexcept;

  void ForEachViewHost(
      const Mso::FunctorRef<void(ReactViewHost &)> &action) noexcept;

  AsyncAction MakeLoadInstanceAction(ReactOptions &&options) noexcept;
  AsyncAction MakeUnloadInstanceAction() noexcept;

 private:
  mutable std::mutex m_mutex;
  const Mso::InvokeElsePostExecutor m_executor{&Queue()};
  const Mso::ActiveField<Mso::Promise<void>> m_onInstanceLoaded{Queue()};
  const Mso::ActiveReadableField<Mso::TCntPtr<AsyncActionQueue>> m_actionQueue{
      Mso::Make<AsyncActionQueue>(Queue()),
      Queue(),
      m_mutex};
  const Mso::TCntPtr<Mso::JSHost::IRekaContextProxy> m_rekaContextProxy;
  const Mso::ActiveField<Mso::ErrorCode> m_lastError{Queue()};
  const Mso::ActiveReadableField<ReactOptions> m_options{Queue(), m_mutex};
  const Mso::ActiveField<
      std::unordered_map<uintptr_t, Mso::WeakPtr<ReactViewHost>>>
      m_viewHosts{Queue()};
  const Mso::ActiveReadableField<Mso::TCntPtr<IReactInstanceInternal>>
      m_reactInstance{Queue(), m_mutex};
  const Mso::ActiveReadableField<Mso::Promise<void>> m_notifyWhenClosed{
      nullptr,
      Queue(),
      m_mutex};
  const Mso::ActiveField<size_t> m_pendingUnloadActionId{0, Queue()};
  const Mso::ActiveField<size_t> m_nextUnloadActionId{0, Queue()};
  const Mso::ActiveField<bool> m_isInstanceLoaded{false, Queue()};
};
#endif
#if 0
//! Implements a cross-platform host for a React view
class ReactViewHost final : public ActiveObject<IReactViewHost> {
  using Super = ActiveObjectType;

 public: // IReactViewHost
  ReactViewOptions Options() const noexcept override;
  IReactHost &ReactHost() const noexcept override;
  Mso::Future<void> ReloadViewInstance() noexcept override;
  Mso::Future<void> ReloadViewInstanceWithOptions(
      ReactViewOptions &&options) noexcept override;
  Mso::Future<void> UnloadViewInstance() noexcept override;
  Mso::Future<void> AttachViewInstance(
      IReactViewInstance &viewInstance) noexcept override;
  Mso::Future<void> DetachViewInstance() noexcept override;

 public:
  Mso::Future<void> LoadViewInstanceInQueue() noexcept;
  Mso::Future<void> LoadViewInstanceInQueue(
      ReactViewOptions &&options) noexcept;
  Mso::Future<void> UnloadViewInstanceInQueue(size_t unloadActionId) noexcept;

 private:
  friend MakePolicy;
  ReactViewHost(
      Mso::React::ReactHost &reactHost,
      ReactViewOptions &&options) noexcept;
  ~ReactViewHost() noexcept override;

  void SetOptions(ReactViewOptions &&options) noexcept;

  AsyncAction MakeLoadViewInstanceAction() noexcept;
  AsyncAction MakeLoadViewInstanceAction(ReactViewOptions &&options) noexcept;
  AsyncAction MakeUnloadViewInstanceAction() noexcept;

 private:
  mutable std::mutex m_mutex;
  const Mso::InvokeElsePostExecutor m_executor{&Queue()};
  const Mso::ActiveField<Mso::TCntPtr<AsyncActionQueue>> m_actionQueue{Queue()};
  const Mso::TCntPtr<Mso::React::ReactHost> m_reactHost;
  const Mso::ActiveReadableField<ReactViewOptions> m_options{Queue(), m_mutex};
  const Mso::ActiveField<Mso::TCntPtr<IReactViewInstance>> m_viewInstance{
      Queue()};
  const Mso::ActiveField<size_t> m_pendingUnloadActionId{0, Queue()};
  const Mso::ActiveField<size_t> m_nextUnloadActionId{0, Queue()};
  const Mso::ActiveField<bool> m_isViewInstanceLoaded{false, Queue()};
};

//! ReactHostRegistry helps with closing of all ReactHosts on Liblet::Uninit.
class ReactHostRegistry final
    : public Mso::RefCountedObjectNoVTable<ReactHostRegistry> {
 public:
  ReactHostRegistry() = default;
  DECLARE_COPYCONSTR_AND_ASSIGNMENT(ReactHostRegistry);

  ~ReactHostRegistry() noexcept;

  // Adds ReactHost to the global registry and returns promise to be set when
  // closed. It returns an empty Mso::Promise in case if RNH cannot be
  // registered.
  static Mso::Promise<void> Register(ReactHost &reactHost) noexcept;

  static void OnLibletInit() noexcept;
  static void OnLibletUninit() noexcept;

 private:
  struct Entry {
    Mso::WeakPtr<ReactHost> WeakReactHost;
    Mso::Future<void> WhenReactHostClosed;
  };

 private:
  // Removes the ReactHost from the global registry
  void Unregister(uintptr_t reactHostKey) noexcept;

 private:
  static std::mutex s_mutex; // protects access to s_registry and m_entries.
  static Mso::TCntPtr<ReactHostRegistry> s_registry;

 private:
  // m_entries is an instance field to ensure that we have correct behavior
  // in case when tests run multiple Liblet Init/Uninit.
  // The instance field ensures that all Unregister calls use the correct set on
  // entries.
  std::unordered_map<uintptr_t, Entry> m_entries;
};

//=============================================================================================
// ReactHost inline implementation
//=============================================================================================

template <class TCallback>
Mso::Future<void> ReactHost::PostInQueue(TCallback &&callback) noexcept {
  using Callback = std::decay_t<TCallback>;
  return Mso::PostFuture(
      m_executor, [
        weakThis = Mso::MakeWeakPtr(this),
        callback = Callback{std::forward<TCallback>(callback)}
      ]() mutable noexcept {
        if (auto strongThis = weakThis.GetStrongPtr()) {
          return callback();
        }

        return Mso::MakeCanceledFuture();
      });
}
#endif
}

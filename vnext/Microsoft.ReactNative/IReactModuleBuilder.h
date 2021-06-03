#pragma once
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReactModuleBuilder.g.h"
#include "ABICxxModule.h"

namespace winrt::Microsoft::ReactNative::implementation {

struct ReactModuleBuilder : ReactModuleBuilderT<ReactModuleBuilder> {
  ReactModuleBuilder(IReactContext const &reactContext, IReactPropertyName const &dispatcherName) noexcept;

 public: // IReactModuleBuilder
  void AddInitializer(InitializerDelegate const &initializer) noexcept;
  void AddConstantProvider(ConstantProviderDelegate const &constantProvider) noexcept;
  void AddMethod(hstring const &name, MethodReturnType const &returnType, MethodDelegate const &method) noexcept;
  void AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept;
  void AddDispatchedInitializer(
      InitializerDelegate const &initializer,
      ReactInitializerType const &initializerType,
      bool useJSDispatcher) noexcept;
  void AddDispatchedFinalizer(FinalizerDelegate const &finalizer, bool useJSDispatcher) noexcept;
  void AddDispatchedConstantProvider(ConstantProviderDelegate const &constantProvider, bool useJSDispatcher) noexcept;
  void AddDispatchedMethod(
      hstring const &name,
      MethodReturnType const &returnType,
      MethodDelegate const &method,
      bool useJSDispatcher) noexcept;
  void AddDispatchedSyncMethod(hstring const &name, SyncMethodDelegate const &method, bool useJSDispatcher) noexcept;

 public:
  std::unique_ptr<facebook::xplat::module::CxxModule> MakeCxxModule(
      std::string const &name,
      IInspectable const &nativeModule) noexcept;

 private:
  static MethodResultCallback MakeMethodResultCallback(
      facebook::xplat::module::CxxModule::Callback &&callback) noexcept;
  static void RunSync(IReactDispatcher const &dispatcher, ReactDispatcherCallback const &callback) noexcept;
  void InitializeModule() noexcept;
  ABICxxModule::ConstantProvider GetConstantProvider() noexcept;

 private:
  struct InitializerEntry {
    InitializerDelegate Delegate;
    ReactInitializerType InitializerType;
    bool UseJSDispatcher{};
  };

  struct FinalizerEntry {
    FinalizerDelegate Delegate;
    bool UseJSDispatcher{};
  };

  struct ConstantProviderEntry {
    ConstantProviderDelegate Delegate;
    bool UseJSDispatcher{};
  };

 private:
  IReactContext m_reactContext{nullptr};
  IReactPropertyName m_dispatcherName{nullptr};
  IReactDispatcher m_moduleDispatcher{nullptr};
  IReactDispatcher m_jsDispatcher{nullptr};

  std::vector<InitializerEntry> m_initializers;
  std::vector<FinalizerEntry> m_finalizers;
  std::vector<ConstantProviderEntry> m_constantProviders;
  std::vector<facebook::xplat::module::CxxModule::Method> m_methods;
};

} // namespace winrt::Microsoft::ReactNative::implementation

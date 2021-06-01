#pragma once
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReactModuleBuilder.g.h"
#include "ABICxxModule.h"

namespace winrt::Microsoft::ReactNative::implementation {

struct ReactModuleBuilder : ReactModuleBuilderT<ReactModuleBuilder> {
  ReactModuleBuilder(IReactContext const &reactContext, IReactPropertyName const& dispatcherName) noexcept;

 public: // IReactModuleBuilder
  void AddInitializer(InitializerDelegate const &initializer) noexcept;
  void AddConstantProvider(ConstantProviderDelegate const &constantProvider) noexcept;
  void AddMethod(hstring const &name, MethodReturnType const &returnType, MethodDelegate const &method) noexcept;
  void AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept;
  void AddDispatchedInitializer(
      InitializerDelegate const &initializer,
      ReactInitializerPriority const &priority,
      IReactPropertyName const &dispatcherName) noexcept;
  void AddDispatchedFinalizer(
      FinalizerDelegate const &finalizer,
      IReactPropertyName const &dispatcherName) noexcept;
  void AddDispatchedConstantProvider(
      ConstantProviderDelegate const &constantProvider,
      IReactPropertyName const &dispatcherName) noexcept;
  void AddDispatchedMethod(
      hstring const &name,
      MethodReturnType const &returnType,
      MethodDelegate const &method,
      IReactPropertyName const &dispatcherName) noexcept;
  void AddDispatchedSyncMethod(
      hstring const &name,
      SyncMethodDelegate const &method,
      IReactPropertyName const &dispatcherName) noexcept;

 public:
  std::unique_ptr<facebook::xplat::module::CxxModule> MakeCxxModule(
      std::string const &name,
      IInspectable const &nativeModule) noexcept;

 private:
  static MethodResultCallback MakeMethodResultCallback(
      facebook::xplat::module::CxxModule::Callback &&callback) noexcept;

 private:
  struct InitializerEntry {
    InitializerDelegate Delegate;
    ReactInitializerPriority Priority;
    IReactPropertyName DispatcherName;
  };

  struct FinalizerEntry {
    ReactDispatcherCallback Delegate;
    IReactPropertyName DispatcherName;
  };

  struct ConstantProviderEntry {
    ConstantProviderDelegate Delegate;
    IReactPropertyName DispatcherName;
  };

 private:
  IReactContext m_reactContext;
  IReactPropertyName m_dispatcherName;
  std::vector<InitializerEntry> m_initializers;
  std::vector<FinalizerEntry> m_finalizers;
  std::vector<ConstantProviderEntry> m_constantProviders;
  std::vector<facebook::xplat::module::CxxModule::Method> m_methods;
};

} // namespace winrt::Microsoft::ReactNative::implementation

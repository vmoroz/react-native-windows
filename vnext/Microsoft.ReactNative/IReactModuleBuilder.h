#pragma once
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ReactModuleBuilder.g.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace winrt::Microsoft::ReactNative::implementation {

struct ReactModuleBuilder : ReactModuleBuilderT<ReactModuleBuilder> {
 public:
  struct Initializer {
    InitializerDelegate Delegate;
    ReactInitializerType InitializerType;
    bool UseJSDispatcher{};
  };

  struct Finalizer {
    FinalizerDelegate Delegate;
    bool UseJSDispatcher{};
  };

  struct ConstantProvider {
    ConstantProviderDelegate Delegate;
    bool UseJSDispatcher{};
  };

  struct Method {
    MethodReturnType ReturnType{};
    MethodDelegate Delegate;
    bool UseJSDispatcher{};
  };

  struct SyncMethod {
    SyncMethodDelegate Delegate;
    bool UseJSDispatcher{};
  };

 public:
  ReactModuleBuilder(IReactContext const reactContext) noexcept;

  std::vector<Initializer> const &GetInitializers() const noexcept;
  std::vector<Finalizer> const &GetFinalizers() const noexcept;
  std::vector<ConstantProvider> const &GetConstantProviders() const noexcept;
  std::unordered_map<std::string, Method> const &GetMethods() const noexcept;
  std::unordered_map<std::string, SyncMethod> const &GetSyncMethods() const noexcept;

 public: // IReactModuleBuilder
  void AddInitializer(InitializerDelegate const &initializer) noexcept;
  void AddConstantProvider(ConstantProviderDelegate const &constantProvider) noexcept;
  void AddMethod(hstring const &name, MethodReturnType const &returnType, MethodDelegate const &method) noexcept;
  void AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept;

 public: // IReactModuleBuilder2
  IReactContext Context() noexcept;
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

 private:
  void EnsureMemberNotSet(std::string const &key, bool checkingMethod) noexcept;

 private:
  IReactContext m_reactContext;
  std::vector<Initializer> m_initializers;
  std::vector<Finalizer> m_finalizers;
  std::vector<ConstantProvider> m_constantProviders;
  std::unordered_map<std::string, Method> m_methods;
  std::unordered_map<std::string, SyncMethod> m_syncMethods;
};

} // namespace winrt::Microsoft::ReactNative::implementation

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "IReactModuleBuilder.h"
#include "ReactModuleBuilder.g.cpp"

namespace winrt::Microsoft::ReactNative::implementation {

//===========================================================================
// ReactModuleBuilder implementation
//===========================================================================

ReactModuleBuilder::ReactModuleBuilder() noexcept {}

std::vector<ReactModuleBuilder::Initializer> const &ReactModuleBuilder::GetInitializers() const noexcept {
  return m_initializers;
}

std::vector<ReactModuleBuilder::Finalizer> const &ReactModuleBuilder::GetFinalizers() const noexcept {
  return m_finalizers;
}

std::vector<ReactModuleBuilder::ConstantProvider> ReactModuleBuilder::GetConstantProviders() const noexcept {
  return m_constantProviders;
}

std::unordered_map<std::string, ReactModuleBuilder::Method> ReactModuleBuilder::GetMethods() const noexcept {
  return m_methods;
}

std::unordered_map<std::string, ReactModuleBuilder::SyncMethod> ReactModuleBuilder::GetSyncMethods() const noexcept {
  return m_syncMethods;
}

void ReactModuleBuilder::AddInitializer(InitializerDelegate const &initializer) noexcept {
  AddDispatchedInitializer(initializer, ReactInitializerType::Method, true);
}

void ReactModuleBuilder::AddConstantProvider(ConstantProviderDelegate const &constantProvider) noexcept {
  AddDispatchedConstantProvider(constantProvider, true);
}

void ReactModuleBuilder::AddMethod(
    hstring const &name,
    MethodReturnType const &returnType,
    MethodDelegate const &method) noexcept {
  AddDispatchedMethod(name, returnType, method, false);
}

void ReactModuleBuilder::AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept {
  AddDispatchedSyncMethod(name, method, true);
}

void ReactModuleBuilder::AddDispatchedInitializer(
    InitializerDelegate const &initializer,
    ReactInitializerType const &initializerType,
    bool useJSDispatcher) noexcept {
  m_initializers.push_back(Initializer{initializer, initializerType, useJSDispatcher});
}

void ReactModuleBuilder::AddDispatchedFinalizer(FinalizerDelegate const &finalizer, bool useJSDispatcher) noexcept {
  m_finalizers.push_back(Finalizer{finalizer, useJSDispatcher});
}

void ReactModuleBuilder::AddDispatchedConstantProvider(
    ConstantProviderDelegate const &constantProvider,
    bool useJSDispatcher) noexcept {
  EnsureMemberNotSet("getConstants", false);
  m_constantProviders.push_back(ConstantProvider{constantProvider, useJSDispatcher});
}

void ReactModuleBuilder::AddDispatchedMethod(
    hstring const &name,
    MethodReturnType const &returnType,
    MethodDelegate const &method,
    bool useJSDispatcher) noexcept {
  auto methodName = to_string(name);
  EnsureMemberNotSet(methodName, true);
  m_methods.try_emplace(std::move(methodName), Method{returnType, method, useJSDispatcher});
}

void ReactModuleBuilder::AddDispatchedSyncMethod(
    hstring const &name,
    SyncMethodDelegate const &method,
    bool useJSDispatcher) noexcept {
  auto methodName = to_string(name);
  EnsureMemberNotSet(methodName, true);
  m_syncMethods.try_emplace(std::move(methodName), SyncMethod{method, useJSDispatcher});
}

void ReactModuleBuilder::EnsureMemberNotSet(const std::string &key, bool checkingMethod) noexcept {
  VerifyElseCrash(m_methods.find(key) == m_methods.end());
  VerifyElseCrash(m_syncMethods.find(key) == m_syncMethods.end());
  if (checkingMethod && key == "getConstants") {
    VerifyElseCrash(m_constantProviders.size() == 0);
  }
}

} // namespace winrt::Microsoft::ReactNative::implementation

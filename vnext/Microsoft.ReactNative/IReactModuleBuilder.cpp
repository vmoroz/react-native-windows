// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "IReactModuleBuilder.h"
#include "ReactModuleBuilder.g.cpp"
#include "DynamicWriter.h"
#include "ReactHost/MsoUtils.h"

using namespace facebook::xplat::module;

namespace winrt::Microsoft::ReactNative::implementation {

//===========================================================================
// ReactModuleBuilder implementation
//===========================================================================

ReactModuleBuilder::ReactModuleBuilder(
    IReactContext const &reactContext,
    IReactPropertyName const &dispatcherName) noexcept
    : m_reactContext{reactContext}, m_dispatcherName{dispatcherName} {}

void ReactModuleBuilder::AddInitializer(InitializerDelegate const &initializer) noexcept {
  AddDispatchedInitializer(initializer, ReactInitializerPriority::Normal, nullptr);
}

void ReactModuleBuilder::AddConstantProvider(ConstantProviderDelegate const &constantProvider) noexcept {
  AddDispatchedConstantProvider(constantProvider, nullptr);
}

void ReactModuleBuilder::AddMethod(
    hstring const &name,
    MethodReturnType const &returnType,
    MethodDelegate const &method) noexcept {
  AddDispatchedMethod(name, returnType, method, nullptr);
}

void ReactModuleBuilder::AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept {
  AddDispatchedSyncMethod(name, method, nullptr);
}

void ReactModuleBuilder::AddDispatchedInitializer(
    InitializerDelegate const &initializer,
    ReactInitializerPriority const &priority,
    IReactPropertyName const &dispatcherName) noexcept {
  m_initializers.emplace_back(initializer, priority, dispatcherName);
}

void ReactModuleBuilder::AddDispatchedFinalizer(
    FinalizerDelegate const &finalizer,
    IReactPropertyName const &dispatcherName) noexcept {
  m_finalizers.emplace_back(finalizer, dispatcherName);
}

void ReactModuleBuilder::AddDispatchedConstantProvider(
    ConstantProviderDelegate const &constantProvider,
    IReactPropertyName const &dispatcherName) noexcept {
  m_constantProviders.emplace_back(constantProvider, dispatcherName);
}

void ReactModuleBuilder::AddDispatchedMethod(
    hstring const &name,
    MethodReturnType const &returnType,
    MethodDelegate const &method,
    IReactPropertyName const &dispatcherName) noexcept {
  auto cxxMethodCallback = [method](
                               folly::dynamic args, CxxModule::Callback resolve, CxxModule::Callback reject) noexcept {
    auto argReader = make<DynamicReader>(args);
    auto resultWriter = make<DynamicWriter>();
    auto resolveCallback = MakeMethodResultCallback(std::move(resolve));
    auto rejectCallback = MakeMethodResultCallback(std::move(reject));
    method(argReader, resultWriter, resolveCallback, rejectCallback);
  };

  if (!IsModuleDispatcher(dispatcherName)) {
    [method](folly::dynamic args, CxxModule::Callback resolve, CxxModule::Callback reject) noexcept {
      dispatcher.Post([]() mutable noexcept { cxxMethodCallback(std::move(args), std::move(resolve), std::move(reject))});
    };
  }

  CxxModule::Method cxxMethod(
      to_string(name), cxxMethodCallback
  );

  switch (returnType) {
    case MethodReturnType::Callback:
      cxxMethod.callbacks = 1;
      cxxMethod.isPromise = false;
      break;
    case MethodReturnType::TwoCallbacks:
      cxxMethod.callbacks = 2;
      cxxMethod.isPromise = false;
      break;
    case MethodReturnType::Promise:
      cxxMethod.callbacks = 2;
      cxxMethod.isPromise = true;
      break;
    default:
      cxxMethod.callbacks = 0;
      cxxMethod.isPromise = false;
  }

  m_methods.push_back(std::move(cxxMethod));
}

void ReactModuleBuilder::AddDispatchedSyncMethod(
    hstring const &name,
    SyncMethodDelegate const &method,
    IReactPropertyName const &dispatcherName) noexcept {
  m_methods.emplace_back(
      to_string(name),
      [method](folly::dynamic args) noexcept {
        auto argReader = make<DynamicReader>(args);
        auto resultWriter = make<DynamicWriter>();
        method(argReader, resultWriter);
        return get_self<DynamicWriter>(resultWriter)->TakeValue();
      },
      CxxModule::SyncTag);
}

/*static*/ MethodResultCallback ReactModuleBuilder::MakeMethodResultCallback(CxxModule::Callback &&callback) noexcept {
  if (callback) {
    return [callback = std::move(callback)](const IJSValueWriter &outputWriter) noexcept {
      if (outputWriter) {
        folly::dynamic argArray = outputWriter.as<DynamicWriter>()->TakeValue();
        callback(std::vector<folly::dynamic>(argArray.begin(), argArray.end()));
      } else {
        callback(std::vector<folly::dynamic>{});
      }
    };
  }

  return {};
}

std::unique_ptr<CxxModule> ReactModuleBuilder::MakeCxxModule(
    std::string const &name,
    IInspectable const &nativeModule) noexcept {
  for (auto &initializer : m_initializers) {
    initializer(m_reactContext);
  }
  return std::make_unique<ABICxxModule>(
      nativeModule, Mso::Copy(name), Mso::Copy(m_constantProviders), Mso::Copy(m_methods));
}

} // namespace winrt::Microsoft::ReactNative::implementation

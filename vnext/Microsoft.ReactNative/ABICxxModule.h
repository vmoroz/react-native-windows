// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
// The ABICxxModule implements the CxxModule interface and wraps up the ABI-safe
// NativeModule.
//

#pragma once

#include "IReactModuleBuilder.h"
#include "ReactContext.h"
#include "cxxreact/CxxModule.h"
#include "winrt/Microsoft.ReactNative.h"

namespace winrt::Microsoft::ReactNative {

struct ABICxxModule : facebook::xplat::module::CxxModule {
  using CxxMethod = facebook::xplat::module::CxxModule::Method;
  using ConstantProvider = std::function<std::map<std::string, folly::dynamic>()>;

  ABICxxModule(
      std::string const &name,
      ReactModuleProvider const &moduleProvider,
      Mso::CntPtr<Mso::React::IReactContext> const &reactContext,
      IReactPropertyName const &dispatcherName) noexcept;

 public: // CxxModule implementation
  std::string getName() noexcept override;
  std::map<std::string, folly::dynamic> getConstants() noexcept override;
  std::vector<facebook::xplat::module::CxxModule::Method> getMethods() noexcept override;

 private:
  void RunInitializers(Mso::CntPtr<Mso::React::IReactContext> const &reactContext) const noexcept;
  void SetupFinalizers(Mso::CntPtr<Mso::React::IReactContext> const &reactContext) const noexcept;
  CxxMethod CreateCxxMethod(std::string const &name, implementation::ReactModuleBuilder::Method const &method)
      const noexcept;
  CxxMethod CreateCxxMethod(std::string const &name, implementation::ReactModuleBuilder::SyncMethod const &method)
      const noexcept;
  static void RunSync(IReactDispatcher const &dispatcher, ReactDispatcherCallback const &callback) noexcept;
  static MethodResultCallback MakeMethodResultCallback(CxxModule::Callback &&callback) noexcept;

 private:
  std::string m_name;
  com_ptr<implementation::ReactModuleBuilder> m_moduleBuilder;
  IInspectable m_nativeModule; // to keep the native module alive
  IReactDispatcher m_jsDispatcher;
  IReactDispatcher m_moduleDispatcher;
};

} // namespace winrt::Microsoft::ReactNative

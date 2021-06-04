// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

//
// The ABICxxModule implements the CxxModule interface and wraps up the ABI-safe
// NativeModule.
//

#pragma once

#include "cxxreact/CxxModule.h"
#include "winrt/Microsoft.ReactNative.h"

namespace winrt::Microsoft::ReactNative {

struct ABICxxModule : facebook::xplat::module::CxxModule {
  using ConstantProvider = std::function<std::map<std::string, folly::dynamic>()>;
  using Finalizer = std::function<void()>;

  ABICxxModule(
      IInspectable const &nativeModule,
      std::string &&name,
      Finalizer &&finalizer,
      ConstantProvider &&constantProvider,
      std::vector<facebook::xplat::module::CxxModule::Method> &&methods) noexcept;
  ~ABICxxModule() noexcept;

 public: // CxxModule implementation
  std::string getName() noexcept override;
  std::map<std::string, folly::dynamic> getConstants() noexcept override;
  std::vector<facebook::xplat::module::CxxModule::Method> getMethods() noexcept override;

 private:
  IInspectable m_nativeModule; // just to keep native module alive
  std::string m_name;
  Finalizer m_finalizer;
  ConstantProvider m_constantProvider;
  std::vector<facebook::xplat::module::CxxModule::Method> m_methods;
};

} // namespace winrt::Microsoft::ReactNative

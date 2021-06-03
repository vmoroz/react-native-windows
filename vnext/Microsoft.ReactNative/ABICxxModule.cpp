// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ABICxxModule.h"
#include "DynamicWriter.h"

using namespace facebook::xplat::module;

namespace winrt::Microsoft::ReactNative {

ABICxxModule::ABICxxModule(
    winrt::Windows::Foundation::IInspectable const &nativeModule,
    std::string &&name,
    ConstantProvider &&constantProvider,
    std::vector<facebook::xplat::module::CxxModule::Method> &&methods) noexcept
    : m_nativeModule{nativeModule},
      m_name{std::move(name)},
      m_constantProvider{std::move(constantProvider)},
      m_methods(std::move(methods)) {}

std::string ABICxxModule::getName() noexcept {
  return m_name;
}

std::map<std::string, folly::dynamic> ABICxxModule::getConstants() noexcept {
  return m_constantProvider();
}

std::vector<CxxModule::Method> ABICxxModule::getMethods() noexcept {
  auto result = std::move(m_methods);
  return result;
}

} // namespace winrt::Microsoft::ReactNative

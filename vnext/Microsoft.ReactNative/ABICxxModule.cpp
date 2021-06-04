// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ABICxxModule.h"

using namespace facebook::xplat::module;

namespace winrt::Microsoft::ReactNative {

ABICxxModule::ABICxxModule(
    IInspectable const &nativeModule,
    std::string &&name,
    Finalizer &&finalizer,
    ConstantProvider &&constantProvider,
    std::vector<CxxModule::Method> &&methods) noexcept
    : m_nativeModule{nativeModule},
      m_name{std::move(name)},
      m_finalizer{std::move(finalizer)},
      m_constantProvider{std::move(constantProvider)},
      m_methods(std::move(methods)) {}

ABICxxModule::~ABICxxModule() noexcept {
  m_finalizer();
}

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

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "I18nManagerModule.h"
#include <IReactDispatcher.h>
#include <XamlUtils.h>
#include <utils/Helpers.h>
#include <winrt/Windows.ApplicationModel.Resources.Core.h>
#include <winrt/Windows.Globalization.h>
#include "Unicode.h"

namespace Microsoft::ReactNative {

static const React::ReactPropertyId<bool> &SystemIsRTLPropertyId() noexcept {
  static const React::ReactPropertyId<bool> prop{L"ReactNative.I18n", L"SystemIsRTL"};
  return prop;
}

static const React::ReactPropertyId<bool> &AllowRTLPropertyId() noexcept {
  static const React::ReactPropertyId<bool> prop{L"ReactNative.I18n", L"AllowRTL"};
  return prop;
}

static const React::ReactPropertyId<bool> &ForceRTLPropertyId() noexcept {
  static const React::ReactPropertyId<bool> prop{L"ReactNative.I18n", L"ForceRTL"};
  return prop;
}

I18nManager::I18nManager(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, React::ReactContext context) noexcept
    : facebook::react::NativeI18nManagerCxxSpecJSI{std::move(jsInvoker)}, m_context{std::move(context)} {}

void I18nManager::InitI18nInfo(const winrt::Microsoft::ReactNative::ReactPropertyBag &propertyBag) noexcept {
  if (xaml::TryGetCurrentApplication() && !react::uwp::IsXamlIsland()) {
    // TODO: Figure out packaged win32 app story for WinUI 3
    auto layoutDirection = winrt::Windows::ApplicationModel::Resources::Core::ResourceContext()
                               .GetForCurrentView()
                               .QualifierValues()
                               .Lookup(L"LayoutDirection");

    propertyBag.Set(SystemIsRTLPropertyId(), layoutDirection != L"LTR");
  }
}

/*static*/ bool I18nManager::IsRTL(const React::ReactPropertyBag &propertyBag) noexcept {
  if (propertyBag.Get(ForceRTLPropertyId()).value_or(false))
    return true;

  if (!propertyBag.Get(AllowRTLPropertyId()).value_or(false))
    return false;

  return propertyBag.Get(SystemIsRTLPropertyId()).value_or(false);
}

void I18nManager::allowRTL(facebook::jsi::Runtime &rt, bool allowRTL) {
  m_context.Properties().Set(AllowRTLPropertyId(), allowRTL);
}

void I18nManager::forceRTL(facebook::jsi::Runtime &rt, bool forceRTL) {
  m_context.Properties().Set(ForceRTLPropertyId(), forceRTL);
}

void I18nManager::swapLeftAndRightInRTL(facebook::jsi::Runtime & /*rt*/, bool /*flipStyles*/) {
  // TODO - https://github.com/microsoft/react-native-windows/issues/4662
}

facebook::jsi::Object I18nManager::getConstants(facebook::jsi::Runtime &rt) {
  std::string locale = "en-us";

  auto langs = winrt::Windows::Globalization::ApplicationLanguages::Languages();
  if (langs.Size() > 0) {
    locale = Microsoft::Common::Unicode::Utf16ToUtf8(langs.GetAt(0));
  }

  auto constants = facebook::jsi::Object{rt};
  constants.setProperty(rt, "localeIdentifier", locale);
  constants.setProperty(rt, "doLeftAndRightSwapInRTL", false);
  constants.setProperty(rt, "isRTL", IsRTL(m_context.Properties()));
  return constants;
}

} // namespace Microsoft::ReactNative

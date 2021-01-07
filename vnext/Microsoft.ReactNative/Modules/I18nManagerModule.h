// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <NativeModules.h>
#include <react/modules/FBReactNativeSpec/NativeModules.h>
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Display.h>

namespace Microsoft::ReactNative {

// Since I18nModule provides constants that require the UI thread, we need to initialize those constants before module
// creation (module creation can happen on background thread) InitI18nInfo will store the required information in the
// PropertyBag so that the I18nModule can return the constants synchronously.
struct I18nManager : facebook::react::NativeI18nManagerCxxSpecJSI {
  I18nManager(std::shared_ptr<facebook::react::CallInvoker> jsInvoker, React::ReactContext context) noexcept;

  static void InitI18nInfo(const React::ReactPropertyBag &propertyBag) noexcept;
  static bool IsRTL(const React::ReactPropertyBag &propertyBag) noexcept;

  facebook::jsi::Object getConstants(facebook::jsi::Runtime &rt) override;
  void allowRTL(facebook::jsi::Runtime &rt, bool allowRTL) override;
  void forceRTL(facebook::jsi::Runtime &rt, bool forceRTL) override;
  void swapLeftAndRightInRTL(facebook::jsi::Runtime &rt, bool flipStyles) override;

 private:
  React::ReactContext m_context;
};

} // namespace Microsoft::ReactNative

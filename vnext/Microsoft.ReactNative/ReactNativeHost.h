// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "ReactNativeHost.g.h"

#include "ReactInstanceManager.h"
#include "ReactInstanceSettings.h"
#include "ReactRootView2.h"

#include <ReactUWP/IReactInstance.h>
#include <ReactUWP/IXamlRootView.h>
#include <ReactUWP/ReactUwp.h>

#ifndef REACT_DEFAULT_ENABLE_DEVELOPER_MENU
#if _DEBUG
#define REACT_DEFAULT_ENABLE_DEVELOPER_MENU true
#else
#define REACT_DEFAULT_ENABLE_DEVELOPER_MENU false
#endif // _DEBUG
#endif // REACT_DEFAULT_ENABLE_DEVELOPER_MENU

namespace winrt::Microsoft::ReactNative::implementation {

struct ReactNativeHost : ReactNativeHostT<ReactNativeHost> {
  ReactNativeHost() noexcept;

  UIElement GetOrCreateRootView(IInspectable initialProps) noexcept;

  ReactNative::ReactInstanceManager ReactInstanceManager() noexcept;
  ReactNative::ReactInstanceSettings InstanceSettings() noexcept;
  void InstanceSettings(ReactNative::ReactInstanceSettings const &value) noexcept;
  bool HasInstance() noexcept;
  Windows::Foundation::Collections::IVector<IReactPackageProvider> PackageProviders() noexcept;
  void PackageProviders(Windows::Foundation::Collections::IVector<IReactPackageProvider> const &value) noexcept;

  void OnSuspend() noexcept;
  void OnEnteredBackground() noexcept;
  void OnLeavingBackground() noexcept;
  void OnResume(OnResumeAction const &action) noexcept;

 private:
  void Init() noexcept;
  ReactNative::ReactInstanceManager CreateReactInstanceManager() noexcept;
  std::shared_ptr<ReactRootView> CreateRootView() noexcept;

 private:
  ReactNative::ReactInstanceSettings m_instanceSettings;
  ReactNative::ReactInstanceManager m_reactInstanceManager{nullptr};
  Windows::Foundation::Collections::IVector<IReactPackageProvider> m_packageProviders{nullptr};

  std::shared_ptr<ReactRootView> m_reactRootView{nullptr};
};

} // namespace winrt::Microsoft::ReactNative::implementation

namespace winrt::Microsoft::ReactNative::factory_implementation {

struct ReactNativeHost : ReactNativeHostT<ReactNativeHost, implementation::ReactNativeHost> {};

} // namespace winrt::Microsoft::ReactNative::factory_implementation

namespace winrt::Microsoft::ReactNative::implementation {

//=============================================================================
// ReactNativeHost inline implementation
//=============================================================================

inline void ReactNativeHost::InstanceSettings(ReactNative::ReactInstanceSettings const &value) noexcept {
  m_instanceSettings = value;
}

inline bool ReactNativeHost::HasInstance() noexcept {
  return m_reactInstanceManager != nullptr;
}

inline void ReactNativeHost::PackageProviders(
    Windows::Foundation::Collections::IVector<IReactPackageProvider> const &value) noexcept {
  m_packageProviders = value;
}

} // namespace winrt::Microsoft::ReactNative::implementation

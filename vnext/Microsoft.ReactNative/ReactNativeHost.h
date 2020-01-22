// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.
#pragma once

#include "ReactNativeHost.g.h"

#include "NativeModulesProvider.h"
#include "ReactInstanceSettings.h"
#include "ReactRootView2.h"
#include "ViewManagersProvider.h"

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

  ReactNative::IReactContext CurrentReactContext() noexcept;
  ReactNative::ReactInstanceSettings InstanceSettings() noexcept;
  void InstanceSettings(ReactNative::ReactInstanceSettings const &value) noexcept;
  Windows::Foundation::Collections::IVector<IReactPackageProvider> PackageProviders() noexcept;
  void PackageProviders(Windows::Foundation::Collections::IVector<IReactPackageProvider> const &value) noexcept;

  void OnSuspend() noexcept;
  void OnEnteredBackground() noexcept;
  void OnLeavingBackground() noexcept;
  void OnResume(OnResumeAction const &action) noexcept;

  void OnBackPressed();

  IAsyncOperation<IReactContext> GetOrCreateReactContextAsync() noexcept;

  std::shared_ptr<react::uwp::IReactInstanceCreator> InstanceCreator() noexcept;
  std::shared_ptr<react::uwp::IReactInstance> Instance() noexcept;

 private:
  void Init() noexcept;
  IAsyncOperation<IReactContext> CreateReactContextCoreAsync() noexcept;

 private:
  ReactNative::ReactInstanceSettings m_instanceSettings;
  Windows::Foundation::Collections::IVector<IReactPackageProvider> m_packageProviders{nullptr};

  IReactContext m_currentReactContext{nullptr};
  std::shared_ptr<NativeModulesProvider> m_modulesProvider{nullptr};
  std::shared_ptr<ViewManagersProvider> m_viewManagersProvider{nullptr};
  IReactPackageBuilder m_packageBuilder;

  //	There should be one react instance creator per instance, as it
  //	both holds the current instance and is responsible for creating new
  //	instances on live reload.
  std::shared_ptr<react::uwp::IReactInstanceCreator> m_reactInstanceCreator{nullptr};
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

inline void ReactNativeHost::PackageProviders(
    Windows::Foundation::Collections::IVector<IReactPackageProvider> const &value) noexcept {
  m_packageProviders = value;
}

inline ReactNative::IReactContext ReactNativeHost::CurrentReactContext() noexcept {
  return m_currentReactContext;
}

inline std::shared_ptr<react::uwp::IReactInstance> ReactNativeHost::Instance() noexcept {
  return InstanceCreator()->getInstance();
}

} // namespace winrt::Microsoft::ReactNative::implementation

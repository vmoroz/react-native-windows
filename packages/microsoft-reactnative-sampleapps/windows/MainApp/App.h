#pragma once

#include "App.xaml.g.h"

namespace activation = winrt::Windows::ApplicationModel::Activation;

namespace winrt::MainApp::implementation {

struct App : AppT<App> {
  App() noexcept;
  void OnLaunched(activation::LaunchActivatedEventArgs const &);
  void OnSuspending(IInspectable const &, Windows::ApplicationModel::SuspendingEventArgs const &);
  void OnNavigationFailed(IInspectable const &, Windows::UI::Xaml::Navigation::NavigationFailedEventArgs const &);
  void OnBackgroundActivated(winrt::Windows::ApplicationModel::Activation::BackgroundActivatedEventArgs const &);

 private:
  using super = AppT<App>;
  winrt::Microsoft::ReactNative::IReactInstanceSettings::InstanceLoaded_revoker m_instanceLoaded;
};

} // namespace winrt::MainApp::implementation

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
//
// In this file we override some of the generated code to consume XAML ABI.
#pragma once

#include <winrt/base.h>
#ifndef USE_WINUI3
#include <winrt/impl/Windows.UI.Xaml.2.h>
#else
#include <winrt/impl/Microsoft.UI.Xaml.2.h>
#endif

namespace winrt::impl {

// Override xaml::Application::Current() to return nullptr instead of crashing
// when Application is not found.
#ifndef USE_WINUI3
template <>
WINRT_IMPL_AUTO(Windows::UI::Xaml::Application)
inline consume_Windows_UI_Xaml_IApplicationStatics<Windows::UI::Xaml::IApplicationStatics>::Current() const {
  using D = Windows::UI::Xaml::IApplicationStatics; // for the WINRT_IMPL_SHIM macro
  void *value{};
  WINRT_IMPL_SHIM(Windows::UI::Xaml::IApplicationStatics)->get_Current(&value);
  return Windows::UI::Xaml::Application{value, take_ownership_from_abi};
}
#else
template <>
WINRT_IMPL_AUTO(Microsoft::UI::Xaml::Application)
inline consume_Microsoft_UI_Xaml_IApplicationStatics<Microsoft::UI::Xaml::IApplicationStatics>::Current() const {
  using D = Microsoft::UI::Xaml::IApplicationStatics; // for the WINRT_IMPL_SHIM macro
  void *value{};
  WINRT_IMPL_SHIM(Microsoft::UI::Xaml::IApplicationStatics)->get_Current(&value);
  return Microsoft::UI::Xaml::Application{value, take_ownership_from_abi};
}
#endif

} // namespace winrt::impl

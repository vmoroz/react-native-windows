// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

//
// In this file we override some of the XAML ABI consume code.
//

#include <winrt/base.h>

#ifndef USE_WINUI3
#include <winrt/impl/Windows.UI.Xaml.2.h>
#define XAML_NS Windows::UI::Xaml
#define consume_XAML_IApplicationStatics consume_Windows_UI_Xaml_IApplicationStatics
#else
#include <winrt/impl/Microsoft.UI.Xaml.2.h>
#define XAML_NS Microsoft::UI::Xaml
#define consume_XAML_IApplicationStatics consume_Microsoft_UI_Xaml_IApplicationStatics
#endif

namespace winrt::impl {

// Override xaml::Application::Currnet() to return nullptr instead of crashing
// if Application is not found.
template <>
WINRT_IMPL_AUTO(XAML_NS::Application)
inline consume_XAML_IApplicationStatics<XAML_NS::IApplicationStatics>::Current() const {
  using D = XAML_NS::IApplicationStatics;
  void *value{};
  WINRT_IMPL_SHIM(XAML_NS::IApplicationStatics)->get_Current(&value);
  return XAML_NS::Application{value, take_ownership_from_abi};
}

} // namespace winrt::impl

#undef XAML_NS
#undef consume_XAML_IApplicationStatics

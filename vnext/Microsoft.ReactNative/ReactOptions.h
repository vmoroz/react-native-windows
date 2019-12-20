// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "winrt/Microsoft.ReactNative.h"

namespace winrt::Microsoft::ReactNative {

struct ReactOptionsData {
  ReactOptionsData() = default;
  ReactOptionsData(Microsoft::ReactNative::IReactOptions const &options) noexcept;

  ReactOptionsData DeepCopy() const noexcept;

  Windows::Foundation::Collections::IVector<IReactPackageProvider> PackageProviders{
      single_threaded_vector<IReactPackageProvider>()};
  hstring MainComponentName{L""};
  bool UseDeveloperSupport{false};
  hstring JavaScriptMainModuleName{L""};
  hstring JavaScriptBundleFile{L""};
};

struct ReactOptions : implements<ReactOptions, IReactOptions> {
  ReactOptions(ReactOptionsData &&data) noexcept;

  Windows::Foundation::Collections::IVectorView<IReactPackageProvider> PackageProviders() noexcept;

  hstring MainComponentName() noexcept;
  bool UseDeveloperSupport() noexcept;
  hstring JavaScriptMainModuleName() noexcept;
  hstring JavaScriptBundleFile() noexcept;

 private:
  ReactOptionsData m_data;
};

} // namespace winrt::Microsoft::ReactNative

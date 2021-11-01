// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <JSI/JsiAbiApi.h>
#include <JSI/JsiApiContext.h>
#include <ReactCommon/TurboModule.h>
#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Windows.Foundation.h>

namespace winrt::Microsoft::ReactNative {

// Creates CallInvoker based on JSDispatcher.
std::shared_ptr<facebook::react::CallInvoker> MakeAbiCallInvoker(IReactDispatcher const &jsDispatcher) noexcept;

template <
    typename TTurboModule,
    std::enable_if_t<std::is_base_of_v<::facebook::react::TurboModule, TTurboModule>, int> = 0>
void AddTurboModuleProvider(IReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) {
  auto packageBuilderObj = packageBuilder.as<ReactPackageBuilder>();
  packageBuilderObj.AddTurboModule(moduleName, [](IReactModuleBuilder const &moduleBuilder) noexcept {
    auto moduleBuilderObj = moduleBuilder.as<ReactModuleBuilder>();
    GetOrCreateContextRuntime(ReactContext{moduleBuilderObj.Context()}); // Ensure the JSI runtime is created.
    auto callInvoker = MakeAbiCallInvoker(moduleBuilderObj.Context().JSDispatcher());
    auto turboModule = std::make_shared<TTurboModule>(callInvoker);
    auto abiTurboModule = winrt::make<JsiHostObjectWrapper>(std::move(turboModule));
    return abiTurboModule.as<winrt::Windows::Foundation::IInspectable>();
  });
}

} // namespace winrt::Microsoft::ReactNative

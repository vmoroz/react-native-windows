// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactModuleBuilderMock.h"

#include <sstream>
#include "NativeModules.h"
#include "future/futureWait.h"

namespace ReactNativeTests {
REACT_MODULE(MyTurboModule)
struct MyTurboModule {
  REACT_METHOD(Add, L"add")
  int Add(int x, int y) noexcept {
    return x + y;
  }
};

// The TurboModule spec is going to be generated from the Flow spec file.
// It verifies that:
// - module methods names are unique;
// - method names are matching to the module spec method names;
// - method signatures match the spec method signatures.
struct MyTurboModuleSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(int, int, Callback<int>) noexcept>{0, L"add"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, MyTurboModuleSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
        0,
        "add",
        "    REACT_METHOD(add) int add(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(add) void add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(add) winrt::fire_and_forget add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n");
  }
};

TEST_CLASS (TurboModuleTest) {
  winrt::Microsoft::ReactNative::ReactModuleBuilderMock m_builderMock{};
  winrt::Microsoft::ReactNative::IReactModuleBuilder m_moduleBuilder;
  Windows::Foundation::IInspectable m_moduleObject{nullptr};
  MyTurboModule *m_module;

  TurboModuleTest() {
    m_moduleBuilder = winrt::make<winrt::Microsoft::ReactNative::ReactModuleBuilderImpl>(m_builderMock);
    auto provider = winrt::Microsoft::ReactNative::MakeTurboModuleProvider<MyTurboModule, MyTurboModuleSpec>();
    m_moduleObject = m_builderMock.CreateModule(provider, m_moduleBuilder);
    auto reactModule = m_moduleObject.as<winrt::Microsoft::ReactNative::IBoxedValue>();
    m_module = &winrt::Microsoft::ReactNative::BoxedValue<MyTurboModule>::GetImpl(reactModule);
  }

  TEST_METHOD(TestMethodCall_Add) {
    m_builderMock.Call1(L"Add", std::function<void(int)>([](int result) noexcept { TestCheck(result == 8); }), 3, 5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }
};

} // namespace ReactNativeTests

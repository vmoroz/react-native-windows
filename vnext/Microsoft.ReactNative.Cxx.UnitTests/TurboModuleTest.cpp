// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactModuleBuilderMock.h"

#include <sstream>
#include "NativeModules.h"
#include "future/futureWait.h"

namespace winrt::Microsoft::ReactNative {

REACT_MODULE(MyTurboModule)
struct MyTurboModule {
  REACT_METHOD(Add, L"add")
  int Add(int x, int y) noexcept {
    return x + y;
  }
};

struct MyTurboModuleSpec {
  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    // TODO: see what virtual inheritance says about missing method
    constexpr auto verificationResult = ReactModuleVerifier<TModule>::GetAsyncMethodCount(L"add");
    static_assert(verificationResult.MethodNameCount <= 1, "Name 'add' used for multiple methods");
    static_assert(verificationResult.MatchCount == 1, "Async Method 'add' is not defined");
    constexpr bool matches =
        ReactMethodVerifier<TModule, verificationResult.MatchedMemberId, MethodSpecArgs<int, int>>::Verify();
    static_assert(matches, "Async Method 'add' does not match signature");
  }
};

TEST_CLASS (TurboModuleTest) {
  ReactModuleBuilderMock m_builderMock{};
  IReactModuleBuilder m_moduleBuilder;
  Windows::Foundation::IInspectable m_moduleObject{nullptr};
  MyTurboModule *m_module;

  TurboModuleTest() {
    m_moduleBuilder = make<ReactModuleBuilderImpl>(m_builderMock);
    auto provider = MakeTurboModuleProvider<MyTurboModule, MyTurboModuleSpec>();
    m_moduleObject = m_builderMock.CreateModule(provider, m_moduleBuilder);
    auto reactModule = m_moduleObject.as<IBoxedValue>();
    m_module = &BoxedValue<MyTurboModule>::GetImpl(reactModule);
  }

  TEST_METHOD(TestMethodCall_Add) {
    m_builderMock.Call1(L"Add", std::function<void(int)>([](int result) noexcept { TestCheck(result == 8); }), 3, 5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }
};

} // namespace winrt::Microsoft::ReactNative

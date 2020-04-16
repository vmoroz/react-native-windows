// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactModuleBuilderMock.h"

#include <sstream>
#include "NativeModules.h"
#include "future/futureWait.h"

namespace winrt::Microsoft::ReactNative {

// static_assert(verificationResult.MethodNameCount <= 1, "Name 'add' used for multiple methods");
// static_assert(verificationResult.MatchCount == 1, "Async Method 'add' is not defined");
//// if constexpr (verificationResult.MatchCount == 1) {
////  constexpr bool matches =
////      ReactMethodVerifier<TModule, verificationResult.MatchedMemberId, ArgsSpec<int, int>>::Verify();
////  static_assert(matches, "Async Method 'add' does not match signature (see output)");
////}

// TurboModule spec provides method names with method signature spec.
// The spec verification checks:
// - that methods names are unique
// - list of names matches the spec list
// - methods match the method signature specification
struct TurboModuleSpec {
  struct BaseMethodSpec {
    constexpr BaseMethodSpec(int index, std::wstring_view name) : Index{index}, Name{name} {}

    int Index;
    std::wstring_view Name;
  };

  template <class TSignature>
  struct Method : BaseMethodSpec {
    using BaseMethodSpec::BaseMethodSpec;
  };

  template <class... TArgs>
  struct Callback : std::tuple<TArgs...> {};

  template <class... TArgs>
  struct Promise : std::tuple<TArgs...> {};

  struct MethodCheckResult {
    size_t MethodNameCount{0};
    size_t MatchCount{0};
  };

  template <class TModule, class TModuleSpec, size_t I>
  static constexpr MethodCheckResult CheckMethod() noexcept {
    constexpr auto verificationResult =
        ReactModuleVerifier<TModule>::VerifyAsyncMethod(std::get<I>(TModuleSpec::methods).Name);
    MethodCheckResult result{};
    result.MethodNameCount = verificationResult.MethodNameCount;
    result.MatchCount = verificationResult.MatchCount;
    return result;
  };

  template <class TModule, class TModuleSpec, size_t... I>
  static constexpr auto CheckMethodsHelper(std::index_sequence<I...>) noexcept {
    return std::array<MethodCheckResult, sizeof...(I)>{CheckMethod<TModule, TModuleSpec, I>()...};
  };

  template <class TModule, class TModuleSpec>
  static constexpr auto CheckMethods() noexcept {
    return CheckMethodsHelper<TModule, TModuleSpec>(
        std::make_index_sequence<std::tuple_size_v<decltype(TModuleSpec::methods)>>{});
  }
};

} // namespace winrt::Microsoft::ReactNative
#define REACT_SHOW_METHOD_SPEC_ERRORS(index, methodName, signatures)                                                \
  static_assert(methodCheckResults[index].MethodNameCount <= 1, "Name '" methodName "' used for multiple methods"); \
  static_assert(                                                                                                    \
      methodCheckResults[index].MatchCount == 1,                                                                    \
      "Method '" methodName                                                                                         \
      "' is not defined (see details below in output).\n"                                                           \
      "It must be one of the following:\n" signatures                                                               \
      "The C++ method name could be different. In that case add the L\"" methodName                                 \
      "\" to the attribute:\n"                                                                                      \
      "  REACT_METHOD(method, L\"" methodName                                                                       \
      "\")\n"                                                                                                       \
      "  \n");

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
      Method<void(int, int, Callback<int>)>{0, L"add"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, MyTurboModuleSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
        0,
        "add",
        "  REACT_METHOD(add) int add(int, int) noexcept {/*implementation*/}\n"
        "  REACT_METHOD(add) void add(int, int, Callback<int>) noexcept {/*implementation*/}\n"
        "  REACT_METHOD(add) winrt::fire_and_forget add(int, int, Callback<int>) noexcept {/*implementation*/}\n");
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

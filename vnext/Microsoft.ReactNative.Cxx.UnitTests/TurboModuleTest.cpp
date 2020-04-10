// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactModuleBuilderMock.h"

#include <sstream>
#include "NativeModules.h"
#include "future/futureWait.h"

namespace winrt::Microsoft::ReactNative {

// REACT_MODULE(MyTurboModule)
// struct MyTurboModule {
//  REACT_METHOD(Add)
//  int Add(int x, int y) noexcept {
//    return x + y;
//  }
//};
////
////template <class... TArgs>
////struct ReactCallback {};
//
//// This class should be generated
////template <class T>
////struct MyTurboModuleSpec
////{
////  static constexpr wchar_t const *Name = L"MyTurboModule";
////
////  static void RegisterModule(ReactModuleBuilder<T> &moduleBuilder) noexcept {
////    moduleBuilder.ModuleName(Name);
////
////    moduleBuilder.AddMethod<decltype(&T::Add), int, int, ReactCallback<int>>(&T::Add, L"Add");
////    // moduleBuilder.RegisterMethod<L"Add">(&T::Add);
////    // moduleBuilder.VerifyMethod<L"Add2", void(int, int, ReactPromise<int>)>();
////  }
////};
////
// enum class VerificationResult
//{
//  Success,
//  NoMatchingName,
//  SignatureMismatch,
//};
//
// struct MyTurboModuleSpec {
//  template <class TMethod>
//  constexpr VerificationResult VerifyMethod(TMethod method, std::string_view methodName) noexcept {
//    if (methodName == "Add") {
//      return ModuleMethodInfo<TMethod>::template Matches<int, std::string>();
//    }
//
//    return VerificationResult::NoMatchingName;
//  }
//};
//
// TEST_CLASS (TurboModuleTest) {
//  ReactModuleBuilderMock m_builderMock{};
//  IReactModuleBuilder m_moduleBuilder;
//  Windows::Foundation::IInspectable m_moduleObject{nullptr};
//  MyTurboModule *m_module;
//
//  TurboModuleTest(){
//    m_moduleBuilder = make<ReactModuleBuilderImpl>(m_builderMock);
//    auto provider = MakeTurboModuleProvider<MyTurboModule, MyTurboModuleSpec>();
//    m_moduleObject = m_builderMock.CreateModule(provider, m_moduleBuilder);
//    auto reactModule = m_moduleObject.as<IBoxedValue>();
//    m_module = &BoxedValue<MyTurboModule>::GetImpl(reactModule);
//  }
//
//  TEST_METHOD(TestMethodCall_Add) {
//    m_builderMock.Call1(L"Add", std::function<void(int)>([](int result) noexcept { TestCheck(result == 8); }), 3, 5);
//    TestCheck(m_builderMock.IsResolveCallbackCalled());
//  }
//};

} // namespace winrt::Microsoft::ReactNative

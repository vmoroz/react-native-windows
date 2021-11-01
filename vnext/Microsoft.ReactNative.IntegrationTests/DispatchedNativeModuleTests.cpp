// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include <JSI/JsiApiContext.h>
#include <NativeModules.h>
#include <winrt/Windows.System.h>
#include "TestEventService.h"
#include "TestReactNativeHostHolder.h"

using namespace facebook::jsi;
using namespace winrt;
using namespace Microsoft::ReactNative;
using namespace Windows::System;

namespace ReactNativeIntegrationTests {

// Use anonymous namespace to avoid any linking conflicts
namespace {

// The default dispatcher is the JSDispatcher.
// We check that all methods are run in the JSDispatcher.
REACT_MODULE(DefaultDispatchedModule)
struct DefaultDispatchedModule {
  REACT_INITIALIZER(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("DefaultDispatchedModule::Initialize");
  }

  REACT_FINALIZER(Finalize)
  void Finalize() noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("DefaultDispatchedModule::Finalize");
  }

  REACT_CONSTANT_PROVIDER(GetConstants)
  void GetConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myConst", 42);
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("DefaultDispatchedModule::GetConstants");
  }

  REACT_METHOD(TestAsyncMethod, L"testAsyncMethod")
  void TestAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("DefaultDispatchedModule::TestAsyncMethod");
  }

  REACT_SYNC_METHOD(TestSyncMethod, L"testSyncMethod")
  int32_t TestSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("DefaultDispatchedModule::TestSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// The UIDispatcher is the STA thread dispatcher.
// The UIDispatcher name used in the attribute macro is a special predefined alias
// for ReactDispatcherHelper::UIDispatcherProperty() that only can be used in the REACT_MODULE macro.
// Alternatively, we can use the ReactDispatcherHelper::UIDispatcherProperty() directly.
// We check that all module methods run in the UI dispatcher.
REACT_MODULE(UIDispatchedModule, dispatcherName = UIDispatcher)
struct UIDispatchedModule {
  REACT_INITIALIZER(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule::Initialize");
  }

  REACT_FINALIZER(Finalize)
  void Finalize() noexcept {
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule::Finalize");
  }

  REACT_CONSTANT_PROVIDER(GetConstants)
  void GetConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myConst", 42);
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule::GetConstants");
  }

  REACT_METHOD(TestAsyncMethod, L"testAsyncMethod")
  void TestAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("UIDispatchedModule::TestAsyncMethod");
  }

  REACT_SYNC_METHOD(TestSyncMethod, L"testSyncMethod")
  int32_t TestSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("UIDispatchedModule::TestSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// The JSDispatcher is the default dispatcher. The JSDispatcher is a special predefined
// alias for ReactDispatcherHelper::JSDispatcherProperty() that only can be used in the REACT_MODULE macro.
REACT_MODULE(JSDispatchedModule, dispatcherName = JSDispatcher)
struct JSDispatchedModule {
  REACT_INITIALIZER(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("JSDispatchedModule::Initialize");
  }

  REACT_FINALIZER(Finalize)
  void Finalize() noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("JSDispatchedModule::Finalize");
  }

  REACT_CONSTANT_PROVIDER(GetConstants)
  void GetConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myConst", 42);
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("JSDispatchedModule::GetConstants");
  }

  REACT_METHOD(TestAsyncMethod, L"testAsyncMethod")
  void TestAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("JSDispatchedModule::TestAsyncMethod");
  }

  REACT_SYNC_METHOD(TestSyncMethod, L"testSyncMethod")
  int32_t TestSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("JSDispatchedModule::TestSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// Define the custom dispatcher property Id: it encapsulates the property name and type.
ReactPropertyId<IReactDispatcher> CustomDispatcherId() noexcept {
  static auto dispatcherId = ReactPropertyId<IReactDispatcher>(L"ReactNativeIntegrationTests", L"CustomDispatcher");
  return dispatcherId;
}

// Use custom dispatcher to run tasks sequentially in background threads.
// While each task may run in a different thread, the dispatcher guarantees that they are run sequentially.
// We must provide the IReactPropertyName as a value to dispatcherName argument.
REACT_MODULE(CustomDispatchedModule, dispatcherName = CustomDispatcherId().Handle())
struct CustomDispatchedModule {
  REACT_INITIALIZER(Initialize)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule::Initialize");
  }

  REACT_FINALIZER(Finalize)
  void Finalize() noexcept {
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule::Finalize");
  }

  REACT_CONSTANT_PROVIDER(GetConstants)
  void GetConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myConst", 42);
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule::GetConstants");
  }

  REACT_METHOD(TestAsyncMethod, L"testAsyncMethod")
  void TestAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("CustomDispatchedModule::TestAsyncMethod");
  }

  REACT_SYNC_METHOD(TestSyncMethod, L"testSyncMethod")
  int32_t TestSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("CustomDispatchedModule::TestSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// This module uses UIDispatcher, but we opt out to call its members in JS dispatcher.
// In your module you could mix and match members that are run in UI and JS dispatchers.
// A module may have more than one member of each type.
REACT_MODULE(UIDispatchedModule2, dispatcherName = UIDispatcher)
struct UIDispatchedModule2 {
  REACT_INITIALIZER(Initialize, useJSDispatcher = true)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule2::Initialize");
  }

  REACT_FINALIZER(Finalize, useJSDispatcher = true)
  void Finalize() noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule2::Finalize");
  }

  REACT_CONSTANT_PROVIDER(GetConstants, useJSDispatcher = true)
  void GetConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myConst", 42);
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule2::GetConstants");
  }

  REACT_METHOD(TestAsyncMethod, L"testAsyncMethod", useJSDispatcher = true)
  void TestAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("UIDispatchedModule2::TestAsyncMethod");
  }

  REACT_SYNC_METHOD(TestSyncMethod, L"testSyncMethod", useJSDispatcher = true)
  int32_t TestSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("UIDispatchedModule2::TestSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// The JSDispatcher is the default dispatcher. The JSDispatcher is a special predefined
// alias for ReactDispatcherHelper::JSDispatcherProperty() that only can be used in the REACT_MODULE macro.
// This module is showing that `useJSDispatcher = true` is no-op for JSDispatcher modules because they run in
// JS dispatcher by default for default and JS dispatcher modules.
REACT_MODULE(JSDispatchedModule2, dispatcherName = JSDispatcher)
struct JSDispatchedModule2 {
  REACT_INITIALIZER(Initialize, useJSDispatcher = true)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("JSDispatchedModule2::Initialize");
  }

  REACT_FINALIZER(Finalize, useJSDispatcher = true)
  void Finalize() noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("JSDispatchedModule2::Finalize");
  }

  REACT_CONSTANT_PROVIDER(GetConstants, useJSDispatcher = true)
  void GetConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myConst", 42);
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("JSDispatchedModule2::GetConstants");
  }

  REACT_METHOD(TestAsyncMethod, L"testAsyncMethod", useJSDispatcher = true)
  void TestAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("JSDispatchedModule2::TestAsyncMethod");
  }

  REACT_SYNC_METHOD(TestSyncMethod, L"testSyncMethod", useJSDispatcher = true)
  int32_t TestSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("JSDispatchedModule2::TestSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// This module uses custom dispatcher, but we opt out to call its members in JS dispatcher.
// In your module you could mix and match members that are run in custom and JS dispatchers.
// A module may have more than one member of each type.
REACT_MODULE(CustomDispatchedModule2, dispatcherName = CustomDispatcherId().Handle())
struct CustomDispatchedModule2 {
  REACT_INITIALIZER(Initialize, useJSDispatcher = true)
  void Initialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule2::Initialize");
  }

  REACT_FINALIZER(Finalize, useJSDispatcher = true)
  void Finalize() noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule2::Finalize");
  }

  REACT_CONSTANT_PROVIDER(GetConstants, useJSDispatcher = true)
  void GetConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myConst", 42);
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule2::GetConstants");
  }

  REACT_METHOD(TestAsyncMethod, L"testAsyncMethod", useJSDispatcher = true)
  void TestAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("CustomDispatchedModule2::TestAsyncMethod");
  }

  REACT_SYNC_METHOD(TestSyncMethod, L"testSyncMethod", useJSDispatcher = true)
  int32_t TestSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("CustomDispatchedModule2::TestSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// This module shows how to have both UI and JS dispatcher members for UIDispatcher modules.
REACT_MODULE(UIDispatchedModule3, dispatcherName = UIDispatcher)
struct UIDispatchedModule3 {
  // The JSDispatcher initializers are run before the UIDispatcher initializers.
  REACT_INITIALIZER(JSInitialize, useJSDispatcher = true)
  void JSInitialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule3::JSInitialize");
  }

  REACT_INITIALIZER(UIInitialize)
  void UIInitialize(ReactContext const & /*reactContext*/) noexcept {
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule3::UIInitialize");
  }

  REACT_FINALIZER(UIFinalize)
  void UIFinalize() noexcept {
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule3::UIFinalize");
  }

  // The JSDispatcher finalizers are run after the UIDispatcher finalizers.
  REACT_FINALIZER(JSFinalize, useJSDispatcher = true)
  void JSFinalize() noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule3::JSFinalize");
  }

  REACT_CONSTANT_PROVIDER(GetUIConstants)
  void GetUIConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myUIConst", 42);
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule3::GetUIConstants");
  }

  REACT_CONSTANT_PROVIDER(GetJSConstants, useJSDispatcher = true)
  void GetJSConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myJSConst", 24);
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("UIDispatchedModule3::GetJSConstants");
  }

  REACT_METHOD(TestUIAsyncMethod, L"testUIAsyncMethod")
  void TestUIAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("UIDispatchedModule3::TestUIAsyncMethod");
  }

  REACT_METHOD(TestJSAsyncMethod, L"testJSAsyncMethod", useJSDispatcher = true)
  void TestJSAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(24, value);
    TestEventService::LogEvent("UIDispatchedModule3::TestJSAsyncMethod");
  }

  REACT_SYNC_METHOD(TestUISyncMethod, L"testUISyncMethod")
  int32_t TestUISyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.UIDispatcher().HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("UIDispatchedModule3::TestUISyncMethod");
    return value;
  }

  REACT_SYNC_METHOD(TestJSSyncMethod, L"testJSSyncMethod", useJSDispatcher = true)
  int32_t TestJSSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(24, value);
    TestEventService::LogEvent("UIDispatchedModule3::TestJSSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

// This module shows how to have both custom and JS dispatcher members for custom dispatcher modules.
REACT_MODULE(CustomDispatchedModule3, dispatcherName = CustomDispatcherId().Handle())
struct CustomDispatchedModule3 {
  // The JSDispatcher initializers are run before the CustomDispatcher initializers.
  REACT_INITIALIZER(JSInitialize, useJSDispatcher = true)
  void JSInitialize(ReactContext const &reactContext) noexcept {
    m_reactContext = reactContext;
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule3::JSInitialize");
  }

  REACT_INITIALIZER(CustomInitialize)
  void CustomInitialize(ReactContext const & /*reactContext*/) noexcept {
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule3::CustomInitialize");
  }

  REACT_FINALIZER(CustomFinalize)
  void CustomFinalize() noexcept {
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule3::CustomFinalize");
  }

  // The JSDispatcher finalizers are run after the CustomDispatcher finalizers.
  REACT_FINALIZER(JSFinalize, useJSDispatcher = true)
  void JSFinalize() noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule3::JSFinalize");
  }

  REACT_CONSTANT_PROVIDER(GetCustomConstants)
  void GetCustomConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myCustomConst", 42);
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule3::GetCustomConstants");
  }

  REACT_CONSTANT_PROVIDER(GetJSConstants, useJSDispatcher = true)
  void GetJSConstants(ReactConstantProvider &constantProvider) noexcept {
    constantProvider.Add(L"myJSConst", 24);
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestEventService::LogEvent("CustomDispatchedModule3::GetJSConstants");
  }

  REACT_METHOD(TestCustomAsyncMethod, L"testCustomAsyncMethod")
  void TestCustomAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("CustomDispatchedModule3::TestCustomAsyncMethod");
  }

  REACT_METHOD(TestJSAsyncMethod, L"testJSAsyncMethod", useJSDispatcher = true)
  void TestJSAsyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(24, value);
    TestEventService::LogEvent("CustomDispatchedModule3::TestJSAsyncMethod");
  }

  REACT_SYNC_METHOD(TestCustomSyncMethod, L"testCustomSyncMethod")
  int32_t TestCustomSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.Properties().Get(CustomDispatcherId()).HasThreadAccess());
    TestCheckEqual(42, value);
    TestEventService::LogEvent("CustomDispatchedModule3::TestCustomSyncMethod");
    return value;
  }

  REACT_SYNC_METHOD(TestJSSyncMethod, L"testJSSyncMethod", useJSDispatcher = true)
  int32_t TestJSSyncMethod(int32_t value) noexcept {
    TestCheck(m_reactContext.JSDispatcher().HasThreadAccess());
    TestCheckEqual(24, value);
    TestEventService::LogEvent("CustomDispatchedModule3::TestJSSyncMethod");
    return value;
  }

 private:
  ReactContext m_reactContext;
};

struct TestPackageProvider : ReactPackageProvider<TestPackageProvider> {
  void CreatePackage(IReactPackageBuilder const &packageBuilder) noexcept {
    AddModule<DefaultDispatchedModule>(packageBuilder);
    AddModule<UIDispatchedModule>(packageBuilder);
    AddModule<JSDispatchedModule>(packageBuilder);
    AddModule<CustomDispatchedModule>(packageBuilder);
    AddModule<UIDispatchedModule2>(packageBuilder);
    AddModule<JSDispatchedModule2>(packageBuilder);
    AddModule<CustomDispatchedModule2>(packageBuilder);
    AddModule<UIDispatchedModule3>(packageBuilder);
    AddModule<CustomDispatchedModule3>(packageBuilder);
  }
};

} // namespace

TEST_CLASS (DispatchedNativeModuleTests) {
  DispatcherQueueController m_uiQueueController{nullptr};
  std::unique_ptr<TestReactNativeHostHolder> m_reactNativeHost;
  ReactContext m_context;
  IReactInstanceSettings::InstanceLoaded_revoker m_instanceLoadedRevoker;

  DispatchedNativeModuleTests() {
    TestEventService::Initialize();

    // Simulate the UI thread using DispatcherQueueController
    m_uiQueueController = DispatcherQueueController::CreateOnDedicatedThread();

    auto uiDispatcher = m_uiQueueController.DispatcherQueue();
    uiDispatcher.TryEnqueue([&]() noexcept {
      m_reactNativeHost = std::make_unique<TestReactNativeHostHolder>(
          L"DispatchedNativeModuleTests", [&](ReactNativeHost const &host) noexcept {
            host.PackageProviders().Append(winrt::make<TestPackageProvider>());
            // Create and store custom dispatcher in the instance property bag.
            host.InstanceSettings().Properties().Set(
                CustomDispatcherId().Handle(), ReactDispatcher::CreateSerialDispatcher().Handle());
            m_instanceLoadedRevoker = host.InstanceSettings().InstanceLoaded(
                winrt::auto_revoke, [&](auto &&, InstanceLoadedEventArgs const &args) {
                  m_context = ReactContext(args.Context());
                  TestEventService::LogEvent("ContextAssigned");
                });
          });
    });

    TestEventService::ObserveEvents({
        TestEvent{"ContextAssigned"},
    });
  }

  ~DispatchedNativeModuleTests() {
    m_uiQueueController.ShutdownQueueAsync().get();
  }

  TEST_METHOD(TestDefaultDispatchedModule) {
    // All members are called in JS dispatcher
    m_context.CallJSFunction(L"TestDriver", L"testDefaultDispatchedModule");
    TestEventService::ObserveEvents({
        TestEvent{"DefaultDispatchedModule::Initialize"},
        TestEvent{"DefaultDispatchedModule::GetConstants"},
        TestEvent{"DefaultDispatchedModule::TestSyncMethod"},
        TestEvent{"DefaultDispatchedModule::TestAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"DefaultDispatchedModule::Finalize"},
    });
  }

  TEST_METHOD(TestUIDispatchedModule) {
    // All members are called in UI dispatcher
    m_context.CallJSFunction(L"TestDriver", L"testUIDispatchedModule");
    TestEventService::ObserveEvents({
        TestEvent{"UIDispatchedModule::Initialize"},
        TestEvent{"UIDispatchedModule::GetConstants"},
        TestEvent{"UIDispatchedModule::TestSyncMethod"},
        TestEvent{"UIDispatchedModule::TestAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"UIDispatchedModule::Finalize"},
    });
  }

  TEST_METHOD(TestJSDispatchedModule) {
    // All members are called in JS dispatcher
    m_context.CallJSFunction(L"TestDriver", L"testJSDispatchedModule");
    TestEventService::ObserveEvents({
        TestEvent{"JSDispatchedModule::Initialize"},
        TestEvent{"JSDispatchedModule::GetConstants"},
        TestEvent{"JSDispatchedModule::TestSyncMethod"},
        TestEvent{"JSDispatchedModule::TestAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"JSDispatchedModule::Finalize"},
    });
  }

  TEST_METHOD(TestCustomDispatchedModule) {
    // All members are called in custom dispatcher
    m_context.CallJSFunction(L"TestDriver", L"testCustomDispatchedModule");
    TestEventService::ObserveEvents({
        TestEvent{"CustomDispatchedModule::Initialize"},
        TestEvent{"CustomDispatchedModule::GetConstants"},
        TestEvent{"CustomDispatchedModule::TestSyncMethod"},
        TestEvent{"CustomDispatchedModule::TestAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"CustomDispatchedModule::Finalize"},
    });
  }

  TEST_METHOD(TestUIDispatchedModule2) {
    // All members are called in JS dispatcher
    m_context.CallJSFunction(L"TestDriver", L"testUIDispatchedModule2");
    TestEventService::ObserveEvents({
        TestEvent{"UIDispatchedModule2::Initialize"},
        TestEvent{"UIDispatchedModule2::GetConstants"},
        TestEvent{"UIDispatchedModule2::TestSyncMethod"},
        TestEvent{"UIDispatchedModule2::TestAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"UIDispatchedModule2::Finalize"},
    });
  }

  TEST_METHOD(TestJSDispatchedModule2) {
    // All members are called in JS dispatcher
    m_context.CallJSFunction(L"TestDriver", L"testJSDispatchedModule2");
    TestEventService::ObserveEvents({
        TestEvent{"JSDispatchedModule2::Initialize"},
        TestEvent{"JSDispatchedModule2::GetConstants"},
        TestEvent{"JSDispatchedModule2::TestSyncMethod"},
        TestEvent{"JSDispatchedModule2::TestAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"JSDispatchedModule2::Finalize"},
    });
  }

  TEST_METHOD(TestCustomDispatchedModule2) {
    // All members are called in JS dispatcher
    m_context.CallJSFunction(L"TestDriver", L"testCustomDispatchedModule2");
    TestEventService::ObserveEvents({
        TestEvent{"CustomDispatchedModule2::Initialize"},
        TestEvent{"CustomDispatchedModule2::GetConstants"},
        TestEvent{"CustomDispatchedModule2::TestSyncMethod"},
        TestEvent{"CustomDispatchedModule2::TestAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"CustomDispatchedModule2::Finalize"},
    });
  }

  TEST_METHOD(TestUIDispatchedModule3) {
    // The members are called in UI and JS dispatchers
    m_context.CallJSFunction(L"TestDriver", L"testUIDispatchedModule3");
    TestEventService::ObserveEvents({
        TestEvent{"UIDispatchedModule3::JSInitialize"},
        TestEvent{"UIDispatchedModule3::UIInitialize"},
        TestEvent{"UIDispatchedModule3::GetJSConstants"},
        TestEvent{"UIDispatchedModule3::GetUIConstants"},
        TestEvent{"UIDispatchedModule3::TestJSSyncMethod"},
        TestEvent{"UIDispatchedModule3::TestUISyncMethod"},
        TestEvent{"UIDispatchedModule3::TestUIAsyncMethod"},
        TestEvent{"UIDispatchedModule3::TestJSAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"UIDispatchedModule3::UIFinalize"},
        TestEvent{"UIDispatchedModule3::JSFinalize"},
    });
  }

  TEST_METHOD(TestCustomDispatchedModule3) {
    // The members are called in custom and JS dispatchers
    m_context.CallJSFunction(L"TestDriver", L"testCustomDispatchedModule3");
    TestEventService::ObserveEvents({
        TestEvent{"CustomDispatchedModule3::JSInitialize"},
        TestEvent{"CustomDispatchedModule3::CustomInitialize"},
        TestEvent{"CustomDispatchedModule3::GetJSConstants"},
        TestEvent{"CustomDispatchedModule3::GetCustomConstants"},
        TestEvent{"CustomDispatchedModule3::TestJSSyncMethod"},
        TestEvent{"CustomDispatchedModule3::TestCustomSyncMethod"},
        TestEvent{"CustomDispatchedModule3::TestCustomAsyncMethod"},
        TestEvent{"CustomDispatchedModule3::TestJSAsyncMethod"},
    });

    m_reactNativeHost = nullptr;
    TestEventService::ObserveEvents({
        TestEvent{"CustomDispatchedModule3::CustomFinalize"},
        TestEvent{"CustomDispatchedModule3::JSFinalize"},
    });
  }
};

} // namespace ReactNativeIntegrationTests

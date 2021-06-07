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
// We check that all module methods run in the UI thread.
REACT_MODULE(UIDispatchedModule, dispatcherName = UIDispatcher)
struct UIDispatchedModule {
  REACT_INIT(Initialize)
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
  REACT_INIT(Initialize)
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
// We must provide the IReactPropertyName as a value to dispatcherName argument.
REACT_MODULE(CustomDispatchedModule, dispatcherName = CustomDispatcherId().Handle())
struct CustomDispatchedModule {
  REACT_INIT(Initialize)
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

struct TestPackageProvider : ReactPackageProvider<TestPackageProvider> {
  void CreatePackage(IReactPackageBuilder const &packageBuilder) noexcept {
    AddModule<DefaultDispatchedModule>(packageBuilder);
    AddModule<UIDispatchedModule>(packageBuilder);
    AddModule<JSDispatchedModule>(packageBuilder);
    AddModule<CustomDispatchedModule>(packageBuilder);
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
};

} // namespace ReactNativeIntegrationTests

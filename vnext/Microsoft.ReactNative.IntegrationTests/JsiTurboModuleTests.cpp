// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// The TurboModule sample is based on the code from the react-native NPM package.
// Below is the original copyright notice for that code.
// The referred LICENSE file is in the root of the react-native NPM package.
/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

// This test shows how we can use the react-native style of C++ TurboModules in
// ReactNative Windows. They work on top of ABI safe JSI implementation.
// The only RNW specific part is the registration of TurboModule class using IReactPackageBuilder.
// See the details for the MySimpleTurboModulePackageProvider below.

#include "pch.h"
#include <JSValue.h>
#include <ReactCommon/TurboModule.h> // This comes from the react-native package.
#include <ReactCommon/TurboModuleUtils.h> // This must come from the react-native package, but we use a local version to fix issues.
#include <TurboModuleProvider.h> // It is RNW specific
#include "TestEventService.h"

#include <condition_variable>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>

using namespace winrt::Microsoft::ReactNative;
using namespace winrt::Windows::Foundation;

namespace ReactNativeIntegrationTests {

// Use anonymous namespace to avoid any linking conflicts
namespace {

// In this test we put spec definition that normally must be generated.
// >>>> Start generated

// The spec from .h file
struct MySimpleTurboModuleSpec : ::facebook::react::TurboModule {
  virtual void
  logAction(facebook::jsi::Runtime &rt, const facebook::jsi::String &actionName, const facebook::jsi::Value &value) = 0;
  virtual void voidFunc(::facebook::jsi::Runtime &rt) = 0;
  virtual bool getBool(::facebook::jsi::Runtime &rt, bool arg) = 0;
  virtual double getNumber(::facebook::jsi::Runtime &rt, double arg) = 0;
  virtual ::facebook::jsi::String getString(::facebook::jsi::Runtime &rt, const ::facebook::jsi::String &arg) = 0;
  virtual ::facebook::jsi::Array getArray(::facebook::jsi::Runtime &rt, const ::facebook::jsi::Array &arg) = 0;
  virtual ::facebook::jsi::Object getObject(::facebook::jsi::Runtime &rt, const ::facebook::jsi::Object &arg) = 0;
  virtual ::facebook::jsi::Object getValue(
      ::facebook::jsi::Runtime &rt,
      double x,
      const ::facebook::jsi::String &y,
      const ::facebook::jsi::Object &z) = 0;
  virtual void getValueWithCallback(::facebook::jsi::Runtime &rt, const ::facebook::jsi::Function &callback) = 0;
  virtual ::facebook::jsi::Value getValueWithPromise(::facebook::jsi::Runtime &rt, bool error) = 0;
  virtual ::facebook::jsi::Object getConstants(::facebook::jsi::Runtime &rt) = 0;

 protected:
  MySimpleTurboModuleSpec(std::shared_ptr<facebook::react::CallInvoker> jsInvoker);
};

// The spec from .cpp file

static ::facebook::jsi::Value MySimpleTurboModuleSpec_logAction(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 2);
  static_cast<MySimpleTurboModuleSpec *>(&turboModule)->logAction(rt, args[0].getString(rt), args[1]);
  return ::facebook::jsi::Value::undefined();
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_voidFunc(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 0);
  static_cast<MySimpleTurboModuleSpec *>(&turboModule)->voidFunc(rt);
  return ::facebook::jsi::Value::undefined();
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getBool(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 1);
  return ::facebook::jsi::Value(static_cast<MySimpleTurboModuleSpec *>(&turboModule)->getBool(rt, args[0].getBool()));
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getNumber(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 1);
  return ::facebook::jsi::Value(
      static_cast<MySimpleTurboModuleSpec *>(&turboModule)->getNumber(rt, args[0].getNumber()));
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getString(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 1);
  return static_cast<MySimpleTurboModuleSpec *>(&turboModule)->getString(rt, args[0].getString(rt));
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getArray(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 1);
  return static_cast<MySimpleTurboModuleSpec *>(&turboModule)->getArray(rt, args[0].getObject(rt).getArray(rt));
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getObject(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 1);
  return static_cast<MySimpleTurboModuleSpec *>(&turboModule)->getObject(rt, args[0].getObject(rt));
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getValue(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 3);
  return static_cast<MySimpleTurboModuleSpec *>(&turboModule)
      ->getValue(rt, args[0].getNumber(), args[1].getString(rt), args[2].getObject(rt));
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getValueWithCallback(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 1);
  static_cast<MySimpleTurboModuleSpec *>(&turboModule)
      ->getValueWithCallback(rt, std::move(args[0].getObject(rt).getFunction(rt)));
  return ::facebook::jsi::Value::undefined();
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getValueWithPromise(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 1);
  return static_cast<MySimpleTurboModuleSpec *>(&turboModule)->getValueWithPromise(rt, args[0].getBool());
}

static ::facebook::jsi::Value MySimpleTurboModuleSpec_getConstants(
    ::facebook::jsi::Runtime &rt,
    ::facebook::react::TurboModule &turboModule,
    [[maybe_unused]] const ::facebook::jsi::Value *args,
    [[maybe_unused]] size_t count) {
  assert(count >= 0);
  return static_cast<MySimpleTurboModuleSpec *>(&turboModule)->getConstants(rt);
}

MySimpleTurboModuleSpec::MySimpleTurboModuleSpec(std::shared_ptr<::facebook::react::CallInvoker> jsInvoker)
    : ::facebook::react::TurboModule("MySimpleTurboModule", std::move(jsInvoker)) {
  methodMap_.try_emplace("logAction", MethodMetadata{0, MySimpleTurboModuleSpec_logAction});
  methodMap_.try_emplace("voidFunc", MethodMetadata{0, MySimpleTurboModuleSpec_voidFunc});
  methodMap_.try_emplace("getBool", MethodMetadata{1, MySimpleTurboModuleSpec_getBool});
  methodMap_.try_emplace("getNumber", MethodMetadata{1, MySimpleTurboModuleSpec_getNumber});
  methodMap_.try_emplace("getString", MethodMetadata{1, MySimpleTurboModuleSpec_getString});
  methodMap_.try_emplace("getArray", MethodMetadata{1, MySimpleTurboModuleSpec_getArray});
  methodMap_.try_emplace("getObject", MethodMetadata{1, MySimpleTurboModuleSpec_getObject});
  methodMap_.try_emplace("getValue", MethodMetadata{3, MySimpleTurboModuleSpec_getValue});
  methodMap_.try_emplace("getValueWithCallback", MethodMetadata{1, MySimpleTurboModuleSpec_getValueWithCallback});
  methodMap_.try_emplace("getValueWithPromise", MethodMetadata{1, MySimpleTurboModuleSpec_getValueWithPromise});
  methodMap_.try_emplace("getConstants", MethodMetadata{0, MySimpleTurboModuleSpec_getConstants});
}

// <<<< End generated

struct MySimpleTurboModule : MySimpleTurboModuleSpec {
  MySimpleTurboModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker);

  void logAction(facebook::jsi::Runtime &rt, const facebook::jsi::String &actionName, const facebook::jsi::Value &value)
      override;
  void voidFunc(facebook::jsi::Runtime &rt) override;
  bool getBool(facebook::jsi::Runtime &rt, bool arg) override;
  double getNumber(facebook::jsi::Runtime &rt, double arg) override;
  facebook::jsi::String getString(facebook::jsi::Runtime &rt, const facebook::jsi::String &arg) override;
  facebook::jsi::Array getArray(facebook::jsi::Runtime &rt, const facebook::jsi::Array &arg) override;
  facebook::jsi::Object getObject(facebook::jsi::Runtime &rt, const facebook::jsi::Object &arg) override;
  facebook::jsi::Object getValue(
      facebook::jsi::Runtime &rt,
      double x,
      const facebook::jsi::String &y,
      const facebook::jsi::Object &z) override;
  void getValueWithCallback(facebook::jsi::Runtime &rt, const facebook::jsi::Function &callback) override;
  facebook::jsi::Value getValueWithPromise(facebook::jsi::Runtime &rt, bool error) override;
  facebook::jsi::Object getConstants(facebook::jsi::Runtime &rt) override;
};

MySimpleTurboModule::MySimpleTurboModule(std::shared_ptr<facebook::react::CallInvoker> jsInvoker)
    : MySimpleTurboModuleSpec(std::move(jsInvoker)) {}

void MySimpleTurboModule::logAction(
    facebook::jsi::Runtime &rt,
    const facebook::jsi::String &actionName,
    const facebook::jsi::Value &value) {
  JSValue jsValue{};
  if (value.isBool()) {
    jsValue = JSValue(value.getBool());
  } else if (value.isNumber()) {
    jsValue = JSValue(value.getNumber());
  } else if (value.isString()) {
    jsValue = JSValue(value.getString(rt).utf8(rt));
  }
  TestEventService::LogEvent(actionName.utf8(rt), std::move(jsValue));
}

void MySimpleTurboModule::voidFunc(facebook::jsi::Runtime & /*rt*/) {
  TestEventService::LogEvent("voidFunc called", nullptr);
}

bool MySimpleTurboModule::getBool(facebook::jsi::Runtime & /*rt*/, bool arg) {
  TestEventService::LogEvent("getBool called", arg);
  return arg;
}

double MySimpleTurboModule::getNumber(facebook::jsi::Runtime & /*rt*/, double arg) {
  TestEventService::LogEvent("getNumber called", arg);
  return arg;
}

facebook::jsi::String MySimpleTurboModule::getString(facebook::jsi::Runtime &rt, const facebook::jsi::String &arg) {
  TestEventService::LogEvent("getString called", arg.utf8(rt));
  return facebook::jsi::String::createFromUtf8(rt, arg.utf8(rt));
}

facebook::jsi::Array MySimpleTurboModule::getArray(facebook::jsi::Runtime &rt, const facebook::jsi::Array &arg) {
  TestEventService::LogEvent("getArray called", arg.length(rt));
  return facebook::react::deepCopyJSIArray(rt, arg);
}

facebook::jsi::Object MySimpleTurboModule::getObject(facebook::jsi::Runtime &rt, const facebook::jsi::Object &arg) {
  TestEventService::LogEvent("getObject called", "OK");
  return facebook::react::deepCopyJSIObject(rt, arg);
}

facebook::jsi::Object MySimpleTurboModule::getValue(
    facebook::jsi::Runtime &rt,
    double x,
    const facebook::jsi::String &y,
    const facebook::jsi::Object &z) {
  TestEventService::LogEvent("getValue called", "OK");
  // Note: return type isn't type-safe.
  facebook::jsi::Object result(rt);
  result.setProperty(rt, "x", facebook::jsi::Value(x));
  result.setProperty(rt, "y", facebook::jsi::String::createFromUtf8(rt, y.utf8(rt)));
  result.setProperty(rt, "z", facebook::react::deepCopyJSIObject(rt, z));
  return result;
}

void MySimpleTurboModule::getValueWithCallback(facebook::jsi::Runtime &rt, const facebook::jsi::Function &callback) {
  TestEventService::LogEvent("getValueWithCallback called", "OK");
  callback.call(rt, facebook::jsi::String::createFromUtf8(rt, "value from callback!"));
}

facebook::jsi::Value MySimpleTurboModule::getValueWithPromise(facebook::jsi::Runtime &rt, bool error) {
  TestEventService::LogEvent("getValueWithPromise called", error);
  return facebook::react::createPromiseAsJSIValue(
      rt, [error](facebook::jsi::Runtime &rt2, std::shared_ptr<facebook::react::Promise> promise) {
        if (error) {
          promise->reject("intentional promise rejection");
        } else {
          promise->resolve(facebook::jsi::String::createFromUtf8(rt2, "result!"));
        }
      });
}

facebook::jsi::Object MySimpleTurboModule::getConstants(facebook::jsi::Runtime &rt) {
  TestEventService::LogEvent("getConstants called", "OK");
  // Note: return type isn't type-safe.
  facebook::jsi::Object result(rt);
  result.setProperty(rt, "const1", facebook::jsi::Value(true));
  result.setProperty(rt, "const2", facebook::jsi::Value(375));
  result.setProperty(rt, "const3", facebook::jsi::String::createFromUtf8(rt, "something"));
  return result;
}

struct MySimpleTurboModulePackageProvider
    : winrt::implements<MySimpleTurboModulePackageProvider, IReactPackageProvider> {
  void CreatePackage(IReactPackageBuilder const &packageBuilder) noexcept {
    AddTurboModuleProvider<MySimpleTurboModule>(packageBuilder, L"MySimpleTurboModule");
  }
};

} // namespace

TEST_CLASS (JsiTurboModuleTests) {
  TEST_METHOD(ExecuteSampleTurboModule) {
    TestEventService::Initialize();

    ReactNativeHost host{};

    auto queueController = winrt::Windows::System::DispatcherQueueController::CreateOnDedicatedThread();
    queueController.DispatcherQueue().TryEnqueue([&]() noexcept {
      host.PackageProviders().Append(winrt::make<MySimpleTurboModulePackageProvider>());

      // bundle is assumed to be co-located with the test binary
      wchar_t testBinaryPath[MAX_PATH];
      TestCheck(GetModuleFileNameW(NULL, testBinaryPath, MAX_PATH) < MAX_PATH);
      testBinaryPath[std::wstring_view{testBinaryPath}.rfind(L"\\")] = 0;

      host.InstanceSettings().BundleRootPath(testBinaryPath);
      host.InstanceSettings().JavaScriptBundleFile(L"JsiTurboModuleTests");
      host.InstanceSettings().UseDeveloperSupport(false);
      host.InstanceSettings().UseWebDebugger(false);
      host.InstanceSettings().UseFastRefresh(false);
      host.InstanceSettings().UseLiveReload(false);
      host.InstanceSettings().EnableDeveloperMenu(false);

      host.LoadInstance().Completed(
          [](IAsyncAction asyncInfo, AsyncStatus asyncStatus) { TestCheckEqual(AsyncStatus::Completed, asyncStatus); });
    });

    TestEventService::ObserveEvents({TestEvent{"voidFunc called", nullptr},
                                     TestEvent{"getBool called", true},
                                     TestEvent{"getBool result", true},
                                     TestEvent{"getBool called", false},
                                     TestEvent{"getBool result", false},
                                     TestEvent{"getNumber called", 5.0},
                                     TestEvent{"getNumber result", 5.0},
                                     TestEvent{"getNumber called", std::numeric_limits<double>::quiet_NaN()},
                                     TestEvent{"getNumber result", std::numeric_limits<double>::quiet_NaN()},
                                     TestEvent{"getNumber called", std::numeric_limits<double>::infinity()},
                                     TestEvent{"getNumber result", std::numeric_limits<double>::infinity()},
                                     TestEvent{"getString called", "Hello"},
                                     TestEvent{"getString result", "Hello"},
                                     TestEvent{"getString called", ""},
                                     TestEvent{"getString result", ""},
                                     TestEvent{"getArray called", 3},
                                     TestEvent{"getArray result", "OK"},
                                     TestEvent{"getObject called", "OK"},
                                     TestEvent{"getObject result", "OK"},
                                     TestEvent{"getValue called", "OK"},
                                     TestEvent{"getValue result", "OK"},
                                     TestEvent{"getConstants called", "OK"},
                                     TestEvent{"getConstants result", "OK"},
                                     TestEvent{"getValueWithCallback called", "OK"},
                                     TestEvent{"getValueWithCallback result", "value from callback!"},
                                     TestEvent{"getValueWithPromise called", false},
                                     TestEvent{"getValueWithPromise called", true},
                                     TestEvent{"getValueWithPromise result resolve", "result!"},
                                     TestEvent{"getValueWithPromise result reject", "intentional promise rejection"}});

    host.UnloadInstance().get();
    queueController.ShutdownQueueAsync().get();
  }
};

} // namespace ReactNativeIntegrationTests

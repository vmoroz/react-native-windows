// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// IMPORTANT: Before updating this file
// please read react-native-windows repo:
// vnext/Microsoft.ReactNative.Cxx/README.md

#include "pch.h"
#include "TurboModulesProvider.h"
#include <ReactCommon/TurboModuleUtils.h>
#include "IReactModuleBuilder.h"
#include "JsiApi.h"
#include "JsiReader.h"
#include "JsiWriter.h"
#ifdef __APPLE__
#include "Crash.h"
#else
#include <crash/verifyElseCrash.h>
#endif

using namespace winrt;
using namespace Windows::Foundation;

namespace winrt::Microsoft::ReactNative {

/*-------------------------------------------------------------------------------
  TurboModuleImpl
-------------------------------------------------------------------------------*/

class TurboModuleImpl : public facebook::react::TurboModule {
 public:
  TurboModuleImpl(
      const IReactContext &reactContext,
      const std::string &name,
      std::shared_ptr<facebook::react::CallInvoker> jsInvoker,
      ReactModuleProvider reactModuleProvider)
      : facebook::react::TurboModule(name, jsInvoker),
        m_moduleBuilder(winrt::make<implementation::ReactModuleBuilder>(reactContext)) {
    providedModule = reactModuleProvider(m_moduleBuilder);

    for (auto const &initializer : m_moduleBuilder.as<implementation::ReactModuleBuilder>()->GetInitializers()) {
      if (initializer.InitializerType == ReactInitializerType::Field) {
        initializer.Delegate(reactContext);
      }
    }

    for (auto const &initializer : m_moduleBuilder.as<implementation::ReactModuleBuilder>()->GetInitializers()) {
      if (initializer.InitializerType == ReactInitializerType::Method) {
        initializer.Delegate(reactContext);
      }
    }

    if (auto hostObject = providedModule.try_as<IJsiHostObject>()) {
      m_hostObjectWrapper = std::make_shared<implementation::HostObjectWrapper>(hostObject);
    }
  }

  std::vector<facebook::jsi::PropNameID> getPropertyNames(facebook::jsi::Runtime &rt) override {
    if (m_hostObjectWrapper) {
      return m_hostObjectWrapper->getPropertyNames(rt);
    }

    std::vector<facebook::jsi::PropNameID> props;
    auto tmb = m_moduleBuilder.as<implementation::ReactModuleBuilder>();
    for (auto &it : tmb->GetMethods()) {
      props.push_back(facebook::jsi::PropNameID::forAscii(rt, it.first));
    }
    return props;
  };

  facebook::jsi::Value get(facebook::jsi::Runtime &runtime, const facebook::jsi::PropNameID &propName) override {
    if (m_hostObjectWrapper) {
      return m_hostObjectWrapper->get(runtime, propName);
    }

    // it is not safe to assume that "runtime" never changes, so members are not cached here
    auto tmb = m_moduleBuilder.as<implementation::ReactModuleBuilder>();
    auto key = propName.utf8(runtime);

    if (key == "getConstants" && tmb->GetConstantProviders().size() > 0) {
      // try to find getConstants if there is any constant
      return facebook::jsi::Function::createFromHostFunction(
          runtime,
          propName,
          0,
          [&runtime, tmb](
              facebook::jsi::Runtime &rt,
              const facebook::jsi::Value &thisVal,
              const facebook::jsi::Value *args,
              size_t count) {
            // collect all constants to an object
            auto writer = winrt::make<JsiWriter>(runtime);
            writer.WriteObjectBegin();
            for (auto const &cp : tmb->GetConstantProviders()) {
              cp.Delegate(writer);
            }
            writer.WriteObjectEnd();
            return writer.as<JsiWriter>()->MoveResult();
          });
    }

    {
      // try to find a Method
      auto it = tmb->GetMethods().find(key);
      if (it != tmb->GetMethods().end()) {
        return facebook::jsi::Function::createFromHostFunction(
            runtime,
            propName,
            0,
            [&runtime, method = it->second](
                facebook::jsi::Runtime &rt,
                const facebook::jsi::Value &thisVal,
                const facebook::jsi::Value *args,
                size_t count) {
              // prepare input arguments
              size_t serializableArgumentCount = count;
              switch (method.ReturnType) {
                case MethodReturnType::Callback:
                  VerifyElseCrash(count >= 1);
                  VerifyElseCrash(args[count - 1].isObject() && args[count - 1].asObject(runtime).isFunction(runtime));
                  serializableArgumentCount -= 1;
                  break;
                case MethodReturnType::TwoCallbacks:
                  VerifyElseCrash(count >= 2);
                  VerifyElseCrash(args[count - 1].isObject() && args[count - 1].asObject(runtime).isFunction(runtime));
                  VerifyElseCrash(args[count - 2].isObject() && args[count - 2].asObject(runtime).isFunction(runtime));
                  serializableArgumentCount -= 2;
                  break;
                case MethodReturnType::Void:
                case MethodReturnType::Promise:
                  // handled below
                  break;
              }
              auto argReader = winrt::make<JsiReader>(runtime, args, serializableArgumentCount);

              // prepare output value
              // TODO: it is no reason to pass a argWriter just to receive [undefined] for void, should be optimized
              auto argWriter = winrt::make<JsiWriter>(runtime);

              // call the function
              switch (method.ReturnType) {
                case MethodReturnType::Void: {
                  method.Delegate(argReader, argWriter, nullptr, nullptr);
                  return facebook::jsi::Value::undefined();
                }
                case MethodReturnType::Promise: {
                  return facebook::react::createPromiseAsJSIValue(
                      runtime, [=](facebook::jsi::Runtime &runtime, std::shared_ptr<facebook::react::Promise> promise) {
                        method.Delegate(
                            argReader,
                            argWriter,
                            [promise, &runtime](const IJSValueWriter &writer) {
                              auto result = writer.as<JsiWriter>()->MoveResult();
                              if (result.isObject()) {
                                auto resultArrayObject = result.getObject(runtime);
                                VerifyElseCrash(resultArrayObject.isArray(runtime));
                                auto resultArray = resultArrayObject.getArray(runtime);
                                VerifyElseCrash(resultArray.length(runtime) == 1);
                                auto resultItem = resultArray.getValueAtIndex(runtime, 0);
                                promise->resolve(resultItem);
                              } else {
                                VerifyElseCrash(false);
                              }
                            },
                            [promise, &runtime](const IJSValueWriter &writer) {
                              auto result = writer.as<JsiWriter>()->MoveResult();
                              if (result.isString()) {
                                promise->reject(result.getString(runtime).utf8(runtime));
                              } else if (result.isObject()) {
                                auto errorArrayObject = result.getObject(runtime);
                                VerifyElseCrash(errorArrayObject.isArray(runtime));
                                auto errorArray = errorArrayObject.getArray(runtime);
                                VerifyElseCrash(errorArray.length(runtime) == 1);
                                auto errorObjectValue = errorArray.getValueAtIndex(runtime, 0);
                                VerifyElseCrash(errorObjectValue.isObject());
                                auto errorObject = errorObjectValue.getObject(runtime);
                                VerifyElseCrash(errorObject.hasProperty(runtime, "message"));
                                auto errorMessage = errorObject.getProperty(runtime, "message");
                                VerifyElseCrash(errorMessage.isString());
                                promise->reject(errorMessage.getString(runtime).utf8(runtime));
                              } else {
                                VerifyElseCrash(false);
                              }
                            });
                      });
                }
                case MethodReturnType::Callback:
                case MethodReturnType::TwoCallbacks: {
                  facebook::jsi::Value resolveFunction;
                  facebook::jsi::Value rejectFunction;
                  if (method.ReturnType == MethodReturnType::Callback) {
                    resolveFunction = {runtime, args[count - 1]};
                  } else {
                    resolveFunction = {runtime, args[count - 2]};
                    rejectFunction = {runtime, args[count - 1]};
                  }

                  auto makeCallback =
                      [&runtime](const facebook::jsi::Value &callbackValue) noexcept -> MethodResultCallback {
                    // workaround: xcode doesn't accept a captured value with only rvalue copy constructor
                    auto functionObject =
                        std::make_shared<facebook::jsi::Function>(callbackValue.asObject(runtime).asFunction(runtime));
                    return [&runtime, callbackFunction = functionObject](const IJSValueWriter &writer) noexcept {
                      const facebook::jsi::Value *resultArgs = nullptr;
                      size_t resultCount = 0;
                      writer.as<JsiWriter>()->AccessResultAsArgs(resultArgs, resultCount);
                      callbackFunction->call(runtime, resultArgs, resultCount);
                    };
                  };

                  method.Delegate(
                      argReader,
                      argWriter,
                      makeCallback(resolveFunction),
                      (method.ReturnType == MethodReturnType::Callback ? nullptr : makeCallback(rejectFunction)));
                  return facebook::jsi::Value::undefined();
                }
                default:
                  VerifyElseCrash(false);
              }
            });
      }
    }

    {
      // try to find a SyncMethod
      auto it = tmb->GetSyncMethods().find(key);
      if (it != tmb->GetSyncMethods().end()) {
        return facebook::jsi::Function::createFromHostFunction(
            runtime,
            propName,
            0,
            [&runtime, method = it->second](
                facebook::jsi::Runtime &rt,
                const facebook::jsi::Value &thisVal,
                const facebook::jsi::Value *args,
                size_t count) {
              // prepare input arguments
              auto argReader = winrt::make<JsiReader>(runtime, args, count);

              // prepare output value
              auto writer = winrt::make<JsiWriter>(runtime);

              // call the function
              method.Delegate(argReader, writer);

              return writer.as<JsiWriter>()->MoveResult();
            });
      }
    }

    // returns undefined if the expected member is not found
    return facebook::jsi::Value::undefined();
  }

  void set(facebook::jsi::Runtime &rt, const facebook::jsi::PropNameID &name, const facebook::jsi::Value &value)
      override {
    if (m_hostObjectWrapper) {
      return m_hostObjectWrapper->set(rt, name, value);
    }

    facebook::react::TurboModule::set(rt, name, value);
  }

 private:
  IReactModuleBuilder m_moduleBuilder;
  IInspectable providedModule;
  std::shared_ptr<implementation::HostObjectWrapper> m_hostObjectWrapper;
};

/*-------------------------------------------------------------------------------
  TurboModulesProvider
-------------------------------------------------------------------------------*/
TurboModulesProvider::TurboModulePtr TurboModulesProvider::getModule(
    const std::string &moduleName,
    const CallInvokerPtr &callInvoker) noexcept {
  // see if the expected turbo module has been cached
  auto pair = std::make_pair(moduleName, callInvoker);
  auto itCached = m_cachedModules.find(pair);
  if (itCached != m_cachedModules.end()) {
    return itCached->second;
  }

  // fail if the expected turbo module has not been registered
  auto it = m_moduleProviders.find(moduleName);
  if (it == m_moduleProviders.end()) {
    return nullptr;
  }

  // cache and return the turbo module
  auto tm = std::make_shared<TurboModuleImpl>(m_reactContext, moduleName, callInvoker, it->second);
  m_cachedModules.insert({pair, tm});
  return tm;
}

std::vector<std::string> TurboModulesProvider::getEagerInitModuleNames() noexcept {
  std::vector<std::string> eagerModules;
  auto it = m_moduleProviders.find("UIManager");
  if (it != m_moduleProviders.end()) {
    eagerModules.push_back("UIManager");
  }
  return eagerModules;
}

void TurboModulesProvider::SetReactContext(const IReactContext &reactContext) noexcept {
  m_reactContext = reactContext;
}

void TurboModulesProvider::AddModuleProvider(
    winrt::hstring const &moduleName,
    ReactModuleProvider const &moduleProvider) noexcept {
  auto key = to_string(moduleName);
  auto it = m_moduleProviders.find(key);
  if (it == m_moduleProviders.end()) {
    m_moduleProviders.insert({key, moduleProvider});
  } else {
    // turbo modules should be replaceable before the first time it is requested
    // if a turbo module has been requested, it will be cached in m_cachedModules
    // in this case, changing m_moduleProviders affects nothing
    it->second = moduleProvider;
  }
}

} // namespace winrt::Microsoft::ReactNative

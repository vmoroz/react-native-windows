// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "ReactInstanceWin.h"
#include "MoveOnCopy.h"
#include "MsoUtils.h"

#include <ReactUWP/CreateUwpModules.h>
#include <Threading/MessageDispatchQueue.h>
//#include "BytecodeHelpers.h"
//#include "CxxModuleProviderRegistry.h"
//#include "FastMessageQueueThread.h"
//#include "JSExceptionCallbackFactory.h"
//#include "MemoryTrackerImpl.h"
//#include "PlatformBundleHelpers.h"
//#include "RekaNativeModule.h"
//#include "UIManagerProviderRegistry_Win.h"
//
//#include "etw/manifest_reactNativeHost.h"
//#include "etw/etwreactnativehost_shim.h"
//
//#include <core/threading.h>
//#include <ReactNativeHost/ReactErrorProvider.h>
//#include <telemetryRuleEngineApi/SimpleTelemetryClient.h>
//#include <ShlwApi.h>

#include <dispatchQueue/dispatchQueue.h>

namespace Mso::React {

//=============================================================================================
// LoadedCallbackGuard ensures that the OnReactInstanceLoaded is always called.
// It calls OnReactInstanceLoaded in destructor with a cancellation error.
// If loading was previously succeeded this call with an error code is ignored.
//=============================================================================================

struct LoadedCallbackGuard {
  LoadedCallbackGuard(ReactInstanceWin &reactInstance) noexcept : m_reactInstance{&reactInstance} {}

  LoadedCallbackGuard(const LoadedCallbackGuard &other) = delete;
  LoadedCallbackGuard &operator=(const LoadedCallbackGuard &other) = delete;

  LoadedCallbackGuard(LoadedCallbackGuard &&other) = default;
  LoadedCallbackGuard &operator=(LoadedCallbackGuard &&other) = default;

  ~LoadedCallbackGuard() noexcept {
    if (m_reactInstance) {
      m_reactInstance->OnReactInstanceLoaded(Mso::CancellationErrorProvider().MakeErrorCode(true));
    }
  }

 private:
  Mso::CntPtr<ReactInstanceWin> m_reactInstance;
};

//=============================================================================================
// ReactInstanceWin implementation
//=============================================================================================

ReactInstanceWin::ReactInstanceWin(
    IReactHost &reactHost,
    ReactOptions &&options,
    Mso::Promise<void> &&whenLoaded) noexcept
    : Super{reactHost.NativeQueue()},
      m_weakReactHost{&reactHost},
      m_options{std::move(options)},
      m_whenLoaded{std::move(whenLoaded)} {}

ReactInstanceWin::~ReactInstanceWin() noexcept {}

//! Initialize() is called from the native queue.
void ReactInstanceWin::Initialize() noexcept {
  InitJSMessageThread();
  InitNativeMessageThread();
  // TODO: [vmorozov] InitUIMessageThread();

  // TODO: [vmorozov] InitUIManager();

  m_legacyReactInstance =
      std::make_shared<react::uwp::UwpReactInstanceProxy>(this, Mso::Copy(m_options.LegacySettings));

  // Objects that must be created on the UI thread
  Mso::DispatchQueue::MainUIQueue().Post([weakThis = Mso::WeakPtr{this}]() noexcept {
    if (auto strongThis = weakThis.GetStrongPtr()) {
      auto const &legacyFuture = strongThis->m_legacyReactInstance;
      strongThis->m_deviceInfo = std::make_shared<react::uwp::DeviceInfo>(legacyFuture);
      strongThis->m_appState = std::make_shared<react::uwp::AppState>(legacyFuture);
      strongThis->m_appTheme =
          std::make_shared<react::uwp::AppTheme>(legacyFuture, strongThis->m_uiMessageThread.LoadWithLock());
      // strongThis->m_i18nInfo = react::uwp:I18nModule::GetI18nInfo();
    }
  });

  auto cxxModulesProviders = GetCxxModuleProviders();

  auto devSettings = std::make_shared<facebook::react::DevSettings>();
  devSettings->useJITCompilation = m_options.EnableJITCompilation;
  devSettings->debugHost = GetDebugHost();
  devSettings->debugBundlePath = m_options.DeveloperSettings.SourceBundlePath;
  devSettings->liveReloadCallback = GetLiveReloadCallback();
  devSettings->errorCallback = GetErrorCallback();
  devSettings->loggingCallback = GetLoggingCallback();
  devSettings->jsExceptionCallback = GetJSExceptionCallback();
  devSettings->useDirectDebugger = m_options.DeveloperSettings.UseDirectDebugger;
  devSettings->debuggerBreakOnNextLine = m_options.DeveloperSettings.DebuggerBreakOnNextLine;
  devSettings->debuggerPort = m_options.DeveloperSettings.DebuggerPort;
  devSettings->debuggerRuntimeName = m_options.DeveloperSettings.DebuggerRuntimeName;
  devSettings->useWebDebugger = m_options.DeveloperSettings.UseWebDebugger;
  // devSettings->memoryTracker = GetMemoryTracker();
  // Now that ReactNativeWindows is building outside devmain, it is missing
  // fix given by PR https://github.com/microsoft/react-native-windows/pull/2624 causing
  // regression. We're turning off console redirection till the fix is available in devmain.
  // Bug https://office.visualstudio.com/DefaultCollection/OC/_workitems/edit/3441551 is tracking this
  devSettings->debuggerConsoleRedirection = false; // JSHost::ChangeGate::ChakraCoreDebuggerConsoleRedirection();

  // We need to keep the instance wrapper alive as its destruction shuts down the native queue.
  auto instanceWrapper = facebook::react::CreateReactInstance(
      Mso::Copy(m_options.SDXBasePath),
      std::move(cxxModulesProviders),
      m_uiManager.Load(),
      m_jsMessageThread.Load(),
      m_nativeMessageThread.Load(),
      std::move(devSettings));

  m_instance.Exchange(Mso::Copy(instanceWrapper->GetInstance()));
  m_instanceWrapper.Exchange(std::move(instanceWrapper));

  if (auto onCreated = m_options.OnInstanceCreated.Get()) {
    onCreated->Invoke(*this);
  }

  LoadJSBundles();
}

void ReactInstanceWin::LoadJSBundles() noexcept {
  //
  // We use m_jsMessageThread to load JS bundles synchronously. In that case we only load
  // them if the m_jsMessageThread is not shut down (quitSynchronous() is not called).
  // After the load we call OnReactInstanceLoaded callback on native queue.
  //
  // Note that the instance could be destroyed while we are loading JS Bundles.
  // Though, the JS engine is not destroyed until this work item is not finished.
  // Thus, we check the m_isDestroyed flag to see if we should do an early exit.
  // Also, since we have to guarantee that the OnReactInstanceLoaded callback is called before
  // the OnReactInstanceDestroyed callback, the OnReactInstanceLoaded is called right before the
  // OnReactInstanceDestroyed callback in the Destroy() method. In that case any OnReactInstanceLoaded
  // calls after we finish this JS message queue work item is ignored.
  //
  // The LoadedCallbackGuard is used for the case when runOnQueue does not execute the lambda
  // before destroying it. It may happen if the m_jsMessageThread is already shutdown.
  // In that case, the LoadedCallbackGuard notifies about cancellation by calling OnReactInstanceLoaded.
  // The OnReactInstanceLoaded internally only accepts the first call and ignores others.
  //

  if (!m_options.DeveloperSettings.UseWebDebugger) {
    m_jsMessageThread.Load()->runOnQueue(
        [weakThis = Mso::WeakPtr{this},
         loadCallbackGuard = Mso::MakeMoveOnCopyWrapper(LoadedCallbackGuard{*this})]() noexcept {
          // TODO: Implement
          // if (auto strongThis = weakThis.GetStrongPtr()) {
          //  auto instance = strongThis->m_instance.LoadWithLock();
          //  auto instanceWrapper = strongThis->m_instanceWrapper.LoadWithLock();
          //  if (!instance || !instanceWrapper) {
          //    return;
          //  }

          //  auto &options = strongThis->m_options;
          //  auto jsBundleInfos = MakePlatformBundleInfos(options.JSBundles);

          //  try {
          //    // Standard JS bundle load (i.e. non-web debugging)
          //    for (auto &jsBundleInfo : jsBundleInfos) {
          //      // Do an early exit if instance is already destroyed.
          //      if (strongThis->m_isDestroyed) {
          //        return;
          //      }

          //      instance->loadScriptFromString(
          //          std::move(jsBundleInfo.Bundle),
          //          jsBundleInfo.Version,
          //          std::move(jsBundleInfo.BundleUrl),
          //          /*synchronously:*/ true,
          //          std::move(jsBundleInfo.BytecodePath));
          //    }

          //    // Legacy Support
          //    // TODO: Remove after all users are switched to new JSBundles API
          //    std::string userJSBundleFilePath = GetJSBundleFilePath(options.SDXBasePath, options.Identity);
          //    if (PathFileExistsA(userJSBundleFilePath.c_str())) {
          //      if (options.JSBundles.size() == 0 ||
          //          _stricmp(
          //              userJSBundleFilePath.c_str(),
          //              options.JSBundles[options.JSBundles.size() - 1]->Info().FileName.c_str()) != 0) {
          //        // Do an early exit if instance is already destroyed.
          //        if (strongThis->m_isDestroyed) {
          //          return;
          //        }

          //        // AssertSz(false, ("JS Bundle is being loaded through Legacy ReactOptions.Identity Support. Please
          //        // switch to ReactOptions.JSBundles. JS Bundle: " + userJSBundleFilePath).c_str());
          //        instanceWrapper->loadBundleSync(Mso::Copy(options.Identity));
          //      }
          //    }
          //  } catch (...) {
          //    strongThis->OnReactInstanceLoaded(Mso::ExceptionErrorProvider().MakeErrorCode(std::current_exception()));
          //    return;
          //  }

          //  // All JS bundles successfully loaded.
          //  strongThis->OnReactInstanceLoaded(Mso::ErrorCode{});
          //}
        });
  } else {
    // Web Debugging
    // TODO: implement
    // auto instance = m_instance.LoadWithLock();
    // auto instanceWrapper = m_instanceWrapper.LoadWithLock();
    // auto jsBundleInfos = MakePlatformBundleInfos(m_options.JSBundles);

    //// Place platform bundles as (really big) strings into a JS global array.
    //// A polyfill in JS bundle will pick up and `eval()` them.
    //// TODO: [vmorozov][OC:3531318] Stop using folly::dynamic
    // folly::dynamic platformBundleArray = folly::dynamic::array;
    // for (size_t i = 0; i < jsBundleInfos.size(); i++) {
    //  // We make a string copy here. Hopefully we can improve it and avoid it in future.
    //  // TODO: [vmorozov][OC:3531324] Avoid string copy
    //  platformBundleArray.push_back(jsBundleInfos[i].Bundle->c_str());
    //}

    // std::string userJSBundleFilePath = GetJSBundleFilePath(m_options.SDXBasePath, m_options.Identity);
    // if (m_options.JSBundles.size() > 0 &&
    //    _stricmp(
    //        userJSBundleFilePath.c_str(),
    //        m_options.JSBundles[m_options.JSBundles.size() - 1]->Info().FileName.c_str()) != 0) {
    //  if (PathFileExistsA(userJSBundleFilePath.c_str())) {
    //    // JS Bundle does not exist in ReactOptions.JSBundle, but exists through Identity. Debugging and Live Reload
    //    are
    //    // supported.
    //    AssertSzTag(
    //        false,
    //        ("Web Debugging and Live Reload JS Bundle is being loaded through Legacy ReactOptions.Identity Support.
    //        Please switch to ReactOptions.JSBundles. JS Bundle: " +
    //         userJSBundleFilePath)
    //            .c_str(),
    //        0x0285e289 /* tag_c74kj */);
    //    instance->setGlobalVariable(
    //        "__platformBundles",
    //        std::make_unique<facebook::react::JSBigStdString>(folly::toJson(platformBundleArray)));
    //    instanceWrapper->loadBundle(Mso::Copy(m_options.Identity));
    //  } else {
    //    // JS Bundle exists in ReactOptions.JSBundle, but not in Identity. Debugging and Live Reload are not
    //    supported.
    //    // TODO: [vmorozov][vmorozov:3531328] Address this scenario
    //    AssertSzTag(
    //        false,
    //        "Web Debugging and Live Reload is not yet supported for ReactOptions.JSBundles. Tracked by:
    //        2960589:Support Web Debugging and Live Reload for ReactOptions.JSBundles", 0x0285e28a /* tag_c74kk */);
    //  }
    //} else {
    //  // JS Bundle present in ReactOptions.JSBundle and through Identity. Debugging and Live Reload are supported.
    //  instance->setGlobalVariable(
    //      "__platformBundles", std::make_unique<facebook::react::JSBigStdString>(folly::toJson(platformBundleArray)));
    //  instanceWrapper->loadBundle(Mso::Copy(m_options.Identity));
    //}

    // m_jsMessageThread.Load()->runOnQueue(
    //    [weakThis = Mso::MakeWeakPtr(this),
    //     loadCallbackGuard = Mso::MakeMoveOnCopyWrapper(LoadedCallbackGuard{*this})]() noexcept {
    //      if (auto strongThis = weakThis.GetStrongPtr()) {
    //        // All JS bundles successfully loaded.
    //        strongThis->OnReactInstanceLoaded(Mso::ErrorCode{});
    //      }
    //    });
  }
}

void ReactInstanceWin::OnReactInstanceLoaded(const Mso::ErrorCode &errorCode) noexcept {
  if (!m_isLoaded) {
    Queue().InvokeElsePost([weakThis = Mso::WeakPtr{this}, errorCode]() noexcept {
      if (auto strongThis = weakThis.GetStrongPtr()) {
        if (!strongThis->m_isLoaded) {
          strongThis->m_isLoaded = true;
          if (auto onLoaded = strongThis->m_options.OnInstanceLoaded.Get()) {
            onLoaded->Invoke(*strongThis, errorCode);
          }

          strongThis->m_whenLoaded.SetValue();
        }
      }
    });
  }
}

Mso::Future<void> ReactInstanceWin::Destroy() noexcept {
  // This method must be called from the native queue.
  VerifyIsInQueueElseCrash();

  if (m_isDestroyed) {
    return m_whenDestroyed.AsFuture();
  }

  m_isDestroyed = true;

  if (!m_isLoaded) {
    OnReactInstanceLoaded(Mso::CancellationErrorProvider().MakeErrorCode(true));
  }

  if (auto onDestroyed = m_options.OnInstanceDestroyed.Get()) {
    onDestroyed->Invoke(*this);
  }

  // Make sure that the instance is not destroyed yet
  if (auto instance = m_instance.Exchange(nullptr)) {
    // Release the message queues before the ui manager and instance.
    m_nativeMessageThread.Exchange(nullptr);
    m_jsMessageThread.Exchange(nullptr);
    m_uiManager.Exchange(nullptr);
    m_instanceWrapper.Exchange(nullptr);
    m_jsDispatchQueue.Exchange(nullptr);
  }

  return m_whenDestroyed.AsFuture();
}

const ReactOptions &ReactInstanceWin::Options() const noexcept {
  return m_options;
}

ReactInstanceState ReactInstanceWin::State() const noexcept {
  return m_state;
}

std::string ReactInstanceWin::LastErrorMessage() const noexcept {
  std::lock_guard lock{m_mutex};
  return m_errorMessage;
}

void ReactInstanceWin::InitJSMessageThread() noexcept {
  // Use the explicit JSQueue if it is provided.
  const auto &properties = m_options.Properties;
  auto jsDispatchQueue = Mso::DispatchQueue{properties.Get(JSDispatchQueueProperty)};
  if (jsDispatchQueue) {
    VerifyElseCrashSz(jsDispatchQueue.IsSerial(), "JS Queue must be sequential");
  } else {
    // Currently we have to use Looper DispatchQueue because our JS Engine based on Chakra uses thread local storage.
    jsDispatchQueue = Mso::DispatchQueue::MakeLooperQueue();
  }

  // Create MessageQueueThread for the DispatchQueue
  VerifyElseCrashSz(jsDispatchQueue, "m_jsDispatchQueue must not be null");
  m_jsMessageThread.Exchange(std::make_shared<MessageDispatchQueue>(
      jsDispatchQueue, Mso::MakeWeakMemberFunctor(this, &ReactInstanceWin::OnError), Mso::Copy(m_whenDestroyed)));
  m_jsDispatchQueue.Exchange(std::move(jsDispatchQueue));
}

void ReactInstanceWin::InitNativeMessageThread() noexcept {
  // Native queue was already given us in constructor.
  m_nativeMessageThread.Exchange(
      std::make_shared<MessageDispatchQueue>(Queue(), Mso::MakeWeakMemberFunctor(this, &ReactInstanceWin::OnError)));
}

void ReactInstanceWin::InitUIMessageThread() noexcept {
  // Native queue was already given us in constructor.
  m_uiMessageThread.Exchange(std::make_shared<MessageDispatchQueue>(
      Mso::DispatchQueue::MainUIQueue(), Mso::MakeWeakMemberFunctor(this, &ReactInstanceWin::OnError)));
}

void ReactInstanceWin::InitUIManager() noexcept {
  // We assume everyone is running with NetUI UIManager.
  // If we couldn't find a NetUI UIManager, we are initializing an instance used for hosting a UIless scenario, such as
  // UDF
  // TODO: implement
  // auto uiManagerProvider = Mso::JSHost::GetRegisteredUIManagerProvider("NetUI");
  // auto uiManager = uiManagerProvider ? uiManagerProvider->CreateUIManager(*this) : nullptr;
  // m_uiManager.Exchange(std::move(uiManager));
}

CxxModuleProviders ReactInstanceWin::GetCxxModuleProviders() noexcept {
  CxxModuleProviders cxxModuleProviders;
  // TODO: implement
  // for (const auto &cxxModuleName : m_options.CxxModuleNames) {
  //  auto registeredCxxModuleProvider = Mso::JSHost::GetRegisteredCxxModuleProvider(cxxModuleName);
  //  VerifyElseCrashSzTag(
  //      registeredCxxModuleProvider,
  //      "Could not find the registeredCxxModuleProvider for the specified cxx module name",
  //      0x0285e28d /* tag_c74kn */);
  //  auto cxxModuleDescriptions = registeredCxxModuleProvider->GetModules(*this);
  //  for (const auto &cxxModuleDescription : cxxModuleDescriptions) {
  //    std::string moduleName{std::get<0>(cxxModuleDescription)};
  //    facebook::xplat::module::CxxModule::Provider cxxModuleProvider(
  //        [factory = std::get<1>(cxxModuleDescription)]() { return factory(); });
  //    std::shared_ptr<facebook::react::MessageQueueThread> moduleMessageThread{std::get<2>(cxxModuleDescription)};
  //    if (!moduleMessageThread) {
  //      moduleMessageThread = m_nativeMessageThread.Load();
  //    }

  //    cxxModuleProviders.emplace_back(
  //        std::move(moduleName), std::move(cxxModuleProvider), std::move(moduleMessageThread));
  //  }
  //}

  return cxxModuleProviders;
}

facebook::react::NativeLoggingHook ReactInstanceWin::GetLoggingCallback() noexcept {
  if (m_options.OnLogging) {
    return [logCallback = m_options.OnLogging](facebook::react::RCTLogLevel logLevel, const char *message) {
      logCallback(static_cast<LogLevel>(logLevel), message);
    };
  } else {
    // When no logging callback was specified, use a default one in DEBUG builds
#if DEBUG
    return [telemetryTag{m_options.Identity}](facebook::react::RCTLogLevel logLevel, const char *message) {
      std::ostringstream ss;
      ss << "ReactNative ['" << telemetryTag << "'] (";
      switch (logLevel) {
        case facebook::react::RCTLogLevel::Trace:
          ss << "trace";
          break;
        case facebook::react::RCTLogLevel::Info:
          ss << "info";
          break;
        case facebook::react::RCTLogLevel::Warning:
          ss << "warning";
          break;
        case facebook::react::RCTLogLevel::Error:
          ss << "error";
          break;
        case facebook::react::RCTLogLevel::Fatal:
          ss << "fatal";
          break;
      }
      ss << "): '" << message << "'\n";
      OutputDebugStringA(ss.str().c_str());
    };
#endif
  }

  return facebook::react::NativeLoggingHook{};
}

std::function<void(facebook::react::JSExceptionInfo &&)> ReactInstanceWin::GetJSExceptionCallback() noexcept {
  // TODO: implement
  // if (m_options.OnJSException) {
  //  return CreateExceptionCallback(Mso::Copy(m_options.OnJSException));
  //}
  return {};
}

std::function<void()> ReactInstanceWin::GetLiveReloadCallback() noexcept {
  // Live reload is enabled if we provide a callback function.
  if (m_options.DeveloperSettings.UseLiveReload) {
    return Mso::MakeWeakMemberStdFunction(this, &ReactInstanceWin::OnLiveReload);
  }
  return std::function<void()>{};
}

std::string ReactInstanceWin::GetDebugHost() noexcept {
  std::string debugHost;
  const ReactDevOptions &devOptions = m_options.DeveloperSettings;
  if (!devOptions.DebugHost.empty()) {
    debugHost = devOptions.DebugHost;
  } else {
    debugHost = !devOptions.SourceBundleHost.empty() ? devOptions.SourceBundleHost : "localhost";
    debugHost.append(":");
    debugHost.append(!devOptions.SourceBundlePort.empty() ? devOptions.SourceBundlePort : "8081");
  }
  return debugHost;
}

std::string ReactInstanceWin::GetBytecodeFileName() noexcept {
  // use bytecode caching if enabled and not debugging
  // (ChakraCore debugging does not work when bytecode caching is enabled)
  // TODO: implement
  // bool useByteCode = Mso::React::BytecodeOptimizationEnabled() && !m_options.DeveloperSettings.UseDirectDebugger;
  // return useByteCode ? Mso::React::GetBytecodeFilePath(m_options.Identity) : "";
  return "";
}

std::function<void(std::string)> ReactInstanceWin::GetErrorCallback() noexcept {
  return Mso::MakeWeakMemberStdFunction(this, &ReactInstanceWin::OnErrorWithMessage);
}

void ReactInstanceWin::OnErrorWithMessage(const std::string &errorMessage) noexcept {
  // TODO: implement
  //  OnError(Mso::React::ReactErrorProvider().MakeErrorCode(Mso::React::ReactError{errorMessage.c_str()}));
}

void ReactInstanceWin::OnError(const Mso::ErrorCode &errorCode) noexcept {
  InvokeInQueue([this, errorCode]() noexcept { m_options.OnError(errorCode); });
}

void ReactInstanceWin::OnLiveReload() noexcept {
  if (auto reactHost = m_weakReactHost.GetStrongPtr()) {
    reactHost->ReloadInstance();
  }
}

void ReactInstanceWin::CallJsFunction(
    std::string &&moduleName,
    std::string &&method,
    folly::dynamic &&params) noexcept {
  // callJSFunction can be called from any thread. The native bridge will post the call to the right queue internally.
  if (auto instance = m_instance.LoadWithLock()) {
    instance->callJSFunction(std::move(moduleName), std::move(method), std::move(params));
  }
}

void ReactInstanceWin::DispatchEvent(int64_t viewTag, std::string &&eventName, folly::dynamic &&eventData) noexcept {
  VerifyElseCrashSz(false, "Implement");
}

facebook::react::INativeUIManager *ReactInstanceWin::NativeUIManager() noexcept {
  VerifyElseCrashSz(false, "Implement");
}

std::shared_ptr<facebook::react::Instance> ReactInstanceWin::GetInnerInstance() noexcept {
  return m_instance.LoadWithLock();
}

std::string ReactInstanceWin::GetBundleRootPath() noexcept {
  VerifyElseCrashSz(false, "Implement");
}

std::shared_ptr<react::uwp::IReactInstance> ReactInstanceWin::UwpReactInstance() noexcept {
  VerifyElseCrashSz(false, "Implement");
}

void ReactInstanceWin::AttachMeasuredRootView(
    facebook::react::IReactRootView *rootView,
    std::string const &initialProps) noexcept {
  VerifyElseCrashSz(false, "Implement");
}

void ReactInstanceWin::DetachRootView(facebook::react::IReactRootView *rootView) noexcept {
  VerifyElseCrashSz(false, "Implement");
}

Mso::CntPtr<IReactInstanceInternal>
MakeReactInstance(IReactHost &reactHost, ReactOptions &&options, Mso::Promise<void> &&whenLoaded) noexcept {
  return Mso::Make<ReactInstanceWin, IReactInstanceInternal>(reactHost, std::move(options), std::move(whenLoaded));
}

} // namespace Mso::React

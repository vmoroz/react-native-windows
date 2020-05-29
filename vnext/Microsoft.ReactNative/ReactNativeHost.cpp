// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactNativeHost.h"
#include "ReactNativeHost.g.cpp"

#include "ReactPackageBuilder.h"
#include "RedBox.h"

#include <errorCode/hresultErrorProvider.h>
#include <winrt/Windows.Foundation.Collections.h>
#include "ReactInstanceSettings.h"

using namespace winrt;
using namespace Windows::Foundation::Collections;

#ifndef CORE_ABI
using namespace xaml;
using namespace xaml::Controls;
#endif

namespace winrt::Microsoft::ReactNative::implementation {

struct AsyncActionFutureAdapter : implements<AsyncActionFutureAdapter, IAsyncAction, IAsyncInfo> {
  using AsyncStatus = Windows::Foundation::AsyncStatus;
  using AsyncCompletedHandler = Windows::Foundation::AsyncActionCompletedHandler;

  AsyncActionFutureAdapter(Mso::Future<void> &&future) noexcept : m_future{std::move(future)} {
    m_future.Then<Mso::Executors::Inline>([weakThis = get_weak()](Mso::Maybe<void> && result) noexcept {
      if (auto strongThis = weakThis.get()) {
        AsyncCompletedHandler handler;
        {
          slim_lock_guard const guard(strongThis->m_lock);
          if (strongThis->m_status == AsyncStatus::Started) {
            if (result.IsValue()) {
              strongThis->m_status = AsyncStatus::Completed;
              if (strongThis->m_completedAssigned) {
                handler = std::move(strongThis->m_completed);
              }
            } else {
              strongThis->m_status = AsyncStatus::Error;
              strongThis->m_error = result.GetError();
            }
          }
        }

        if (handler) {
          invoke(handler, *strongThis, AsyncStatus::Completed);
        }
      }
    });
  }

  void Completed(AsyncCompletedHandler const &handler) {
    AsyncStatus status;

    {
      slim_lock_guard const guard(m_lock);

      if (m_completedAssigned) {
        throw hresult_illegal_delegate_assignment();
      }

      m_completedAssigned = true;

      if (m_status == AsyncStatus::Started) {
        m_completed = impl::make_agile_delegate(handler);
        return;
      }

      status = m_status;
    }

    if (handler) {
      invoke(handler, *this, status);
    }
  }

  auto Completed() noexcept {
    slim_lock_guard const guard(m_lock);
    return m_completed;
  }

  uint32_t Id() const noexcept {
    return 1;
  }

  AsyncStatus Status() noexcept {
    slim_lock_guard const guard(m_lock);
    return m_status;
  }

  hresult ErrorCode() noexcept {
    try {
      slim_lock_guard const guard(m_lock);
      RethrowIfFailed();
      return 0;
    } catch (...) {
      return to_hresult();
    }
  }

  void Cancel() noexcept {
    slim_lock_guard const guard(m_lock);

    if (m_status == AsyncStatus::Started) {
      m_status = AsyncStatus::Canceled;
      m_error = Mso::HResultErrorProvider().MakeErrorCode(winrt::impl::error_canceled);
    }
  }

  void Close() noexcept {
    Mso::Future<void> future;
    {
      slim_lock_guard const guard(m_lock);
      future = std::move(future);
    }
  }

  void GetResults() {
    slim_lock_guard const guard(m_lock);

    if (m_status == AsyncStatus::Completed) {
      return;
    }

    RethrowIfFailed();
    VerifyElseCrash(m_status == AsyncStatus::Started);
    throw hresult_illegal_method_call();
  }

 private:
  void RethrowIfFailed() const {
    if (m_status == AsyncStatus::Error || m_status == AsyncStatus::Canceled) {
      m_error.Throw();
    }
  }

 private:
  slim_mutex m_lock;
  Mso::Future<void> m_future;
  AsyncStatus m_status{AsyncStatus::Started};
  Mso::ErrorCode m_error{};
  AsyncCompletedHandler m_completed;
  bool m_completedAssigned{false};
};

ReactNativeHost::ReactNativeHost() noexcept : m_reactHost{Mso::React::MakeReactHost()} {
#if _DEBUG
  facebook::react::InitializeLogging([](facebook::react::RCTLogLevel /*logLevel*/, const char *message) {
    std::string str = std::string("ReactNative:") + message;
    OutputDebugStringA(str.c_str());
  });
#endif
}

IVector<IReactPackageProvider> ReactNativeHost::PackageProviders() noexcept {
  return InstanceSettings().PackageProviders();
}

ReactNative::ReactInstanceSettings ReactNativeHost::InstanceSettings() noexcept {
  if (!m_instanceSettings) {
    m_instanceSettings = make<ReactInstanceSettings>();
  }

  return m_instanceSettings;
}

void ReactNativeHost::InstanceSettings(ReactNative::ReactInstanceSettings const &value) noexcept {
  m_instanceSettings = value;
}

IAsyncAction ReactNativeHost::LoadInstance() noexcept {
  return ReloadInstance();
}

IAsyncAction ReactNativeHost::ReloadInstance() noexcept {
#ifndef CORE_ABI
  auto modulesProvider = std::make_shared<NativeModulesProvider>();

  auto viewManagersProvider = std::make_shared<ViewManagersProvider>();

  if (!m_packageBuilder) {
    m_packageBuilder = make<ReactPackageBuilder>(modulesProvider, viewManagersProvider);

    if (auto packageProviders = InstanceSettings().PackageProviders()) {
      for (auto const &packageProvider : packageProviders) {
        packageProvider.CreatePackage(m_packageBuilder);
      }
    }
  }

  react::uwp::ReactInstanceSettings legacySettings{};
  legacySettings.BundleRootPath = to_string(m_instanceSettings.BundleRootPath());
  legacySettings.ByteCodeFileUri = to_string(m_instanceSettings.ByteCodeFileUri());
  legacySettings.DebugBundlePath = to_string(m_instanceSettings.DebugBundlePath());
  legacySettings.DebugHost = to_string(m_instanceSettings.DebugHost());
  legacySettings.EnableByteCodeCaching = m_instanceSettings.EnableByteCodeCaching();
  legacySettings.EnableDeveloperMenu = m_instanceSettings.EnableDeveloperMenu();
  legacySettings.EnableJITCompilation = m_instanceSettings.EnableJITCompilation();
  legacySettings.UseDirectDebugger = m_instanceSettings.UseDirectDebugger();
  legacySettings.DebuggerBreakOnNextLine = m_instanceSettings.DebuggerBreakOnNextLine();
  legacySettings.UseJsi = m_instanceSettings.UseJsi();
  legacySettings.UseFastRefresh = m_instanceSettings.UseFastRefresh();
  legacySettings.UseLiveReload = m_instanceSettings.UseLiveReload();
  legacySettings.UseWebDebugger = m_instanceSettings.UseWebDebugger();
  legacySettings.DebuggerPort = m_instanceSettings.DebuggerPort();
  legacySettings.SourceBundleHost = to_string(m_instanceSettings.SourceBundleHost());
  legacySettings.SourceBundlePort = m_instanceSettings.SourceBundlePort();

  if (m_instanceSettings.RedBoxHandler()) {
    legacySettings.RedBoxHandler = std::move(Mso::React::CreateRedBoxHandler(m_instanceSettings.RedBoxHandler()));
  }

  Mso::React::ReactOptions reactOptions{};
  reactOptions.Properties = m_instanceSettings.Properties();
  reactOptions.Notifications = m_instanceSettings.Notifications();
  reactOptions.DeveloperSettings.IsDevModeEnabled = legacySettings.EnableDeveloperMenu;
  reactOptions.DeveloperSettings.SourceBundleName = legacySettings.DebugBundlePath;
  reactOptions.DeveloperSettings.UseWebDebugger = legacySettings.UseWebDebugger;
  reactOptions.DeveloperSettings.UseDirectDebugger = legacySettings.UseDirectDebugger;
  reactOptions.DeveloperSettings.DebuggerBreakOnNextLine = legacySettings.DebuggerBreakOnNextLine;
  reactOptions.DeveloperSettings.UseFastRefresh = legacySettings.UseFastRefresh;
  reactOptions.DeveloperSettings.UseLiveReload = legacySettings.UseLiveReload;
  reactOptions.EnableJITCompilation = legacySettings.EnableJITCompilation;
  reactOptions.DeveloperSettings.DebugHost = legacySettings.DebugHost;
  reactOptions.BundleRootPath = legacySettings.BundleRootPath;
  reactOptions.DeveloperSettings.DebuggerPort = legacySettings.DebuggerPort;
  reactOptions.RedBoxHandler = legacySettings.RedBoxHandler;
  reactOptions.DeveloperSettings.SourceBundleHost = legacySettings.SourceBundleHost;
  reactOptions.DeveloperSettings.SourceBundlePort =
      legacySettings.SourceBundlePort != 0 ? std::to_string(legacySettings.SourceBundlePort) : "";

  reactOptions.LegacySettings = std::move(legacySettings);

  reactOptions.ModuleProvider = modulesProvider;
  reactOptions.ViewManagerProvider = viewManagersProvider;

  std::string jsBundleFile = to_string(m_instanceSettings.JavaScriptBundleFile());
  std::string jsMainModuleName = to_string(m_instanceSettings.JavaScriptMainModuleName());
  if (jsBundleFile.empty()) {
    if (!jsMainModuleName.empty()) {
      jsBundleFile = jsMainModuleName;
    } else {
      jsBundleFile = "index.windows";
    }
  }

  reactOptions.Identity = jsBundleFile;

  return make<AsyncActionFutureAdapter>(m_reactHost->ReloadInstanceWithOptions(std::move(reactOptions)));
#else
  // Core ABI work needed
  VerifyElseCrash(false);
#endif
}

IAsyncAction ReactNativeHost::UnloadInstance() noexcept {
  return make<AsyncActionFutureAdapter>(m_reactHost->UnloadInstance());
}

Mso::React::IReactHost *ReactNativeHost::ReactHost() noexcept {
  return m_reactHost.Get();
}

} // namespace winrt::Microsoft::ReactNative::implementation

#pragma once

#include "ReactInstanceSettings.g.h"

#ifndef REACT_DEFAULT_USE_DEVELOPER_SUPPORT
#if _DEBUG
#define REACT_DEFAULT_USE_DEVELOPER_SUPPORT true
#else
#define REACT_DEFAULT_USE_DEVELOPER_SUPPORT false
#endif // _DEBUG
#endif // REACT_DEFAULT_USE_DEVELOPER_SUPPORT

namespace winrt::Microsoft::ReactNative::implementation {
struct ReactInstanceSettings : ReactInstanceSettingsT<ReactInstanceSettings> {
  ReactInstanceSettings() = default;

  hstring MainComponentName() noexcept;
  void MainComponentName(hstring const &value) noexcept;

  bool UseDeveloperSupport() noexcept;
  void UseDeveloperSupport(bool value) noexcept;

  hstring JavaScriptMainModuleName() noexcept;
  void JavaScriptMainModuleName(hstring const &value) noexcept;

  hstring JavaScriptBundleFile() noexcept;
  void JavaScriptBundleFile(hstring const &value) noexcept;


  bool UseWebDebugger() {
    return m_useWebDebugger;
  }
  void UseWebDebugger(bool value) {
    m_useWebDebugger = value;
  }

  bool UseLiveReload() {
    return m_useLiveReload;
  }
  void UseLiveReload(bool value) {
    m_useLiveReload = value;
  }

  bool UseDirectDebugger() {
    return m_useDirectDebugger;
  }
  void UseDirectDebugger(bool value) {
    m_useDirectDebugger = value;
  }

  bool UseJsi() {
    return m_useJsi;
  }
  void UseJsi(bool value) {
    m_useJsi = value;
  }

  bool EnableJITCompilation() {
    return m_enableJITCompilation;
  }
  void EnableJITCompilation(bool value) {
    m_enableJITCompilation = value;
  }

  bool EnableByteCodeCaching() {
    return m_enableByteCodeCaching;
  }
  void EnableByteCodeCaching(bool value) {
    m_enableByteCodeCaching = value;
  }

  bool EnableDeveloperMenu() {
    return m_enableDeveloperMenu;
  }
  void EnableDeveloperMenu(bool value) {
    m_enableDeveloperMenu = value;
  }

  hstring ByteCodeFileUri() {
    return m_byteCodeFileUri;
  }
  void ByteCodeFileUri(hstring const &value) {
    m_byteCodeFileUri = value;
  }

  hstring DebugHost() {
    return m_debugHost;
  }
  void DebugHost(hstring const &value) {
    m_debugHost = value;
  }

  hstring DebugBundlePath() {
    return m_debugBundlePath;
  }
  void DebugBundlePath(hstring const &value) {
    m_debugBundlePath = value;
  }

  hstring BundleRootPath() {
    return m_bundleRootPath;
  }
  void BundleRootPath(hstring const &value) {
    m_bundleRootPath = value;
  }

 private:
  hstring m_mainComponentName{};
  bool m_useDeveloperSupport{REACT_DEFAULT_USE_DEVELOPER_SUPPORT};
  hstring m_javaScriptMainModuleName{};
  hstring m_javaScriptBundleFile{};


#if _DEBUG
  // TODO: Discuss whether this is where we should have these set by default
  //       versus in the new project template.  This makes it easier for a
  //       debug build to work out of the box.
  bool m_useWebDebugger{true};
  bool m_useLiveReload{true};
#else
  bool m_useWebDebugger{false};
  bool m_useLiveReload{false};
#endif
  bool m_enableDeveloperMenu{false};
  bool m_useDirectDebugger{false};
  bool m_useJsi{true};
  bool m_enableJITCompilation{true};
  bool m_enableByteCodeCaching{false};

  hstring m_byteCodeFileUri{};
  hstring m_debugHost{};
  hstring m_debugBundlePath{};
  hstring m_bundleRootPath{};
};

inline hstring ReactInstanceSettings::MainComponentName() noexcept {
  return m_mainComponentName;
}

inline void ReactInstanceSettings::MainComponentName(hstring const &value) noexcept {
  m_mainComponentName = value;
}

inline bool ReactInstanceSettings::UseDeveloperSupport() noexcept {
  return m_useDeveloperSupport;
}

inline void ReactInstanceSettings::UseDeveloperSupport(bool value) noexcept {
  m_useDeveloperSupport = value;
}

inline hstring ReactInstanceSettings::JavaScriptMainModuleName() noexcept {
  return m_javaScriptMainModuleName;
}

inline void ReactInstanceSettings::JavaScriptMainModuleName(hstring const &value) noexcept {
  m_javaScriptMainModuleName = value;
}

inline hstring ReactInstanceSettings::JavaScriptBundleFile() noexcept {
  return m_javaScriptBundleFile;
}

inline void ReactInstanceSettings::JavaScriptBundleFile(hstring const &value) noexcept {
  m_javaScriptBundleFile = value;
}

} // namespace winrt::Microsoft::ReactNative::implementation

namespace winrt::Microsoft::ReactNative::factory_implementation {
struct ReactInstanceSettings : ReactInstanceSettingsT<ReactInstanceSettings, implementation::ReactInstanceSettings> {};
} // namespace winrt::Microsoft::ReactNative::factory_implementation

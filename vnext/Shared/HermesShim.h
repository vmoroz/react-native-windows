// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <JSI/ScriptStore.h>
#include <cxxreact/MessageQueueThread.h>
#include <jsi/jsi.h>
#include <hermes/hermes_api.h>

//! We do not package hermes.dll for projects that do not require it. We cannot
//! use pure delay-loading to achieve this, since WACK will detect the
//! non-present DLL. Functions in this namespace shim to the Hermes DLL via
//! GetProcAddress.
namespace Microsoft::ReactNative {

class HermesRuntimeConfig {
 public:
  HermesRuntimeConfig &enableDefaultCrashHandler(bool value) noexcept;
  HermesRuntimeConfig &useDirectDebugger(bool value) noexcept;
  HermesRuntimeConfig &debuggerRuntimeName(const std::string &value) noexcept;
  HermesRuntimeConfig &debuggerPort(uint16_t value) noexcept;
  HermesRuntimeConfig &debuggerBreakOnNextLine(bool value) noexcept;
  HermesRuntimeConfig &foregroundTaskRunner(std::shared_ptr<facebook::react::MessageQueueThread> value) noexcept;
  HermesRuntimeConfig &scriptCache(std::unique_ptr<facebook::jsi::PreparedScriptStore> value) noexcept;
  jsr_runtime createRuntime() const noexcept;

 private:
  std::string debuggerRuntimeName_;
  uint16_t debuggerPort_{};
  bool enableDefaultCrashHandler_{};
  bool useDirectDebugger_{};
  bool debuggerBreakOnNextLine_{};
  std::shared_ptr<facebook::react::MessageQueueThread> foregroundTaskRunner_;
  std::shared_ptr<facebook::jsi::PreparedScriptStore> scriptStore_;
};

class HermesShim : public std::enable_shared_from_this<HermesShim> {
 public:
  HermesShim(jsr_runtime runtimeAbiPtr) noexcept;
  ~HermesShim();

  static std::shared_ptr<HermesShim> make(const HermesRuntimeConfig &config) noexcept;

  std::shared_ptr<facebook::jsi::Runtime> getRuntime() const noexcept;

  void dumpCrashData(int fileDescriptor) const noexcept;

  void stopDebugging() noexcept;

  static void enableSamplingProfiler() noexcept;
  static void disableSamplingProfiler() noexcept;
  static void dumpSampledTraceToFile(const std::string &fileName) noexcept;
  void addToProfiling() const noexcept;
  void removeFromProfiling() const noexcept;

  HermesShim(const HermesShim &) = delete;
  HermesShim &operator=(const HermesShim &) = delete;

 private:
  // It must be a raw pointer to avoid circular reference.
  //facebook::jsi::Runtime *jsiRuntime_{};
  jsr_runtime runtime_{};
  napi_env env_{};
  std::shared_ptr<facebook::jsi::Runtime> nodeApiRuntime_;
};

} // namespace Microsoft::ReactNative

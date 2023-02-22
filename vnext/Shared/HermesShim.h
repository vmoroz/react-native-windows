// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <jsi/jsi.h>
#include <napi/hermes_api.h>

//! We do not package hermes.dll for projects that do not require it. We cannot
//! use pure delay-loading to achieve this, since WACK will detect the
//! non-present DLL. Functions in this namespace shim to the Hermes DLL via
//! GetProcAddress.
namespace Microsoft::ReactNative {

class HermesShim : public std::enable_shared_from_this<HermesShim> {
 public:
  HermesShim(hermes_runtime runtimeAbiPtr) noexcept;
  ~HermesShim();

  static std::shared_ptr<HermesShim> make() noexcept;
  static std::shared_ptr<HermesShim> makeWithWER() noexcept;

  std::shared_ptr<facebook::jsi::Runtime> getRuntime() const noexcept;

  void enableDebugging(const char *debuggerRuntimeName) const noexcept;
  void disableDebugging() const noexcept;

  void dumpCrashData(int fileDescriptor) const noexcept;

  static void enableSamplingProfiler() noexcept;
  static void disableSamplingProfiler() noexcept;
  static void dumpSampledTraceToFile(const std::string &fileName) noexcept;
  void addToProfiling() const noexcept;
  void removeFromProfiling() const noexcept;

  HermesShim(const HermesShim &) = delete;
  HermesShim &operator=(const HermesShim &) = delete;

 private:
  // It must be a raw pointer to avoid circular reference.
  facebook::jsi::Runtime *jsiRuntime_{};
  hermes_runtime runtime_{};
};

} // namespace Microsoft::ReactNative

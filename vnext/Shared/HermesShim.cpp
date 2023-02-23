// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "HermesShim.h"
#include "Crash.h"

#define DECLARE_SYMBOL(symbol, importedSymbol) \
  decltype(&importedSymbol) s_##symbol{};      \
  constexpr const char symbol##Symbol[] = #importedSymbol;

#define INIT_SYMBOL(symbol)                                                                            \
  s_##symbol = reinterpret_cast<decltype(s_##symbol)>(GetProcAddress(s_hermesModule, symbol##Symbol)); \
  VerifyElseCrash(s_##symbol);

#define CRASH_ON_ERROR(result) VerifyElseCrash(result == hermes_ok);

namespace Microsoft::ReactNative {

namespace {

DECLARE_SYMBOL(createRuntime, hermes_create_runtime);
DECLARE_SYMBOL(createRuntimeWithWER, hermes_create_runtime_with_wer);
DECLARE_SYMBOL(deleteRuntime, hermes_delete_runtime);
DECLARE_SYMBOL(dumpCrashData, hermes_dump_crash_data);
DECLARE_SYMBOL(samplingProfilerEnable, hermes_sampling_profiler_enable);
DECLARE_SYMBOL(samplingProfilerDisable, hermes_sampling_profiler_disable);
DECLARE_SYMBOL(samplingProfilerAdd, hermes_sampling_profiler_add);
DECLARE_SYMBOL(samplingProfilerRemove, hermes_sampling_profiler_remove);
DECLARE_SYMBOL(samplingProfilerDumpToFile, hermes_sampling_profiler_dump_to_file);

HMODULE s_hermesModule{};
std::once_flag s_hermesLoading;

void EnsureHermesLoaded() noexcept {
  std::call_once(s_hermesLoading, []() {
    VerifyElseCrashSz(!s_hermesModule, "Invalid state: \"hermes.dll\" being loaded again.");

    s_hermesModule = LoadLibrary(L"hermes.dll");
    VerifyElseCrashSz(s_hermesModule, "Could not load \"hermes.dll\"");

    INIT_SYMBOL(createRuntime);
    INIT_SYMBOL(createRuntimeWithWER);
    INIT_SYMBOL(deleteRuntime);
    INIT_SYMBOL(dumpCrashData);
    INIT_SYMBOL(samplingProfilerEnable);
    INIT_SYMBOL(samplingProfilerDisable);
    INIT_SYMBOL(samplingProfilerAdd);
    INIT_SYMBOL(samplingProfilerRemove);
    INIT_SYMBOL(samplingProfilerDumpToFile);
  });
}

} // namespace

HermesRuntimeConfig &HermesRuntimeConfig::enableDefaultCrashHandler(bool /*value*/) noexcept {
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::useDirectDebugger(bool /*value*/) noexcept {
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::debuggerRuntimeName(const std::string & /*value*/) noexcept {
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::debuggerPort(uint16_t /*value*/) noexcept {
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::debuggerBreakOnNextLine(bool /*value*/) noexcept {
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::foregroundTaskRunner(
    std::shared_ptr<facebook::react::MessageQueueThread> /*value*/) noexcept {
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::scriptCache(
    std::unique_ptr<facebook::jsi::PreparedScriptStore> /*value*/) noexcept {
  return *this;
}

HermesShim::HermesShim(hermes_runtime runtime) noexcept : runtime_(runtime) {}

HermesShim::~HermesShim() {
  CRASH_ON_ERROR(s_deleteRuntime(runtime_));
}

/*static*/ std::shared_ptr<HermesShim> HermesShim::make(const HermesRuntimeConfig & /*config*/) noexcept {
  EnsureHermesLoaded();
  hermes_runtime runtime{};
  CRASH_ON_ERROR(s_createRuntime(&runtime));
  return std::make_shared<HermesShim>(runtime);
}

std::shared_ptr<facebook::jsi::Runtime> HermesShim::getRuntime() const noexcept {
  // TODO:
  // return std::shared_ptr<facebook::hermes::HermesRuntime>(nonAbiSafeRuntime_, RuntimeDeleter(shared_from_this()));
  return nullptr;
}

void HermesShim::dumpCrashData(int fileDescriptor) const noexcept {
  CRASH_ON_ERROR(s_dumpCrashData(runtime_, fileDescriptor));
}

void HermesShim::stopDebugging() noexcept {
  // TODO: Implement
}

/*static*/ void HermesShim::enableSamplingProfiler() noexcept {
  EnsureHermesLoaded();
  CRASH_ON_ERROR(s_samplingProfilerEnable());
}

/*static*/ void HermesShim::disableSamplingProfiler() noexcept {
  EnsureHermesLoaded();
  CRASH_ON_ERROR(s_samplingProfilerDisable());
}

/*static*/ void HermesShim::dumpSampledTraceToFile(const std::string &fileName) noexcept {
  EnsureHermesLoaded();
  CRASH_ON_ERROR(s_samplingProfilerDumpToFile(fileName.c_str()));
}
void HermesShim::addToProfiling() const noexcept {
  CRASH_ON_ERROR(s_samplingProfilerAdd(runtime_));
}

void HermesShim::removeFromProfiling() const noexcept {
  CRASH_ON_ERROR(s_samplingProfilerRemove(runtime_));
}

} // namespace Microsoft::ReactNative

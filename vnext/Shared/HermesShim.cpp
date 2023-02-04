// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "HermesShim.h"
#include "Crash.h"

#define INIT_SYMBOL(symbol, symbolName)                                                    \
  symbol = reinterpret_cast<decltype(symbol)>(GetProcAddress(s_hermesModule, symbolName)); \
  VerifyElseCrash(symbol);

#define CRASH_ON_ERROR(result) VerifyElseCrash(result == hermes_ok);

namespace Microsoft::ReactNative {

namespace {

HMODULE s_hermesModule{};

decltype(&hermes_create_runtime) s_createRuntime{};
decltype(&hermes_create_runtime_with_wer) s_createRuntimeWithWER{};
decltype(&hermes_delete_runtime) s_deleteRuntime{};
decltype(&hermes_get_jsi_runtime) s_getJsiRuntime{};
decltype(&hermes_dump_crash_data) s_dumpCrashData{};
decltype(&hermes_sampling_profiler_enable) s_samplingProfilerEnable{};
decltype(&hermes_sampling_profiler_disable) s_samplingProfilerDisable{};
decltype(&hermes_sampling_profiler_add) s_samplingProfilerAdd{};
decltype(&hermes_sampling_profiler_remove) s_samplingProfilerRemove{};
decltype(&hermes_sampling_profiler_dump_to_file) s_samplingProfilerDumpToFile{};

#if _M_X64
constexpr const char createRuntimeSymbol[] = "hermes_create_runtime";
constexpr const char createRuntimeWithWERSymbol[] = "hermes_create_runtime_with_wer";
constexpr const char deleteRuntimeSymbol[] = "hermes_delete_runtime";
constexpr const char getJsiRuntimeSymbol[] = "hermes_get_jsi_runtime";
constexpr const char dumpCrashDataSymbol[] = "hermes_dump_crash_data";
constexpr const char samplingProfilerEnableSymbol[] = "hermes_sampling_profiler_enable";
constexpr const char samplingProfilerDisableSymbol[] = "hermes_sampling_profiler_disable";
constexpr const char samplingProfilerAddSymbol[] = "hermes_sampling_profiler_add";
constexpr const char samplingProfilerRemoveSymbol[] = "hermes_sampling_profiler_remove";
constexpr const char samplingProfilerDumpToFileSymbol[] = "hermes_sampling_profiler_dump_to_file";
#elif _M_ARM64
constexpr const char createRuntimeSymbol[] = "hermes_create_runtime";
constexpr const char createRuntimeWithWERSymbol[] = "hermes_create_runtime_with_wer";
constexpr const char deleteRuntimeSymbol[] = "hermes_delete_runtime";
constexpr const char getJsiRuntimeSymbol[] = "hermes_get_jsi_runtime";
constexpr const char dumpCrashDataSymbol[] = "hermes_dump_crash_data";
constexpr const char samplingProfilerEnableSymbol[] = "hermes_sampling_profiler_enable";
constexpr const char samplingProfilerDisableSymbol[] = "hermes_sampling_profiler_disable";
constexpr const char samplingProfilerAddSymbol[] = "hermes_sampling_profiler_add";
constexpr const char samplingProfilerRemoveSymbol[] = "hermes_sampling_profiler_remove";
constexpr const char samplingProfilerDumpToFileSymbol[] = "hermes_sampling_profiler_dump_to_file";
#elif _M_IX86
constexpr const char createRuntimeSymbol[] = "_hermes_create_runtime";
constexpr const char createRuntimeWithWERSymbol[] = "_hermes_create_runtime_with_wer";
constexpr const char deleteRuntimeSymbol[] = "_hermes_delete_runtime";
constexpr const char getJsiRuntimeSymbol[] = "_hermes_get_jsi_runtime";
constexpr const char dumpCrashDataSymbol[] = "_hermes_dump_crash_data";
constexpr const char samplingProfilerEnableSymbol[] = "_hermes_sampling_profiler_enable";
constexpr const char samplingProfilerDisableSymbol[] = "_hermes_sampling_profiler_disable";
constexpr const char samplingProfilerAddSymbol[] = "_hermes_sampling_profiler_add";
constexpr const char samplingProfilerRemoveSymbol[] = "_hermes_sampling_profiler_remove";
constexpr const char samplingProfilerDumpToFileSymbol[] = "_hermes_sampling_profiler_dump_to_file";
#endif

std::once_flag s_hermesLoading;

void EnsureHermesLoaded() noexcept {
  std::call_once(s_hermesLoading, []() {
    VerifyElseCrashSz(!s_hermesModule, "Invalid state: \"hermes.dll\" being loaded again.");

    s_hermesModule = LoadLibrary(L"hermes.dll");
    VerifyElseCrashSz(s_hermesModule, "Could not load \"hermes.dll\"");

    INIT_SYMBOL(s_createRuntime, createRuntimeSymbol);
    INIT_SYMBOL(s_createRuntimeWithWER, createRuntimeWithWERSymbol);
    INIT_SYMBOL(s_deleteRuntime, deleteRuntimeSymbol);
    INIT_SYMBOL(s_getJsiRuntime, getJsiRuntimeSymbol);
    INIT_SYMBOL(s_dumpCrashData, dumpCrashDataSymbol);
    INIT_SYMBOL(s_samplingProfilerEnable, samplingProfilerEnableSymbol);
    INIT_SYMBOL(s_samplingProfilerDisable, samplingProfilerDisableSymbol);
    INIT_SYMBOL(s_samplingProfilerAdd, samplingProfilerAddSymbol);
    INIT_SYMBOL(s_samplingProfilerRemove, samplingProfilerRemoveSymbol);
    INIT_SYMBOL(s_samplingProfilerDumpToFile, samplingProfilerDumpToFileSymbol);
  });
}

struct RuntimeDeleter {
  RuntimeDeleter(std::shared_ptr<const HermesShim> &&hermesShimPtr) noexcept : hermesShimPtr_(std::move(hermesShimPtr)) {}

  void operator()(facebook::hermes::HermesRuntime * /*runtime*/) {
    // Do nothing. Instead, we rely on the RuntimeDeleter destructor.
  }

 private:
  std::shared_ptr<const HermesShim> hermesShimPtr_;
};

} // namespace

HermesShim::HermesShim(hermes_runtime runtime) noexcept : runtime_(runtime) {
  CRASH_ON_ERROR(s_getJsiRuntime(runtime_, reinterpret_cast<void **>(&nonAbiSafeRuntime_)));
}

HermesShim::~HermesShim() {
  CRASH_ON_ERROR(s_deleteRuntime(runtime_));
}

/*static*/ std::shared_ptr<HermesShim> HermesShim::make() noexcept {
  EnsureHermesLoaded();
  hermes_runtime runtime{};
  CRASH_ON_ERROR(s_createRuntime(&runtime));
  return std::make_shared<HermesShim>(runtime);
}

/*static*/
std::shared_ptr<HermesShim> HermesShim::makeWithWER() noexcept {
  EnsureHermesLoaded();
  hermes_runtime runtime{};
  CRASH_ON_ERROR(s_createRuntimeWithWER(&runtime));
  return std::make_shared<HermesShim>(runtime);
}

std::shared_ptr<facebook::hermes::HermesRuntime> HermesShim::getRuntime() const noexcept {
  return std::shared_ptr<facebook::hermes::HermesRuntime>(nonAbiSafeRuntime_, RuntimeDeleter(shared_from_this()));
}

void HermesShim::dumpCrashData(int fileDescriptor) const noexcept {
  CRASH_ON_ERROR(s_dumpCrashData(runtime_, fileDescriptor));
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

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "HermesShim.h"
#include "Crash.h"

#define INIT_SYMBOL(symbol) \
  s_hermesApi.symbol = reinterpret_cast<decltype(&hermes_##symbol)>(GetSymbolAddress("hermes_" #symbol));

#define CRASH_ON_ERROR(result) VerifyElseCrash(result == hermes_ok);

namespace Microsoft::ReactNative {

namespace {

struct HermesApi {
  decltype(&hermes_create_runtime) create_runtime;
  decltype(&hermes_delete_runtime) delete_runtime;
  decltype(&hermes_get_node_api_env) get_node_api_env;
  decltype(&hermes_dump_crash_data) dump_crash_data;
  decltype(&hermes_sampling_profiler_enable) sampling_profiler_enable;
  decltype(&hermes_sampling_profiler_disable) sampling_profiler_disable;
  decltype(&hermes_sampling_profiler_add) sampling_profiler_add;
  decltype(&hermes_sampling_profiler_remove) sampling_profiler_remove;
  decltype(&hermes_sampling_profiler_dump_to_file) sampling_profiler_dump_to_file;
  decltype(&hermes_create_config) create_config;
  decltype(&hermes_delete_config) delete_config;
  decltype(&hermes_config_enable_default_crash_handler) config_enable_default_crash_handler;
  decltype(&hermes_config_enable_debugger) config_enable_debugger;
  decltype(&hermes_config_set_debugger_runtime_name) config_set_debugger_runtime_name;
  decltype(&hermes_config_set_debugger_port) config_set_debugger_port;
  decltype(&hermes_config_set_debugger_break_on_start) config_set_debugger_break_on_start;
  decltype(&hermes_config_set_task_runner) config_set_task_runner;
  decltype(&hermes_config_set_script_cache) config_set_script_cache;
} s_hermesApi;

HMODULE s_hermesModule{};

void *GetSymbolAddress(const char *symbolName) {
  void *result = GetProcAddress(s_hermesModule, symbolName);
  VerifyElseCrash(result);
  return result;
}

std::once_flag s_hermesLoading;

HermesApi &GetHermesApi() noexcept {
  std::call_once(s_hermesLoading, []() {
    VerifyElseCrashSz(!s_hermesModule, "Invalid state: \"hermes.dll\" being loaded again.");

    s_hermesModule = LoadLibrary(L"hermes.dll");
    VerifyElseCrashSz(s_hermesModule, "Could not load \"hermes.dll\"");

    INIT_SYMBOL(create_runtime);
    INIT_SYMBOL(delete_runtime);
    INIT_SYMBOL(get_node_api_env);
    INIT_SYMBOL(dump_crash_data);
    INIT_SYMBOL(sampling_profiler_enable);
    INIT_SYMBOL(sampling_profiler_disable);
    INIT_SYMBOL(sampling_profiler_add);
    INIT_SYMBOL(sampling_profiler_remove);
    INIT_SYMBOL(sampling_profiler_dump_to_file);
    INIT_SYMBOL(create_config);
    INIT_SYMBOL(delete_config);
    INIT_SYMBOL(config_enable_default_crash_handler);
    INIT_SYMBOL(config_enable_debugger);
    INIT_SYMBOL(config_set_debugger_runtime_name);
    INIT_SYMBOL(config_set_debugger_port);
    INIT_SYMBOL(config_set_debugger_break_on_start);
    INIT_SYMBOL(config_set_task_runner);
    INIT_SYMBOL(config_set_script_cache);
  });

  return s_hermesApi;
}

class HermesTask {
 public:
  HermesTask(
      void *taskData,
      hermes_task_run_cb taskRunCallback,
      hermes_data_delete_cb taskDataDeleteCallback,
      void *deleterData)
      : taskData_(taskData),
        taskRunCallback_(taskRunCallback),
        taskDataDeleteCallback_(taskDataDeleteCallback),
        deleterData_(deleterData) {}

  HermesTask(const HermesTask &other) = delete;
  HermesTask &operator=(const HermesTask &other) = delete;

  ~HermesTask() {
    if (taskDataDeleteCallback_ != nullptr) {
      taskDataDeleteCallback_(taskData_, deleterData_);
    }
  }

  void Run() const {
    if (taskRunCallback_ != nullptr) {
      taskRunCallback_(taskData_);
    }
  }

 private:
  void *taskData_;
  hermes_task_run_cb taskRunCallback_;
  hermes_data_delete_cb taskDataDeleteCallback_;
  void *deleterData_;
};

class HermesTaskRunner {
 public:
  static void Create(hermes_config config, std::shared_ptr<facebook::react::MessageQueueThread> queue) {
    GetHermesApi().config_set_task_runner(config, new HermesTaskRunner(std::move(queue)), &PostTask, &Delete, nullptr);
  }

 private:
  HermesTaskRunner(std::shared_ptr<facebook::react::MessageQueueThread> queue) : queue_(std::move(queue)) {}

  static void HERMES_CDECL PostTask(
      void *taskRunnerData,
      void *taskData,
      hermes_task_run_cb taskRunCallback,
      hermes_data_delete_cb taskDataDeleteCallback,
      void *deleterData) {
    auto task = std::make_shared<HermesTask>(taskData, taskRunCallback, taskDataDeleteCallback, deleterData);
    reinterpret_cast<HermesTaskRunner *>(taskRunnerData)->queue_->runOnQueue([task = std::move(task)] { task->Run(); });
  }

  static void HERMES_CDECL Delete(void *taskRunner, void * /*deleterData*/) {
    delete reinterpret_cast<HermesTaskRunner *>(taskRunner);
  }

 private:
  std::shared_ptr<facebook::react::MessageQueueThread> queue_;
};

} // namespace

HermesRuntimeConfig &HermesRuntimeConfig::enableDefaultCrashHandler(bool value) noexcept {
  enableDefaultCrashHandler_ = value;
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::useDirectDebugger(bool value) noexcept {
  useDirectDebugger_ = value;
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::debuggerRuntimeName(const std::string &value) noexcept {
  debuggerRuntimeName_ = value;
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::debuggerPort(uint16_t value) noexcept {
  debuggerPort_ = value;
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::debuggerBreakOnNextLine(bool value) noexcept {
  debuggerBreakOnNextLine_ = value;
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::foregroundTaskRunner(
    std::shared_ptr<facebook::react::MessageQueueThread> value) noexcept {
  foregroundTaskRunner_ = std::move(value);
  return *this;
}

HermesRuntimeConfig &HermesRuntimeConfig::scriptCache(
    std::unique_ptr<facebook::jsi::PreparedScriptStore> value) noexcept {
  scriptStore_ = std::move(value);
  return *this;
}

hermes_runtime HermesRuntimeConfig::createRuntime() const noexcept {
  HermesApi &api = GetHermesApi();
  hermes_config config{};
  CRASH_ON_ERROR(api.create_config(&config));
  CRASH_ON_ERROR(api.config_enable_default_crash_handler(config, enableDefaultCrashHandler_));
  CRASH_ON_ERROR(api.config_enable_debugger(config, useDirectDebugger_));
  CRASH_ON_ERROR(api.config_set_debugger_runtime_name(config, debuggerRuntimeName_.c_str()));
  CRASH_ON_ERROR(api.config_set_debugger_port(config, debuggerPort_));
  CRASH_ON_ERROR(api.config_set_debugger_break_on_start(config, debuggerBreakOnNextLine_));
  if (foregroundTaskRunner_) {
    HermesTaskRunner::Create(config, foregroundTaskRunner_);
  }
  if (scriptStore_) {
    // CRASH_ON_ERROR(api.config_set_script_cache(config, _debuggerBreakOnNextLine));
  }
  hermes_runtime runtime{};
  CRASH_ON_ERROR(api.create_runtime(config, &runtime));
  CRASH_ON_ERROR(api.delete_config(config));
  return runtime;
}

HermesShim::HermesShim(hermes_runtime runtime) noexcept : runtime_(runtime) {}

HermesShim::~HermesShim() {
  CRASH_ON_ERROR(GetHermesApi().delete_runtime(runtime_));
}

/*static*/ std::shared_ptr<HermesShim> HermesShim::make(const HermesRuntimeConfig &config) noexcept {
  return std::make_shared<HermesShim>(config.createRuntime());
}

std::shared_ptr<facebook::jsi::Runtime> HermesShim::getRuntime() const noexcept {
  // TODO:
  // return std::shared_ptr<facebook::hermes::HermesRuntime>(nonAbiSafeRuntime_, RuntimeDeleter(shared_from_this()));
  return nullptr;
}

void HermesShim::dumpCrashData(int fileDescriptor) const noexcept {
  CRASH_ON_ERROR(GetHermesApi().dump_crash_data(runtime_, fileDescriptor));
}

void HermesShim::stopDebugging() noexcept {
  // TODO: Implement
}

/*static*/ void HermesShim::enableSamplingProfiler() noexcept {
  CRASH_ON_ERROR(GetHermesApi().sampling_profiler_enable());
}

/*static*/ void HermesShim::disableSamplingProfiler() noexcept {
  CRASH_ON_ERROR(GetHermesApi().sampling_profiler_disable());
}

/*static*/ void HermesShim::dumpSampledTraceToFile(const std::string &fileName) noexcept {
  CRASH_ON_ERROR(GetHermesApi().sampling_profiler_dump_to_file(fileName.c_str()));
}
void HermesShim::addToProfiling() const noexcept {
  CRASH_ON_ERROR(GetHermesApi().sampling_profiler_add(runtime_));
}

void HermesShim::removeFromProfiling() const noexcept {
  CRASH_ON_ERROR(GetHermesApi().sampling_profiler_remove(runtime_));
}

} // namespace Microsoft::ReactNative

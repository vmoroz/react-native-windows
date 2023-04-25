// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "HermesShim.h"
#include <jsinspector/InspectorInterfaces.h>
#include "Crash.h"

#define INIT_SYMBOL(symbol) \
  s_hermesApi.symbol = reinterpret_cast<decltype(&hermes_##symbol)>(getSymbolAddress("hermes_" #symbol));

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

  decltype(&hermes_set_inspector) set_inspector;
  decltype(&hermes_create_local_connection) create_local_connection;
  decltype(&hermes_delete_local_connection) delete_local_connection;
  decltype(&hermes_local_connection_send_message) local_connection_send_message;
  decltype(&hermes_local_connection_disconnect) local_connection_disconnect;
} s_hermesApi;

HMODULE s_hermesModule{};
std::once_flag s_hermesLoading;

void *getSymbolAddress(const char *symbolName) {
  void *result = GetProcAddress(s_hermesModule, symbolName);
  VerifyElseCrash(result);
  return result;
}

int32_t addInspectorPage(void *pageData, const char *title) noexcept;
void removeInspectorPage(int32_t pageId) noexcept;

HermesApi &getHermesApi() noexcept {
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

    INIT_SYMBOL(set_inspector);
    INIT_SYMBOL(create_local_connection);
    INIT_SYMBOL(delete_local_connection);
    INIT_SYMBOL(local_connection_send_message);
    INIT_SYMBOL(local_connection_disconnect);

    CRASH_ON_ERROR(s_hermesApi.set_inspector(&addInspectorPage, &removeInspectorPage));
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
    CRASH_ON_ERROR(getHermesApi().config_set_task_runner(
        config, new HermesTaskRunner(std::move(queue)), &PostTask, &Delete, nullptr));
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

struct HermesJsiBuffer : facebook::jsi::Buffer {
  static std::shared_ptr<const facebook::jsi::Buffer>
  Create(const uint8_t *buffer, size_t bufferSize, hermes_data_delete_cb bufferDeleteCallback, void *deleterData) {
    return std::shared_ptr<const facebook::jsi::Buffer>(
        new HermesJsiBuffer(buffer, bufferSize, bufferDeleteCallback, deleterData));
  }

  HermesJsiBuffer(
      const uint8_t *buffer,
      size_t bufferSize,
      hermes_data_delete_cb bufferDeleteCallback,
      void *deleterData) noexcept
      : buffer_(buffer),
        bufferSize_(bufferSize),
        bufferDeleteCallback_(bufferDeleteCallback),
        deleterData_(deleterData) {}

  ~HermesJsiBuffer() override {
    if (bufferDeleteCallback_) {
      bufferDeleteCallback_(const_cast<uint8_t *>(buffer_), deleterData_);
    }
  }

  const uint8_t *data() const override {
    return buffer_;
  }

  size_t size() const override {
    return bufferSize_;
  }

 private:
  const uint8_t *buffer_;
  size_t bufferSize_;
  hermes_data_delete_cb bufferDeleteCallback_;
  void *deleterData_;
};

class HermesScriptCache {
 public:
  static void Create(hermes_config config, std::shared_ptr<facebook::jsi::PreparedScriptStore> scriptStore) {
    CRASH_ON_ERROR(getHermesApi().config_set_script_cache(
        config, new HermesScriptCache(std::move(scriptStore)), &LoadScript, &StoreScript, &Delete, nullptr));
  }

 private:
  HermesScriptCache(std::shared_ptr<facebook::jsi::PreparedScriptStore> scriptStore)
      : scriptStore_(std::move(scriptStore)) {}

  static void HERMES_CDECL LoadScript(
      void *scriptCache,
      hermes_script_cache_metadata *scriptMetadata,
      const uint8_t **buffer,
      size_t *bufferSize,
      hermes_data_delete_cb *bufferDeleteCallback,
      void **deleterData) {
    auto &scriptStore = reinterpret_cast<HermesScriptCache *>(scriptCache)->scriptStore_;
    std::shared_ptr<const facebook::jsi::Buffer> preparedScript = scriptStore->tryGetPreparedScript(
        facebook::jsi::ScriptSignature{scriptMetadata->source_url, scriptMetadata->source_hash},
        facebook::jsi::JSRuntimeSignature{scriptMetadata->runtime_name, scriptMetadata->runtime_version},
        scriptMetadata->tag);
    *buffer = preparedScript->data();
    *bufferSize = preparedScript->size();
    *bufferDeleteCallback = [](void * /*data*/, void *deleterData) noexcept {
      delete reinterpret_cast<std::shared_ptr<const facebook::jsi::Buffer> *>(deleterData);
    };
    *deleterData = new std::shared_ptr<const facebook::jsi::Buffer>(std::move(preparedScript));
  }

  static void HERMES_CDECL StoreScript(
      void *scriptCache,
      hermes_script_cache_metadata *scriptMetadata,
      const uint8_t *buffer,
      size_t bufferSize,
      hermes_data_delete_cb bufferDeleteCallback,
      void *deleterData) {
    auto &scriptStore = reinterpret_cast<HermesScriptCache *>(scriptCache)->scriptStore_;
    scriptStore->persistPreparedScript(
        HermesJsiBuffer::Create(buffer, bufferSize, bufferDeleteCallback, deleterData),
        facebook::jsi::ScriptSignature{scriptMetadata->source_url, scriptMetadata->source_hash},
        facebook::jsi::JSRuntimeSignature{scriptMetadata->runtime_name, scriptMetadata->runtime_version},
        scriptMetadata->tag);
  }

  static void HERMES_CDECL Delete(void *scriptCache, void * /*deleterData*/) {
    delete reinterpret_cast<HermesScriptCache *>(scriptCache);
  }

 private:
  std::shared_ptr<facebook::jsi::PreparedScriptStore> scriptStore_;
  // napi_ext_script_cache NapiJsiV8RuntimeHolder::InitScriptCache(
  //     unique_ptr<PreparedScriptStore> &&preparedScriptStore) noexcept {
  //   napi_ext_script_cache scriptCache{};
  //   scriptCache.cache_object =
  //       NativeObjectWrapper<unique_ptr<PreparedScriptStore>>::Wrap(std::move(preparedScriptStore));
  //   scriptCache.load_cached_script = [](napi_env env,
  //                                       napi_ext_script_cache *script_cache,
  //                                       napi_ext_cached_script_metadata *script_metadata,
  //                                       napi_ext_buffer *result) -> napi_status {
  //     PreparedScriptStore *scriptStore = reinterpret_cast<PreparedScriptStore *>(script_cache->cache_object.data);
  //     std::shared_ptr<const facebook::jsi::Buffer> buffer = scriptStore->tryGetPreparedScript(
  //         ScriptSignature{script_metadata->source_url, script_metadata->source_hash},
  //         JSRuntimeSignature{script_metadata->runtime_name, script_metadata->runtime_version},
  //         script_metadata->tag);
  //     if (buffer) {
  //       result->buffer_object = NativeObjectWrapper<std::shared_ptr<const facebook::jsi::Buffer>>::Wrap(
  //           std::shared_ptr<const facebook::jsi::Buffer>{buffer});
  //       result->data = buffer->data();
  //       result->byte_size = buffer->size();
  //     } else {
  //       *result = napi_ext_buffer{};
  //     }
  //     return napi_ok;
  //   };
  //   scriptCache.store_cached_script = [](napi_env env,
  //                                        napi_ext_script_cache *script_cache,
  //                                        napi_ext_cached_script_metadata *script_metadata,
  //                                        const napi_ext_buffer *buffer) -> napi_status {
  //     PreparedScriptStore *scriptStore = reinterpret_cast<PreparedScriptStore *>(script_cache->cache_object.data);
  //     scriptStore->persistPreparedScript(
  //         NodeApiJsiBuffer::CreateJsiBuffer(buffer),
  //         ScriptSignature{script_metadata->source_url, script_metadata->source_hash},
  //         JSRuntimeSignature{script_metadata->runtime_name, script_metadata->runtime_version},
  //         script_metadata->tag);
  //     return napi_ok;
  //   };
  //   return scriptCache;
  // }
};

class HermesLocalConnection : public facebook::react::ILocalConnection {
 public:
  HermesLocalConnection(std::unique_ptr<facebook::react::IRemoteConnection> remoteConneciton, void *pageData) noexcept {
    CRASH_ON_ERROR(getHermesApi().create_local_connection(
        pageData,
        reinterpret_cast<hermes_remote_connection>(remoteConneciton.release()),
        &OnRemoteConnectionSendMessage,
        &OnRemoteConnectionDisconnect,
        &OnRemoteConnectionDelete,
        nullptr,
        &localConnection_));
  }

  ~HermesLocalConnection() override {
    CRASH_ON_ERROR(getHermesApi().delete_local_connection(localConnection_));
  }

  void sendMessage(std::string message) {
    CRASH_ON_ERROR(getHermesApi().local_connection_send_message(localConnection_, message.c_str()));
  }

  void disconnect() {
    CRASH_ON_ERROR(getHermesApi().local_connection_disconnect(localConnection_));
  }

 private:
  static void OnRemoteConnectionSendMessage(hermes_remote_connection remoteConnection, const char *message) {
    reinterpret_cast<facebook::react::IRemoteConnection *>(remoteConnection)->onMessage(message);
  }

  static void OnRemoteConnectionDisconnect(hermes_remote_connection remoteConnection) {
    reinterpret_cast<facebook::react::IRemoteConnection *>(remoteConnection)->onDisconnect();
  }

  static void OnRemoteConnectionDelete(void *remoteConnection, void * /*deleterData*/) {
    delete reinterpret_cast<facebook::react::IRemoteConnection *>(remoteConnection);
  }

 private:
  hermes_local_connection localConnection_{};
};

int32_t addInspectorPage(void *pageData, const char *title) noexcept {
  return facebook::react::getInspectorInstance().addPage(
      title, "Hermes", [pageData](std::unique_ptr<facebook::react::IRemoteConnection> remoteConneciton) {
        return std::make_unique<HermesLocalConnection>(std::move(remoteConneciton), pageData);
      });
}

void removeInspectorPage(int32_t pageId) noexcept {
  facebook::react::getInspectorInstance().removePage(pageId);
}

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
  HermesApi &api = getHermesApi();
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
    HermesScriptCache::Create(config, scriptStore_);
  }
  hermes_runtime runtime{};
  CRASH_ON_ERROR(api.create_runtime(config, &runtime));
  CRASH_ON_ERROR(api.delete_config(config));
  return runtime;
}

HermesShim::HermesShim(hermes_runtime runtime) noexcept : runtime_(runtime) {}

HermesShim::~HermesShim() {
  CRASH_ON_ERROR(getHermesApi().delete_runtime(runtime_));
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
  CRASH_ON_ERROR(getHermesApi().dump_crash_data(runtime_, fileDescriptor));
}

void HermesShim::stopDebugging() noexcept {
  // TODO: Implement
}

/*static*/ void HermesShim::enableSamplingProfiler() noexcept {
  CRASH_ON_ERROR(getHermesApi().sampling_profiler_enable());
}

/*static*/ void HermesShim::disableSamplingProfiler() noexcept {
  CRASH_ON_ERROR(getHermesApi().sampling_profiler_disable());
}

/*static*/ void HermesShim::dumpSampledTraceToFile(const std::string &fileName) noexcept {
  CRASH_ON_ERROR(getHermesApi().sampling_profiler_dump_to_file(fileName.c_str()));
}
void HermesShim::addToProfiling() const noexcept {
  CRASH_ON_ERROR(getHermesApi().sampling_profiler_add(runtime_));
}

void HermesShim::removeFromProfiling() const noexcept {
  CRASH_ON_ERROR(getHermesApi().sampling_profiler_remove(runtime_));
}

} // namespace Microsoft::ReactNative

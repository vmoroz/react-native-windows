// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "NapiJsiV8RuntimeHolder.h"
#include <ApiLoaders/V8Api.h>
#include <crash/verifyElseCrash.h>

using namespace Microsoft::NodeApiJsi;

#define CRASH_ON_ERROR(result) VerifyElseCrash(result == napi_ok);

namespace Microsoft::ReactNative {
namespace {

class V8FuncResolver : public IFuncResolver {
 public:
  // TODO: Use Office safe DLL loading
  V8FuncResolver() : libHandle_(LoadLibrary(L"v8jsi.dll")) {}

  FuncPtr getFuncPtr(const char *funcName) override {
    return reinterpret_cast<FuncPtr>(GetProcAddress(libHandle_, funcName));
  }

 private:
  HMODULE libHandle_;
};

V8Api &initV8Api() noexcept {
  static V8FuncResolver funcResolver;
  static V8Api s_v8Api(&funcResolver);
  V8Api::setCurrent(&s_v8Api);
  return s_v8Api;
}

V8Api &getV8Api() noexcept {
  static V8Api &s_v8Api = initV8Api();
  return s_v8Api;
}

class V8Task {
 public:
  V8Task(void *taskData, jsr_task_run_cb taskRunCallback, jsr_data_delete_cb taskDataDeleteCallback, void *deleterData)
      : taskData_(taskData),
        taskRunCallback_(taskRunCallback),
        taskDataDeleteCallback_(taskDataDeleteCallback),
        deleterData_(deleterData) {}

  V8Task(const V8Task &other) = delete;
  V8Task &operator=(const V8Task &other) = delete;

  ~V8Task() {
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
  jsr_task_run_cb taskRunCallback_;
  jsr_data_delete_cb taskDataDeleteCallback_;
  void *deleterData_;
};

class V8TaskRunner {
 public:
  static void Create(jsr_config config, std::shared_ptr<facebook::react::MessageQueueThread> queue) {
    CRASH_ON_ERROR(
        getV8Api().jsr_config_set_task_runner(config, new V8TaskRunner(std::move(queue)), &PostTask, &Delete, nullptr));
  }

 private:
  V8TaskRunner(std::shared_ptr<facebook::react::MessageQueueThread> queue) : queue_(std::move(queue)) {}

  static void NAPI_CDECL PostTask(
      void *taskRunnerData,
      void *taskData,
      jsr_task_run_cb taskRunCallback,
      jsr_data_delete_cb taskDataDeleteCallback,
      void *deleterData) {
    auto task = std::make_shared<V8Task>(taskData, taskRunCallback, taskDataDeleteCallback, deleterData);
    reinterpret_cast<V8TaskRunner *>(taskRunnerData)->queue_->runOnQueue([task = std::move(task)] { task->Run(); });
  }

  static void NAPI_CDECL Delete(void *taskRunner, void * /*deleterData*/) {
    delete reinterpret_cast<V8TaskRunner *>(taskRunner);
  }

 private:
  std::shared_ptr<facebook::react::MessageQueueThread> queue_;
};

struct V8JsiBuffer : facebook::jsi::Buffer {
  static std::shared_ptr<const facebook::jsi::Buffer>
  Create(const uint8_t *buffer, size_t bufferSize, jsr_data_delete_cb bufferDeleteCallback, void *deleterData) {
    return std::shared_ptr<const facebook::jsi::Buffer>(
        new V8JsiBuffer(buffer, bufferSize, bufferDeleteCallback, deleterData));
  }

  V8JsiBuffer(
      const uint8_t *buffer,
      size_t bufferSize,
      jsr_data_delete_cb bufferDeleteCallback,
      void *deleterData) noexcept
      : buffer_(buffer),
        bufferSize_(bufferSize),
        bufferDeleteCallback_(bufferDeleteCallback),
        deleterData_(deleterData) {}

  ~V8JsiBuffer() override {
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
  jsr_data_delete_cb bufferDeleteCallback_;
  void *deleterData_;
};

class V8ScriptCache {
 public:
  static void Create(jsr_config config, std::shared_ptr<facebook::jsi::PreparedScriptStore> scriptStore) {
    CRASH_ON_ERROR(getV8Api().jsr_config_set_script_cache(
        config, new V8ScriptCache(std::move(scriptStore)), &LoadScript, &StoreScript, &Delete, nullptr));
  }

 private:
  V8ScriptCache(std::shared_ptr<facebook::jsi::PreparedScriptStore> scriptStore)
      : scriptStore_(std::move(scriptStore)) {}

  static void NAPI_CDECL LoadScript(
      void *scriptCache,
      const char *sourceUrl,
      uint64_t sourceHash,
      const char *runtimeName,
      uint64_t runtimeVersion,
      const char *cacheTag,
      const uint8_t **buffer,
      size_t *bufferSize,
      jsr_data_delete_cb *bufferDeleteCallback,
      void **deleterData) {
    auto &scriptStore = reinterpret_cast<V8ScriptCache *>(scriptCache)->scriptStore_;
    std::shared_ptr<const facebook::jsi::Buffer> preparedScript = scriptStore->tryGetPreparedScript(
        facebook::jsi::ScriptSignature{sourceUrl, sourceHash},
        facebook::jsi::JSRuntimeSignature{runtimeName, runtimeVersion},
        cacheTag);
    *buffer = preparedScript->data();
    *bufferSize = preparedScript->size();
    *bufferDeleteCallback = [](void * /*data*/, void *deleterData) noexcept {
      delete reinterpret_cast<std::shared_ptr<const facebook::jsi::Buffer> *>(deleterData);
    };
    *deleterData = new std::shared_ptr<const facebook::jsi::Buffer>(std::move(preparedScript));
  }

  static void NAPI_CDECL StoreScript(
      void *scriptCache,
      const char *sourceUrl,
      uint64_t sourceHash,
      const char *runtimeName,
      uint64_t runtimeVersion,
      const char *cacheTag,
      const uint8_t *buffer,
      size_t bufferSize,
      jsr_data_delete_cb bufferDeleteCallback,
      void *deleterData) {
    auto &scriptStore = reinterpret_cast<V8ScriptCache *>(scriptCache)->scriptStore_;
    scriptStore->persistPreparedScript(
        V8JsiBuffer::Create(buffer, bufferSize, bufferDeleteCallback, deleterData),
        facebook::jsi::ScriptSignature{sourceUrl, sourceHash},
        facebook::jsi::JSRuntimeSignature{runtimeName, runtimeVersion},
        cacheTag);
  }

  static void NAPI_CDECL Delete(void *scriptCache, void * /*deleterData*/) {
    delete reinterpret_cast<V8ScriptCache *>(scriptCache);
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

} // namespace

NapiJsiV8RuntimeHolder::NapiJsiV8RuntimeHolder(
    std::shared_ptr<facebook::react::DevSettings> devSettings,
    std::shared_ptr<facebook::react::MessageQueueThread> jsQueue,
    std::unique_ptr<facebook::jsi::PreparedScriptStore> preparedScriptStore) noexcept
    : m_weakDevSettings(devSettings),
      m_jsQueue(std::move(jsQueue)),
      m_preparedScriptStore(std::move(preparedScriptStore)) {}

void NapiJsiV8RuntimeHolder::InitRuntime() noexcept {
  std::shared_ptr<facebook::react::DevSettings> devSettings = m_weakDevSettings.lock();
  VerifyElseCrash(devSettings);

  V8Api &api = getV8Api();
  V8Api::setCurrent(&api);
  jsr_config config{};
  CRASH_ON_ERROR(api.jsr_create_config(&config));
  CRASH_ON_ERROR(api.jsr_config_enable_inspector(config, devSettings->useDirectDebugger));
  CRASH_ON_ERROR(api.jsr_config_set_inspector_runtime_name(config, devSettings->debuggerRuntimeName.c_str()));
  CRASH_ON_ERROR(api.jsr_config_set_inspector_port(config, devSettings->debuggerPort));
  CRASH_ON_ERROR(api.jsr_config_set_inspector_break_on_start(config, devSettings->debuggerBreakOnNextLine));

  if (m_jsQueue) {
    V8TaskRunner::Create(config, m_jsQueue);
  }
  if (m_preparedScriptStore) {
    V8ScriptCache::Create(config, m_preparedScriptStore);
  }
  jsr_runtime runtime{};
  CRASH_ON_ERROR(api.jsr_create_runtime(config, &runtime));
  CRASH_ON_ERROR(api.jsr_delete_config(config));

  CRASH_ON_ERROR(api.jsr_create_runtime(config, &runtime));

  napi_env env{};
  CRASH_ON_ERROR(api.jsr_runtime_get_node_api_env(runtime, &env));

  m_jsiRuntime =
      makeNodeApiJsiRuntime(env, &api, [runtime]() { CRASH_ON_ERROR(V8Api::current()->jsr_delete_runtime(runtime)); });
  m_ownThreadId = std::this_thread::get_id();
}

// napi_ext_script_cache NapiJsiV8RuntimeHolder::InitScriptCache(
//     unique_ptr<PreparedScriptStore> &&preparedScriptStore) noexcept {
//   napi_ext_script_cache scriptCache{};
//   scriptCache.cache_object =
//   NativeObjectWrapper<unique_ptr<PreparedScriptStore>>::Wrap(std::move(preparedScriptStore));
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

facebook::react::JSIEngineOverride NapiJsiV8RuntimeHolder::getRuntimeType() noexcept {
  return facebook::react::JSIEngineOverride::V8NodeApi;
}

std::shared_ptr<facebook::jsi::Runtime> NapiJsiV8RuntimeHolder::getRuntime() noexcept {
  std::call_once(m_onceFlag, [this]() { InitRuntime(); });
  VerifyElseCrash(m_jsiRuntime);
  VerifyElseCrashSz(m_ownThreadId == std::this_thread::get_id(), "Must be accessed from JS thread.");
  return m_jsiRuntime;
}

} // namespace Microsoft::ReactNative

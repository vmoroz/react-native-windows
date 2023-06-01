// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <DevSettings.h>
#include <NodeApiJsiRuntime.h>
#include "RuntimeHolder.h"
#include "ScriptStore.h"

#include <cxxreact/MessageQueueThread.h>

namespace Microsoft::ReactNative {

class NapiJsiV8RuntimeHolder : public Microsoft::JSI::RuntimeHolderLazyInit {
 public: // RuntimeHolderLazyInit implementation.
  std::shared_ptr<facebook::jsi::Runtime> getRuntime() noexcept override;
  facebook::react::JSIEngineOverride getRuntimeType() noexcept override;

  NapiJsiV8RuntimeHolder(
      std::shared_ptr<facebook::react::DevSettings> devSettings,
      std::shared_ptr<facebook::react::MessageQueueThread> jsQueue,
      std::unique_ptr<facebook::jsi::PreparedScriptStore> preparedScriptStore) noexcept;

 private:
  void InitRuntime() noexcept;
  // napi_ext_script_cache InitScriptCache(
  //     std::unique_ptr<facebook::jsi::PreparedScriptStore> &&preparedScriptStore) noexcept;

 private:
  std::shared_ptr<facebook::jsi::Runtime> m_jsiRuntime;
  std::once_flag m_onceFlag{};
  std::thread::id m_ownThreadId{};
  std::weak_ptr<facebook::react::DevSettings> m_weakDevSettings;
  std::shared_ptr<facebook::react::MessageQueueThread> m_jsQueue;
  std::shared_ptr<facebook::jsi::PreparedScriptStore> m_preparedScriptStore;
};

} // namespace Microsoft::ReactNative

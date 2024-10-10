/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#include "HermesRuntimeAgentDelegate.h"

#include <jsinspector-modern/ReactCdp.h>

using namespace facebook::react::jsinspector_modern;
using namespace facebook::hermes;

namespace Microsoft::ReactNative {
/*
class HermesRuntimeAgentDelegate::Impl final : public RuntimeAgentDelegate {
  using HermesState = hermes::cdp::State;

  struct HermesStateWrapper : public ExportedState {
    explicit HermesStateWrapper(HermesState state) : state_(std::move(state)) {}

    static HermesState unwrapDestructively(ExportedState* wrapper) {
      if (!wrapper) {
        return {};
      }
      if (auto* typedWrapper = dynamic_cast<HermesStateWrapper*>(wrapper)) {
        return std::move(typedWrapper->state_);
      }
      return {};
    }

   private:
    HermesState state_;
  };

 public:
  Impl(
      FrontendChannel frontendChannel,
      SessionState& sessionState,
      std::unique_ptr<RuntimeAgentDelegate::ExportedState>
          previouslyExportedState,
      const ExecutionContextDescription& executionContextDescription,
      HermesRuntime& runtime,
      HermesRuntimeTargetDelegate& runtimeTargetDelegate,
      const RuntimeExecutor& runtimeExecutor)
      : hermes_(hermes::cdp::CDPAgent::create(
            executionContextDescription.id,
            runtimeTargetDelegate.getCDPDebugAPI(),
            // RuntimeTask takes a HermesRuntime whereas our RuntimeExecutor
            // takes a jsi::Runtime.
            [runtimeExecutor,
             &runtime](facebook::hermes::debugger::RuntimeTask fn) {
              runtimeExecutor(
                  [&runtime, fn = std::move(fn)](auto&) { fn(runtime); });
            },
            frontendChannel,
            HermesStateWrapper::unwrapDestructively(
                previouslyExportedState.get()))) {
    if (sessionState.isRuntimeDomainEnabled) {
      hermes_->enableRuntimeDomain();
    }
    if (sessionState.isDebuggerDomainEnabled) {
      hermes_->enableDebuggerDomain();
    }
  }

  bool handleRequest(const cdp::PreparsedRequest& req) override {
    // TODO: Change to string::starts_with when we're on C++20.
    if (req.method.rfind("Log.", 0) == 0) {
      // Since we know Hermes doesn't do anything useful with Log messages,
      // but our containing HostAgent will, bail out early.
      // TODO: We need a way to negotiate this more dynamically with Hermes
      // through the API.
      return false;
    }
    // Forward everything else to Hermes's CDPAgent.
    hermes_->handleCommand(req.toJson());
    // Let the call know that this request is handled (i.e. it is Hermes's
    // responsibility to respond with either success or an error).
    return true;
  }

  std::unique_ptr<ExportedState> getExportedState() override {
    return std::make_unique<HermesStateWrapper>(hermes_->getState());
  }

 private:
  std::unique_ptr<hermes::cdp::CDPAgent> hermes_;
};
*/

namespace {

struct HermesStateWrapper : public RuntimeAgentDelegate::ExportedState {
  explicit HermesStateWrapper(facebook::hermes::HermesUniqueCdpState &&hermesCdpState)
      : hermesCdpState_(std::move(hermesCdpState)) {}

  static facebook::hermes::HermesUniqueCdpState unwrapDestructively(ExportedState *wrapper) {
    if (!wrapper) {
      return {};
    }
    if (auto *typedWrapper = dynamic_cast<HermesStateWrapper *>(wrapper)) {
      return std::move(typedWrapper->hermesCdpState_);
    }
    return {};
  }

 private:
  facebook::hermes::HermesUniqueCdpState hermesCdpState_;
};

} // namespace

HermesRuntimeAgentDelegate::HermesRuntimeAgentDelegate(
    FrontendChannel frontendChannel,
    SessionState &sessionState,
    std::unique_ptr<RuntimeAgentDelegate::ExportedState> previouslyExportedState,
    const ExecutionContextDescription &executionContextDescription,
    hermes_runtime runtime,
    HermesRuntimeTargetDelegate &runtimeTargetDelegate,
    facebook::react::RuntimeExecutor runtimeExecutor)
    : hermesCdpAgent_(facebook::hermes::HermesApi2().createCdpAgent(
          runtimeTargetDelegate.getCdpDebugger(),
          executionContextDescription.id,
          // RuntimeTask takes a HermesRuntime whereas our RuntimeExecutor
          // takes a jsi::Runtime.
          facebook::hermes::AsFunctor<hermes_enqueue_runtime_task_functor>(
              [runtimeExecutor, runtime](hermes_runtime_task_functor runtimeTask) {
                runtimeExecutor(
                    [runtime, fn = std::make_shared<FunctorWrapper<hermes_runtime_task_functor>>(runtimeTask)](auto &) {
                      (*fn)(runtime);
                    });
              }),
          AsFunctor<hermes_enqueue_frontend_message_functor>(
              [frontendChannel](const char *json_utf8, size_t json_size) {
                frontendChannel(std::string_view(json_utf8, json_size));
              }),
          HermesStateWrapper::unwrapDestructively(previouslyExportedState.get()).release())) {
  if (sessionState.isRuntimeDomainEnabled) {
    facebook::hermes::HermesApi2().enableRuntimeDomain(hermesCdpAgent_.get());
  }
  if (sessionState.isDebuggerDomainEnabled) {
    facebook::hermes::HermesApi2().enableDebuggerDomain(hermesCdpAgent_.get());
  }
}

HermesRuntimeAgentDelegate::~HermesRuntimeAgentDelegate() = default;

bool HermesRuntimeAgentDelegate::handleRequest(const cdp::PreparsedRequest &req) {
  if (req.method.starts_with("Log.")) {
    // Since we know Hermes doesn't do anything useful with Log messages,
    // but our containing HostAgent will, bail out early.
    // TODO: We need a way to negotiate this more dynamically with Hermes
    // through the API.
    return false;
  }
  // Forward everything else to Hermes's CDPAgent.
  std::string json = req.toJson();
  facebook::hermes::HermesApi2().handleCommand(hermesCdpAgent_.get(), json.c_str(), json.size());
  // Let the call know that this request is handled (i.e. it is Hermes's
  // responsibility to respond with either success or an error).
  return true;
}

std::unique_ptr<RuntimeAgentDelegate::ExportedState> HermesRuntimeAgentDelegate::getExportedState() {
  // TODO: Implement this
  // return std::make_unique<HermesStateWrapper>(hermes_->getState());
  return nullptr;
}

} // namespace Microsoft::ReactNative

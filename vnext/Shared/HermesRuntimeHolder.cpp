// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include "HermesRuntimeHolder.h"

#include <JSI/decorator.h>
#include <crash/verifyElseCrash.h>
#include <cxxreact/MessageQueueThread.h>
#include <cxxreact/SystraceSection.h>
#include "HermesShim.h"

#include <memory>
#include <mutex>

namespace React {
using namespace winrt::Microsoft::ReactNative;
}

namespace Microsoft::ReactNative {

React::ReactPropertyId<React::ReactNonAbiValue<std::shared_ptr<HermesRuntimeHolder>>>
HermesRuntimeHolderProperty() noexcept {
  static React::ReactPropertyId<React::ReactNonAbiValue<std::shared_ptr<HermesRuntimeHolder>>> propId{
      L"ReactNative.HermesRuntimeHolder", L"HermesRuntimeHolder"};
  return propId;
}

namespace {

std::shared_ptr<HermesShim> makeHermesShimSystraced(const HermesRuntimeConfig &config) {
  facebook::react::SystraceSection s("HermesExecutorFactory::makeHermesRuntimeSystraced");
  return HermesShim::make(config);
}

} // namespace

void HermesRuntimeHolder::crashHandler(int fileDescriptor) noexcept {
  m_hermesShim->dumpCrashData(fileDescriptor);
}

void HermesRuntimeHolder::teardown() noexcept {
  m_hermesShim->stopDebugging();
}

facebook::react::JSIEngineOverride HermesRuntimeHolder::getRuntimeType() noexcept {
  return facebook::react::JSIEngineOverride::Hermes;
}

std::shared_ptr<facebook::jsi::Runtime> HermesRuntimeHolder::getRuntime() noexcept {
  std::call_once(m_onceFlag, [this]() { initRuntime(); });
  VerifyElseCrash(m_jsiRuntime);
  VerifyElseCrashSz(m_ownThreadId == std::this_thread::get_id(), "Must be accessed from JS thread.");
  return m_jsiRuntime;
}

HermesRuntimeHolder::HermesRuntimeHolder(
    std::shared_ptr<facebook::react::DevSettings> devSettings,
    std::shared_ptr<facebook::react::MessageQueueThread> jsQueue,
    std::unique_ptr<facebook::jsi::PreparedScriptStore> preparedScriptStore) noexcept
    : m_weakDevSettings(devSettings), m_jsQueue(std::move(jsQueue)) {}

void HermesRuntimeHolder::initRuntime() noexcept {
  auto devSettings = m_weakDevSettings.lock();
  VerifyElseCrash(devSettings);
  HermesRuntimeConfig hermesConfig;
  hermesConfig.enableDefaultCrashHandler(devSettings->enableDefaultCrashHandler);
  hermesConfig.useDirectDebugger(devSettings->useDirectDebugger);
  hermesConfig.debuggerRuntimeName(devSettings->debuggerRuntimeName);
  hermesConfig.debuggerPort(devSettings->debuggerPort);
  hermesConfig.debuggerBreakOnNextLine(devSettings->debuggerBreakOnNextLine);
  hermesConfig.foregroundTaskRunner(m_jsQueue);
  hermesConfig.scriptCache(std::move(m_preparedScriptStore));

  m_hermesShim = makeHermesShimSystraced(hermesConfig);
  m_jsiRuntime = m_hermesShim->getRuntime();
  m_ownThreadId = std::this_thread::get_id();

  // if (devSettings->useDirectDebugger) {
  //   //TODO:
  //   //auto adapter = std::make_unique<HermesExecutorRuntimeAdapter>(m_hermesShim, *m_jsiRuntime, m_jsQueue);
  //   m_hermesShim->enableDebugging();
  //       //devSettings->debuggerRuntimeName.empty() ? "Hermes React Native" :
  //       devSettings->debuggerRuntimeName.c_str());
  // }

  // Add js engine information to Error.prototype so in error reporting we
  // can send this information.
  auto errorPrototype = m_jsiRuntime->global()
                            .getPropertyAsObject(*m_jsiRuntime, "Error")
                            .getPropertyAsObject(*m_jsiRuntime, "prototype");
  errorPrototype.setProperty(*m_jsiRuntime, "jsEngine", "hermes");
}

std::shared_ptr<HermesRuntimeHolder> HermesRuntimeHolder::loadFrom(
    React::ReactPropertyBag const &propertyBag) noexcept {
  return *(propertyBag.Get(HermesRuntimeHolderProperty()));
}

void HermesRuntimeHolder::storeTo(
    React::ReactPropertyBag const &propertyBag,
    std::shared_ptr<HermesRuntimeHolder> const &holder) noexcept {
  propertyBag.Set(HermesRuntimeHolderProperty(), holder);
}

void HermesRuntimeHolder::addToProfiling() const noexcept {
  m_hermesShim->addToProfiling();
}

void HermesRuntimeHolder::removeFromProfiling() const noexcept {
  m_hermesShim->removeFromProfiling();
}

} // namespace Microsoft::ReactNative

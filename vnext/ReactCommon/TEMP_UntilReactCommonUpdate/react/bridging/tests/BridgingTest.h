/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

#include <gtest/gtest.h>
#include <react/bridging/Bridging.h>

// #define USE_HERMES_JSI

#ifdef USE_HERMES_JSI
#include <hermes/hermes.h>
#else
#include "jsi/JsiAbiApi.h"
#include "jsi/test/testlib.h"
#endif

#define EXPECT_JSI_THROW(expr) EXPECT_THROW((expr), facebook::jsi::JSIException)

namespace facebook::react {

class TestCallInvoker : public CallInvoker {
 public:
  void invokeAsync(std::function<void()> &&fn) override {
    queue_.push_back(std::move(fn));
  }

  void invokeSync(std::function<void()> &&) override {
    FAIL() << "JSCallInvoker does not support invokeSync()";
  }

 private:
  friend class BridgingTest;

  std::list<std::function<void()>> queue_;
};

class BridgingTest : public ::testing::Test {
 protected:
  BridgingTest() : theInvoker(std::make_shared<TestCallInvoker>()), invoker(theInvoker), runtime(makeRuntime()), rt(*runtime) {}

  ~BridgingTest() {
    LongLivedObjectCollection::get().clear();
  }

  void TearDown() override {
    flushQueue();

    // After flushing the invoker queue, we shouldn't leak memory.
    EXPECT_EQ(0, LongLivedObjectCollection::get().size());
  }

  jsi::Value eval(const std::string &js) {
    return rt.global().getPropertyAsFunction(rt, "eval").call(rt, js);
  }

  jsi::Function function(const std::string &js) {
    return eval(("(" + js + ")").c_str()).getObject(rt).getFunction(rt);
  }

  void flushQueue() {
    while (!theInvoker->queue_.empty()) {
      theInvoker->queue_.front()();
      theInvoker->queue_.pop_front();
      rt.drainMicrotasks(); // Run microtasks every cycle.
    }
  }

  static std::unique_ptr<jsi::Runtime> makeRuntime() {
#ifdef USE_HERMES_JSI
    return hermes::makeHermesRuntime(
       ::hermes::vm::RuntimeConfig::Builder()
       // Make promises work with Hermes microtasks.
       .withMicrotaskQueue(true)
       .build());
#else
    using winrt::Microsoft::ReactNative::JsiAbiRuntime;
    using winrt::Microsoft::ReactNative::JsiRuntime;
    JsiRuntime runtime{JsiRuntime::MakeChakraRuntime()};
    return std::make_unique<JsiAbiRuntime>(runtime);
#endif
  }

  std::shared_ptr<TestCallInvoker> theInvoker;
  std::shared_ptr<CallInvoker> invoker;
  std::unique_ptr<jsi::Runtime> runtime;
  jsi::Runtime &rt;
};

} // namespace facebook::react

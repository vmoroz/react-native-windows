// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

// The TestEventService helps to verify that test observes
// expected sequence of events that are logged by code being tested.

#include <JSValue.h>
#include <string_view>

namespace ReactNativeIntegrationTests {

using namespace winrt::Microsoft::ReactNative;

struct TestEvent {
  std::string EventName;
  JSValue Value;
};

template <typename T>
struct Span {
  template <std::size_t N>
  constexpr Span(T (&arr)[N]) noexcept : m_data(arr), m_size(N) {}
  constexpr Span(std::initializer_list<T> il) noexcept : m_data(il.begin()), m_size(il.size()) {}

  constexpr const T *Data() const noexcept {
    return m_data;
  }

  constexpr std::size_t Size() const noexcept {
    return m_size;
  }

 private:
  const T *m_data;
  const size_t m_size;
};

struct TestEventService {
  // Set to the service to the initial state.
  static void Initialize() noexcept;

  // The current expectation is that this method is always called on a thread
  // different to the one that runs the ObserveEvents method.
  // We can see a deadlock if this expectation is not met.
  static void LogEvent(std::string_view eventName, JSValue &&value) noexcept;

  // Block current thread and observe all incoming events until we see them all.
  static void ObserveEvents(Span<TestEvent> expectedEvents) noexcept;

 private:
  static std::mutex s_mutex;
  static std::condition_variable s_cv;
  static TestEvent s_currentEvent;
  static int s_eventIndex;
  static bool s_eventIsHandled;
};

} // namespace ReactNativeIntegrationTests

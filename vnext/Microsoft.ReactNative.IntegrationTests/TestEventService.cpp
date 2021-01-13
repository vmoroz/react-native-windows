// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"

#include <chrono>
#include <sstream>
#include <thread>
#include "TestEventService.h"

using namespace std::literals::chrono_literals;

namespace ReactNativeIntegrationTests {

/*static*/ std::mutex TestEventService::s_mutex;
/*static*/ std::condition_variable TestEventService::s_cv;
/*static*/ TestEvent TestEventService::s_currentEvent{};
/*static*/ int TestEventService::s_eventIndex{-1};
/*static*/ bool TestEventService::s_eventIsHandled{true};

/*static*/ void TestEventService::Initialize() noexcept {
  auto lock = std::scoped_lock{s_mutex};
  s_eventIndex = -1;
  s_eventIsHandled = true;
}

// The current expectation is that this method is always called on a thread
// different to the one that runs the ObserveEvents method.
// We can see a deadlock if this expectation is not met.
/*static*/ void TestEventService::LogEvent(std::string_view eventName, JSValue &&value) noexcept {
  while (true) {
    auto lock = std::scoped_lock{s_mutex};
    if (s_eventIsHandled) {
      s_currentEvent.EventName = eventName;
      s_currentEvent.Value = std::move(value);
      ++s_eventIndex;
      s_eventIsHandled = false;
      s_cv.notify_all();
      break;
    }
    std::this_thread::sleep_for(100ms);
  }
}

/*static*/ void TestEventService::ObserveEvents(Span<TestEvent> expectedEvents) noexcept {
  auto lock = std::unique_lock{s_mutex};
  s_cv.wait(lock, [&]() {
    if (s_eventIndex >= 0 && !s_eventIsHandled) {
      s_eventIsHandled = true;
      TestCheck(s_eventIndex < expectedEvents.Size());
      auto const &expectedEvent = *(expectedEvents.Data() + s_eventIndex);
      TestCheckEqual(expectedEvent.EventName, s_currentEvent.EventName);
      if (auto d1 = expectedEvent.Value.TryGetDouble(), d2 = s_currentEvent.Value.TryGetDouble(); d1 && d2) {
        if (!isnan(*d1) && !isnan(*d2)) {
          TestCheckEqual(*d1, *d2);
        }
      } else if (expectedEvent.Value != s_currentEvent.Value) {
        std::stringstream os;
        os << "Event index: " << s_eventIndex << '\n'
           << "Expected: " << expectedEvent.Value.ToString() << '\n'
           << "Actual: " << s_currentEvent.Value.ToString();
        TestCheckFail("%s", os.str().c_str());
      }
    }
    return s_eventIndex >= static_cast<ptrdiff_t>(expectedEvents.Size()) - 1;
  });

  // Make sure that we did all actions
  TestCheckEqual(s_eventIndex, static_cast<ptrdiff_t>(expectedEvents.Size()) - 1);
}

} // namespace ReactNativeIntegrationTests

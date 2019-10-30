// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License

#pragma once

#include <condition_variable>
#include <mutex>

namespace Microsoft::ReactNative {

// A simplified "event" to emulate waitable win32 events in a cross-platform way
struct SimpleEvent {
  void Set() noexcept {
    std::lock_guard<std::mutex> lk(m_cv_m);
    m_isSet = true;
    m_cv.notify_all();
  }

  void Wait() noexcept {
    std::unique_lock<std::mutex> lk(m_cv_m);
    m_cv.wait(lk, [&] { return m_isSet; });
  }

 private:
  std::condition_variable m_cv;
  std::mutex m_cv_m; // to synchronize accesses to m_isSet and for the condition
                     // variable m_cv
  bool m_isSet{false};
};

} // namespace Microsoft::ReactNative

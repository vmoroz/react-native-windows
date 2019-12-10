// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include "pch.h"

#include <functional>

#include "NativeModules.h"

namespace SampleLibraryCPP {

REACT_MODULE(Calculator);
struct Calculator {
  REACT_METHOD(Add);
  int Add(int x, int y) noexcept {
    return x + y;
  }

  REACT_METHOD(Subtract);
  void Subtract(int x, int y, winrt::Microsoft::ReactNative::Bridge::ReactPromise<int> &&result) noexcept {
    if (x > y) {
      result.Resolve(x - y);
    } else {
      winrt::Microsoft::ReactNative::Bridge::ReactError error{};
      error.Message = "x must be greater than y";
      result.Reject(std::move(error));
    }
  }
};

} // namespace SampleLibraryCPP

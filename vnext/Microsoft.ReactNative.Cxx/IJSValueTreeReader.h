// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_IJSVALUETREEREADER
#define MICROSOFT_REACTNATIVE_IJSVALUETREEREADER

#include <unknwn.h> // It must go before any other includes to enable use of custom interfaces
#include "JSValue.h"

namespace winrt::Microsoft::ReactNative::Bridge {

struct __declspec(uuid("2abb7c60-88d7-45e7-bb72-863c117a0c00")) IJSValueTreeReader : ::IUnknown {
  virtual const JSValue &Current() noexcept = 0;
  virtual const JSValue &Root() noexcept = 0;
};

} // namespace winrt::Microsoft::ReactNative::Bridge

#endif // MICROSOFT_REACTNATIVE_IJSVALUETREEREADER

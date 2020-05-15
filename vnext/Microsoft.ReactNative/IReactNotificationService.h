// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "ReactNotificationServiceHelper.g.h"

namespace winrt::Microsoft::ReactNative::implementation {
struct ReactNotificationServiceHelper {
  ReactNotificationServiceHelper() = default;

  static Microsoft::ReactNative::IReactNotificationService CreateNotificationService();
};
} // namespace winrt::Microsoft::ReactNative::implementation

namespace winrt::Microsoft::ReactNative::factory_implementation {
struct ReactNotificationServiceHelper
    : ReactNotificationServiceHelperT<ReactNotificationServiceHelper, implementation::ReactNotificationServiceHelper> {
};
} // namespace winrt::Microsoft::ReactNative::factory_implementation

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <jsinspector/InspectorInterfaces.h>
#include <functional>

namespace Microsoft::ReactNative {

class InspectorConnection {
 public:
  using ConnectionCreator = std::function<
      std::unique_ptr<facebook::react::ILocalConnection>(int32_t, std::unique_ptr<facebook::react::IRemoteConnection>)>;

  static std::unique_ptr<facebook::react::ILocalConnection> connect(
      int32_t pageId,
      std::unique_ptr<facebook::react::IRemoteConnection> remoteConnection) noexcept;

  static void registerConnectionCreator(const std::string& vm, const ConnectionCreator& connector) noexcept;
};

} // namespace Microsoft::ReactNative

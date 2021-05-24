// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "MessageQueueThreadFactory.h"
#include "BatchingQueueThread.h"

namespace Microsoft::ReactNative {

std::shared_ptr<facebook::react::BatchingMessageQueueThread> MakeBatchingQueueThread(
    std::shared_ptr<facebook::react::MessageQueueThread> const &queueThread) noexcept {
  return std::make_shared<BatchingQueueThread>(queueThread);
}

} // namespace Microsoft::ReactNative

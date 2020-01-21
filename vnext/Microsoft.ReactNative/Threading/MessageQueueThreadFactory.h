// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <cxxreact/MessageQueueThread.h>

namespace react::uwp {

std::shared_ptr<facebook::react::MessageQueueThread> MakeJSQueueThread() noexcept;

std::shared_ptr<facebook::react::MessageQueueThread> MakeUIQueueThread() noexcept;

std::shared_ptr<facebook::react::MessageQueueThread> MakeSerialQueueThread() noexcept;

}

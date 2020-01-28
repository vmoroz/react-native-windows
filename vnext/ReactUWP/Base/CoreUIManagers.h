// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <ReactWindowsCore/ReactWindowsAPI.h>
#include <memory>

namespace facebook::react {
class IUIManager;
} // namespace facebook::react

namespace react::uwp {

struct ViewManagerProvider;
struct IReactInstance;

REACTWINDOWS_API_(std::shared_ptr<facebook::react::IUIManager>)
CreateUIManager(
    std::shared_ptr<IReactInstance> const &instance,
    std::shared_ptr<ViewManagerProvider> const &viewManagerProvider) noexcept;

} // namespace react::uwp

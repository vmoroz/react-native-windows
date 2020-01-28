// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <string>
#include "React.h"
#include <functional/functor.h>

namespace Mso::React {

using ErrorCallback = Mso::Functor<void(const std::string &error)>;

//! Members of this interface are going to be moved somewhere else.
MSO_GUID(IReactInstanceWin32, "89de446a-b754-4ac5-8a91-be318dfaa7d3")
struct ILegacyReactInstance : IUnknown {
  /// Queues up a call to a JS function in the loaded bundle
  virtual void CallJsFunction(std::string &&moduleName, std::string &&method, folly::dynamic &&params) noexcept = 0;

  virtual std::shared_ptr<react::uwp::IReactInstance> UwpReactInstance() noexcept = 0;

  virtual void AttachMeasuredRootView(
      facebook::react::IReactRootView *rootView,
      std::string const &initialProps) noexcept = 0;

  virtual void DetachRootView(facebook::react::IReactRootView *rootView) noexcept = 0;
};

} // namespace Mso::React

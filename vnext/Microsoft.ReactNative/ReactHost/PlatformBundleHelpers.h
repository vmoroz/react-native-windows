#pragma once

#include <cxxreact/PlatformBundleInfo.h>
#include "JSBundle.h"
#include "ReactNativeHeaders.h"

namespace Mso::React {

std::vector<facebook::react::PlatformBundleInfo> MakePlatformBundleInfos(
    const std::vector<CntPtr<IJSBundle>> &jsBundles) noexcept;

} // namespace Mso::React

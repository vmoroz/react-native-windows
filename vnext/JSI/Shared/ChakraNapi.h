// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define NAPI_EXPERIMENTAL
#include "js_native_api.h"

namespace Microsoft::JSI {
struct ChakraRuntimeArgs;
}

namespace chakra {

napi_env MakeChakraNapiEnv(Microsoft::JSI::ChakraRuntimeArgs &&args) noexcept;

} // namespace chakra

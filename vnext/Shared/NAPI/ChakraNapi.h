// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#define NAPI_EXPERIMENTAL
#include <NAPI/js_native_api_ext.h>

namespace Microsoft::JSI {
struct ChakraRuntimeArgs;
}

namespace chakra {

napi_env MakeChakraNapiEnv(Microsoft::JSI::ChakraRuntimeArgs &&args) noexcept;

} // namespace chakra

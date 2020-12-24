// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include <gtest/gtest.h>
#include "ChakraNapi.h"
#include "ChakraRuntimeArgs.h"
#include "ChakraRuntimeFactory.h"
#include "NapiJsiRuntime.h"
#include "NapiTests.h"
#include "jsi/test/testlib.h"

using namespace Microsoft::JSI;

namespace facebook::jsi {

std::vector<RuntimeFactory> runtimeGenerators() {
  return {RuntimeFactory{[]() -> std::unique_ptr<Runtime> {
            ChakraRuntimeArgs args{};
            return makeChakraRuntime(std::move(args));
          }},
          RuntimeFactory{[]() -> std::unique_ptr<Runtime> {
            ChakraRuntimeArgs args{};
            napi_env env = chakra::MakeChakraNapiEnv(std::move(args));
            return ::react::jsi::MakeNapiJsiRuntime(env);
          }}};
}

} // namespace facebook::jsi

namespace napi {

std::vector<NapiEnvFactory> napiEnvGenerators() {
  return {NapiEnvFactory{[]() -> napi_env {
    ChakraRuntimeArgs args{};
    return chakra::MakeChakraNapiEnv(std::move(args));
  }}};
}

} // namespace napi

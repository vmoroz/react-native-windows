// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include <functional>
#include <vector>

#include <gtest/gtest.h>
#include <js_native_api_ext.h>

namespace napi {

using NapiEnvFactory = std::function<napi_env()>;

std::vector<NapiEnvFactory> napiEnvGenerators();

struct NapiException : std::exception {
  NapiException() {}
  NapiException(std::string what) : m_what(std::move(what)) {}

  virtual const char *what() const noexcept override {
    return m_what.c_str();
  }

  virtual ~NapiException() {}

 protected:
  std::string m_what;
};

[[noreturn]] void ThrowNapiException(napi_env env, napi_status errorCode);

struct NapiTestBase : ::testing::TestWithParam<NapiEnvFactory> {
  NapiTestBase() : factory(GetParam()), env(factory()) {}
  napi_value Eval(const char *code);
  napi_value Function(const std::string &code);
  napi_value CallFunction(std::initializer_list<napi_value> args, const std::string &code);
  bool CallBoolFunction(std::initializer_list<napi_value> args, const std::string &code);
  bool CheckEqual(napi_value value, const std::string &jsValue);
  bool CheckStrictEqual(napi_value value, const std::string &jsValue);
  bool CheckStrictEqual(const std::string &left, const std::string &right);

  NapiEnvFactory factory;
  napi_env env;
};

} // namespace napi

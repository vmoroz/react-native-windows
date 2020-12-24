// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define NAPI_EXPERIMENTAL
#include "NapiTests.h"

// Check condition and crash process if it fails.
#define CHECK_ELSE_CRASH(condition, message)               \
  do {                                                     \
    if (!(condition)) {                                    \
      assert(false && "Failed: " #condition && (message)); \
      *((int *)0) = 1;                                     \
    }                                                      \
  } while (false)

#define CHECK_NAPI(expr)                        \
  do {                                          \
    napi_status temp_status_ = (expr);          \
    if (temp_status_ != napi_status::napi_ok) { \
      ThrowNapiException(env, temp_status_);    \
    }                                           \
  } while (false)

namespace napi {

[[noreturn]] void ThrowNapiException(napi_env env, napi_status errorCode) {
  napi_value jsError{};
  CHECK_ELSE_CRASH(napi_get_and_clear_last_exception(env, &jsError) == napi_ok, "Cannot retrieve JS exception.");
  throw NapiException();
}

napi_value NapiTestBase::eval(const char *code) {
  napi_value result{}, global{}, func{}, undefined{}, codeStr{};
  CHECK_NAPI(napi_get_global(env, &global));
  CHECK_NAPI(napi_get_named_property(env, global, "eval", &func));
  CHECK_NAPI(napi_get_undefined(env, &undefined));
  CHECK_NAPI(napi_create_string_utf8(env, code, NAPI_AUTO_LENGTH, &codeStr));
  CHECK_NAPI(napi_call_function(env, undefined, func, 1, &codeStr, &result));
  return result;
}

napi_value NapiTestBase::function(const std::string &code) {
  return eval(("(" + code + ")").c_str());
}

bool NapiTestBase::checkValue(napi_value value, const std::string &jsValue) {
  bool result{};
  napi_value undefined{}, booleanResult{};
  napi_value func = function("function(value) { return value == " + jsValue + "; }");
  CHECK_NAPI(napi_get_undefined(env, &undefined));
  CHECK_NAPI(napi_call_function(env, undefined, func, 1, &value, &booleanResult));
  CHECK_NAPI(napi_get_value_bool(env, booleanResult, &result));
  return result;
}

} // namespace napi

using namespace napi;

struct NapiTest : NapiTestBase {};

TEST_P(NapiTest, RunScriptTest) {
  napi_value script{}, scriptResult{}, global{}, xValue{};
  int intValue{};
  CHECK_NAPI(napi_create_string_utf8(env, "1", NAPI_AUTO_LENGTH, &script));
  CHECK_NAPI(napi_run_script(env, script, &scriptResult));
  CHECK_NAPI(napi_get_value_int32(env, scriptResult, &intValue));
  EXPECT_EQ(intValue, 1);

  CHECK_NAPI(napi_create_string_utf8(env, "x = 1", NAPI_AUTO_LENGTH, &script));
  CHECK_NAPI(napi_run_script(env, script, &scriptResult));
  CHECK_NAPI(napi_get_global(env, &global));
  CHECK_NAPI(napi_get_named_property(env, global, "x", &xValue));
  CHECK_NAPI(napi_get_value_int32(env, xValue, &intValue));
  EXPECT_EQ(intValue, 1);
}

INSTANTIATE_TEST_CASE_P(NapiEnv, NapiTest, ::testing::ValuesIn(napiEnvGenerators()));

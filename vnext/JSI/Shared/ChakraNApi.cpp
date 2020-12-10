// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "js_native_api.h"
#ifdef CHAKRACORE
#include "ChakraCore.h"
#else
#ifndef USE_EDGEMODE_JSRT
#define USE_EDGEMODE_JSRT
#endif
#include <jsrt.h>
#endif
#include <array>
#include <cassert>

#define RETURN_STATUS_IF_FALSE(env, condition, status) \
  do {                                                 \
    if (!(condition)) {                                \
      return napi_set_last_error((env), (status));     \
    }                                                  \
  } while (0)

#define CHECK_ENV(env)         \
  do {                         \
    if ((env) == nullptr) {    \
      return napi_invalid_arg; \
    }                          \
  } while (0)

#define CHECK_ARG(env, arg) RETURN_STATUS_IF_FALSE((env), ((arg) != nullptr), napi_invalid_arg)

struct napi_env__ {
  JsSourceContext source_context = JS_SOURCE_CONTEXT_NONE;
  napi_extended_error_info last_error{nullptr, nullptr, 0, napi_ok};
  JsValueRef has_own_property_function = JS_INVALID_REFERENCE;
};

static napi_status napi_set_last_error(
    napi_env env,
    napi_status error_code,
    uint32_t engine_error_code = 0,
    void *engine_reserved = nullptr) {
  env->last_error.error_code = error_code;
  env->last_error.engine_error_code = engine_error_code;
  env->last_error.engine_reserved = engine_reserved;

  return error_code;
}

// Warning: Keep in-sync with napi_status enum
static const char *error_messages[] = {
    nullptr,
    "Invalid argument",
    "An object was expected",
    "A string was expected",
    "A string or symbol was expected",
    "A function was expected",
    "A number was expected",
    "A boolean was expected",
    "An array was expected",
    "Unknown failure",
    "An exception is pending",
    "The async work item was canceled",
    "napi_escape_handle already called on scope",
    "Invalid handle scope usage",
    "Invalid callback scope usage",
    "Thread-safe function queue is full",
    "Thread-safe function handle is closing",
    "A BigInt was expected",
};

napi_status napi_get_last_error_info(napi_env env, const napi_extended_error_info **result) {
  CHECK_ENV(env);
  CHECK_ARG(env, result);

  // you must update this assert to reference the last message
  // in the napi_status enum each time a new error message is added.
  // We don't have a napi_status_last as this would result in an ABI
  // change each time a message was added.
  static_assert(
      std::size(error_messages) == napi_bigint_expected + 1,
      "Count of error messages must match count of error values");
  assert(env->last_error.error_code <= napi_callback_scope_mismatch);

  // Wait until someone requests the last error information to fetch the error
  // message string
  env->last_error.error_message = error_messages[env->last_error.error_code];

  *result = &env->last_error;
  return napi_ok;
}

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
#include <memory>
#include <string>
#include <string_view>

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

#define CHECK_ENV_AND_ARG(env, arg) \
  CHECK_ENV((env));                 \
  CHECK_ARG((env), (arg))

#define CHECK_ENV_AND_ARG2(env, arg1, arg2) \
  CHECK_ENV((env));                         \
  CHECK_ARG((env), (arg1));                 \
  CHECK_ARG((env), (arg2))

#define CHECK_JSRT(env, expr)               \
  do {                                      \
    JsErrorCode err = (expr);               \
    if (err != JsNoError)                   \
      return napi_set_last_error(env, err); \
  } while (0)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)         \
  do {                           \
    napi_status status = (expr); \
    if (status != napi_ok)       \
      return status;             \
  } while (0)

#define STR_AND_LENGTH(str) str, sizeof(str) - 1

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

static void napi_clear_last_error(napi_env env) {
  env->last_error.error_code = napi_ok;
  env->last_error.engine_error_code = 0;
  env->last_error.engine_reserved = nullptr;
}

static napi_status napi_set_last_error(napi_env env, JsErrorCode jsError, void *engine_reserved = nullptr) {
  napi_status status;
  switch (jsError) {
    case JsNoError:
      status = napi_ok;
      break;
    case JsErrorNullArgument:
    case JsErrorInvalidArgument:
      status = napi_invalid_arg;
      break;
    case JsErrorPropertyNotString:
      status = napi_string_expected;
      break;
    case JsErrorArgumentNotObject:
      status = napi_object_expected;
      break;
    case JsErrorScriptException:
    case JsErrorInExceptionState:
      status = napi_pending_exception;
      break;
    default:
      status = napi_generic_failure;
      break;
  }

  env->last_error.error_code = status;
  env->last_error.engine_error_code = jsError;
  env->last_error.engine_reserved = engine_reserved;
  return status;
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

namespace {
constexpr UINT CP_LATIN1 = 28591;

std::wstring NarrowToWide(std::string_view value, UINT codePage = CP_UTF8) {
  if (value.size() == 0) {
    return {};
  }

  int requiredSize = ::MultiByteToWideChar(codePage, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  assert(requiredSize != 0);
  std::wstring wstr(requiredSize, 0);
  int result = ::MultiByteToWideChar(codePage, 0, value.data(), static_cast<int>(value.size()), &wstr[0], requiredSize);
  assert(result != 0);
  return std::move(wstr);
}

JsErrorCode JsCreateString(_In_ const char *content, _In_ size_t length, _Out_ JsValueRef *value) {
  auto str = (length == NAPI_AUTO_LENGTH ? NarrowToWide({content}) : NarrowToWide({content, length}));
  return JsPointerToString(str.data(), str.size(), value);
}

JsErrorCode JsCreatePropertyId(_In_z_ const char *name, _In_ size_t length, _Out_ JsPropertyIdRef *propertyId) {
  auto str = (length == NAPI_AUTO_LENGTH ? NarrowToWide({name}) : NarrowToWide({name, length}));
  return JsGetPropertyIdFromName(str.data(), propertyId);
}

// Callback Info struct as per JSRT native function.
struct CallbackInfo {
  napi_value newTarget;
  napi_value thisArg;
  napi_value *argv;
  void *data;
  uint16_t argc;
  bool isConstructCall;
};

// Adapter for JSRT external callback + callback data.
class ExternalCallback {
 public:
  ExternalCallback(napi_env env, napi_callback cb, void *data) : _env(env), _cb(cb), _data(data) {}

  // JsNativeFunction
  static JsValueRef CALLBACK Callback(
      JsValueRef callee,
      bool isConstructCall,
      JsValueRef *arguments,
      unsigned short argumentCount,
      void *callbackState) {
    ExternalCallback *externalCallback = reinterpret_cast<ExternalCallback *>(callbackState);

    // Make sure any errors encountered last time we were in N-API are gone.
    napi_clear_last_error(externalCallback->_env);

    CallbackInfo cbInfo;
    cbInfo.thisArg = reinterpret_cast<napi_value>(arguments[0]);
    cbInfo.newTarget = reinterpret_cast<napi_value>(externalCallback->newTarget);
    cbInfo.isConstructCall = isConstructCall;
    cbInfo.argc = argumentCount - 1;
    cbInfo.argv = reinterpret_cast<napi_value *>(arguments + 1);
    cbInfo.data = externalCallback->_data;

    napi_value result = externalCallback->_cb(externalCallback->_env, reinterpret_cast<napi_callback_info>(&cbInfo));
    return reinterpret_cast<JsValueRef>(result);
  }

  // JsObjectBeforeCollectCallback
  static void CALLBACK Finalize(JsRef ref, void *callbackState) {
    ExternalCallback *externalCallback = reinterpret_cast<ExternalCallback *>(callbackState);
    delete externalCallback;
  }

  // Value for 'new.target'
  JsValueRef newTarget;

 private:
  napi_env _env;
  napi_callback _cb;
  void *_data;
};

napi_status SetErrorCode(napi_env env, JsValueRef error, napi_value code, const char *codeString) {
  if ((code != nullptr) || (codeString != nullptr)) {
    JsValueRef codeValue = reinterpret_cast<JsValueRef>(code);
    if (codeValue != JS_INVALID_REFERENCE) {
      JsValueType valueType = JsUndefined;
      CHECK_JSRT(env, JsGetValueType(codeValue, &valueType));
      RETURN_STATUS_IF_FALSE(env, valueType == JsString, napi_string_expected);
    } else {
      CHECK_JSRT(env, JsCreateString(codeString, NAPI_AUTO_LENGTH, &codeValue));
    }

    JsPropertyIdRef codePropId = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreatePropertyId(STR_AND_LENGTH("code"), &codePropId));

    CHECK_JSRT(env, JsSetProperty(error, codePropId, codeValue, true));

    JsValueRef nameArray = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreateArray(0, &nameArray));

    JsPropertyIdRef pushPropId = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreatePropertyId(STR_AND_LENGTH("push"), &pushPropId));

    JsValueRef pushFunction = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsGetProperty(nameArray, pushPropId, &pushFunction));

    JsPropertyIdRef namePropId = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreatePropertyId(STR_AND_LENGTH("name"), &namePropId));

    bool hasProp = false;
    CHECK_JSRT(env, JsHasProperty(error, namePropId, &hasProp));

    JsValueRef nameValue = JS_INVALID_REFERENCE;
    std::array<JsValueRef, 2> args = {nameArray, JS_INVALID_REFERENCE};

    if (hasProp) {
      CHECK_JSRT(env, JsGetProperty(error, namePropId, &nameValue));

      args[1] = nameValue;
      CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));
    }

    const char *openBracket = " [";
    JsValueRef openBracketValue = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreateString(openBracket, NAPI_AUTO_LENGTH, &openBracketValue));

    args[1] = openBracketValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    args[1] = codeValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    const char *closeBracket = "]";
    JsValueRef closeBracketValue = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreateString(closeBracket, NAPI_AUTO_LENGTH, &closeBracketValue));

    args[1] = closeBracketValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    JsValueRef emptyValue = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreateString("", 0, &emptyValue));

    const char *joinPropIdName = "join";
    JsPropertyIdRef joinPropId = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsCreatePropertyId(joinPropIdName, strlen(joinPropIdName), &joinPropId));

    JsValueRef joinFunction = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsGetProperty(nameArray, joinPropId, &joinFunction));

    args[1] = emptyValue;
    CHECK_JSRT(env, JsCallFunction(joinFunction, args.data(), static_cast<unsigned short>(args.size()), &nameValue));

    CHECK_JSRT(env, JsSetProperty(error, namePropId, nameValue, true));
  }
  return napi_ok;
}

} // namespace

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

//==============================================================================
// Getters for defined singletons
//==============================================================================

napi_status napi_get_undefined(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsGetUndefinedValue(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_null(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsGetNullValue(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_global(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsGetGlobalObject(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_boolean(napi_env env, bool value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsBoolToBoolean(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

//==============================================================================
// Methods to create Primitive types/Objects
//==============================================================================

NAPI_EXTERN napi_status napi_create_object(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateObject(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_array(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateArray(0, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateArray(static_cast<unsigned int>(length), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_double(napi_env env, double value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_int32(napi_env env, int32_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsIntToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_int64(napi_env env, int64_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_string_latin1(napi_env env, const char *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  std::wstring wstr =
      (length == NAPI_AUTO_LENGTH) ? NarrowToWide({str}, CP_LATIN1) : NarrowToWide({str, length}, CP_LATIN1);
  CHECK_JSRT(env, JsPointerToString(wstr.data(), wstr.size(), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_string_utf8(napi_env env, const char *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateString(str, length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  static_assert(sizeof(char16_t) == sizeof(wchar_t));
  CHECK_JSRT(
      env, JsPointerToString(reinterpret_cast<const wchar_t *>(str), length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_symbol(napi_env env, napi_value description, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef js_description = reinterpret_cast<JsValueRef>(description);
  CHECK_JSRT(env, JsCreateSymbol(js_description, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_function(
    napi_env env,
    const char *utf8name,
    size_t length,
    napi_callback cb,
    void *data,
    napi_value *result) try {
  CHECK_ENV_AND_ARG(env, result);

  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(env, cb, data)};

  JsValueRef function;
  if (utf8name != nullptr) {
    JsValueRef name{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsCreateString(utf8name, length, &name));
    CHECK_JSRT(env, JsCreateNamedFunction(name, ExternalCallback::Callback, externalCallback.get(), &function));
  } else {
    CHECK_JSRT(env, JsCreateFunction(ExternalCallback::Callback, externalCallback.get(), &function));
  }

  externalCallback->newTarget = function;

  CHECK_JSRT(env, JsSetObjectBeforeCollectCallback(function, externalCallback.get(), ExternalCallback::Finalize));
  externalCallback.release();

  *result = reinterpret_cast<napi_value>(function);
  return napi_ok;
} catch (...) {
  return napi_set_last_error(env, napi_generic_failure);
}

NAPI_EXTERN napi_status napi_create_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreateError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_type_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error = JS_INVALID_REFERENCE;
  CHECK_JSRT(env, JsCreateTypeError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

NAPI_EXTERN napi_status napi_create_range_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error = JS_INVALID_REFERENCE;
  CHECK_JSRT(env, JsCreateRangeError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

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
#include <cmath>
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

#define CHECK_JSRT_EXPECTED(env, expr, expected) \
  do {                                           \
    JsErrorCode err = (expr);                    \
    if (err == JsErrorInvalidArgument)           \
      return napi_set_last_error(env, expected); \
    if (err != JsNoError)                        \
      return napi_set_last_error(env, err);      \
  } while (0)

#define CHECK_JSRT_ERROR_CODE(expr)       \
  do {                                    \
    auto result = (expr);                 \
    if (result != JsErrorCode::JsNoError) \
      return result;                      \
  } while (0)

// This does not call napi_set_last_error because the expression
// is assumed to be a NAPI function call that already did.
#define CHECK_NAPI(expr)         \
  do {                           \
    napi_status status = (expr); \
    if (status != napi_ok)       \
      return status;             \
  } while (0)

// utf8 multibyte codepoint start check
#define UTF8_MULTIBYTE_START(c) (((c)&0xC0) == 0xC0)

#define STR_AND_LENGTH(str) str, std::size(str) - 1

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

JsErrorCode JsCopyString(
    _In_ JsValueRef value,
    _Out_opt_ char *buffer,
    _In_ size_t bufferSize,
    _Out_opt_ size_t *length,
    UINT codePage = CP_UTF8) {
  const wchar_t *stringValue;
  size_t stringLength;
  CHECK_JSRT_ERROR_CODE(JsStringToPointer(value, &stringValue, &stringLength));

  if (length != nullptr) {
    *length = stringLength;
  }

  if (buffer != nullptr) {
    int result = ::WideCharToMultiByte(
        codePage,
        0,
        stringValue,
        static_cast<int>(stringLength),
        buffer,
        static_cast<int>(bufferSize),
        nullptr,
        nullptr);
    assert(result != 0);
  }

  return JsErrorCode::JsNoError;
}

JsErrorCode
JsCopyStringUtf16(_In_ JsValueRef value, _Out_opt_ char16_t *buffer, _In_ size_t bufferSize, _Out_opt_ size_t *length) {
  const wchar_t *stringValue;
  size_t stringLength;
  CHECK_JSRT_ERROR_CODE(JsStringToPointer(value, &stringValue, &stringLength));

  if (length != nullptr) {
    *length = stringLength;
  }

  if (buffer != nullptr) {
    static_assert(sizeof(char16_t) == sizeof(wchar_t));
    memcpy_s(buffer, bufferSize, stringValue, stringLength * sizeof(wchar_t));
  }

  return JsErrorCode::JsNoError;
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

JsErrorCode JsPropertyIdFromKey(JsValueRef key, JsPropertyIdRef *propertyId) {
  JsValueType keyType;
  CHECK_JSRT_ERROR_CODE(JsGetValueType(key, &keyType));

  if (keyType == JsString) {
    const wchar_t *stringValue;
    size_t stringLength;
    CHECK_JSRT_ERROR_CODE(JsStringToPointer(key, &stringValue, &stringLength));
    CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromName(stringValue, propertyId));
  } else if (keyType == JsSymbol) {
    CHECK_JSRT_ERROR_CODE(JsGetPropertyIdFromSymbol(key, propertyId));
  } else {
    return JsErrorCode::JsErrorInvalidArgument;
  }
  return JsErrorCode::JsNoError;
}

JsErrorCode JsPropertyIdFromPropertyDescriptor(const napi_property_descriptor *p, JsPropertyIdRef *propertyId) {
  if (p->utf8name != nullptr) {
    return JsCreatePropertyId(p->utf8name, strlen(p->utf8name), propertyId);
  } else {
    return JsPropertyIdFromKey(p->name, propertyId);
  }
}

JsErrorCode JsNameValueFromPropertyDescriptor(const napi_property_descriptor *p, napi_value *name) {
  if (p->utf8name != nullptr) {
    return JsCreateString(p->utf8name, NAPI_AUTO_LENGTH, reinterpret_cast<JsValueRef *>(name));
  } else {
    *name = p->name;
    return JsErrorCode::JsNoError;
  }
}

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

    JsPropertyIdRef codePropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"code", &codePropId));

    CHECK_JSRT(env, JsSetProperty(error, codePropId, codeValue, true));

    JsValueRef nameArray{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsCreateArray(0, &nameArray));

    JsPropertyIdRef pushPropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"push", &pushPropId));

    JsValueRef pushFunction{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetProperty(nameArray, pushPropId, &pushFunction));

    JsPropertyIdRef namePropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"name", &namePropId));

    bool hasProp = false;
    CHECK_JSRT(env, JsHasProperty(error, namePropId, &hasProp));

    JsValueRef nameValue{JS_INVALID_REFERENCE};
    std::array<JsValueRef, 2> args = {nameArray, JS_INVALID_REFERENCE};

    if (hasProp) {
      CHECK_JSRT(env, JsGetProperty(error, namePropId, &nameValue));

      args[1] = nameValue;
      CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));
    }

    JsValueRef openBracketValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsPointerToString(STR_AND_LENGTH(L" ["), &openBracketValue));

    args[1] = openBracketValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    args[1] = codeValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    JsValueRef closeBracketValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsPointerToString(STR_AND_LENGTH(L"]"), &closeBracketValue));

    args[1] = closeBracketValue;
    CHECK_JSRT(env, JsCallFunction(pushFunction, args.data(), static_cast<unsigned short>(args.size()), nullptr));

    JsValueRef emptyValue{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsPointerToString(L"", 0, &emptyValue));

    JsPropertyIdRef joinPropId{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsGetPropertyIdFromName(L"join", &joinPropId));

    JsValueRef joinFunction = JS_INVALID_REFERENCE;
    CHECK_JSRT(env, JsGetProperty(nameArray, joinPropId, &joinFunction));

    args[1] = emptyValue;
    CHECK_JSRT(env, JsCallFunction(joinFunction, args.data(), static_cast<unsigned short>(args.size()), &nameValue));

    CHECK_JSRT(env, JsSetProperty(error, namePropId, nameValue, true));
  }
  return napi_ok;
}

napi_status CreatePropertyFunction(
    napi_env env,
    napi_value property_name,
    napi_callback cb,
    void *callback_data,
    napi_value *result) try {
  CHECK_ENV_AND_ARG2(env, property_name, result);

  std::unique_ptr<ExternalCallback> externalCallback{new ExternalCallback(env, cb, callback_data)};

  napi_valuetype nameType;
  CHECK_NAPI(napi_typeof(env, property_name, &nameType));

  JsValueRef function;
  if (nameType == napi_string) {
    JsValueRef name{JS_INVALID_REFERENCE};
    name = property_name;
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

napi_status napi_create_object(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateObject(reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_array(napi_env env, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateArray(0, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_array_with_length(napi_env env, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateArray(static_cast<unsigned int>(length), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_double(napi_env env, double value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_int32(napi_env env, int32_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsIntToNumber(value, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_uint32(napi_env env, uint32_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_int64(napi_env env, int64_t value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsDoubleToNumber(static_cast<double>(value), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_string_latin1(napi_env env, const char *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  std::wstring wstr =
      (length == NAPI_AUTO_LENGTH) ? NarrowToWide({str}, CP_LATIN1) : NarrowToWide({str, length}, CP_LATIN1);
  CHECK_JSRT(env, JsPointerToString(wstr.data(), wstr.size(), reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_string_utf8(napi_env env, const char *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  CHECK_JSRT(env, JsCreateString(str, length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  static_assert(sizeof(char16_t) == sizeof(wchar_t));
  CHECK_JSRT(
      env, JsPointerToString(reinterpret_cast<const wchar_t *>(str), length, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_symbol(napi_env env, napi_value description, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef js_description = reinterpret_cast<JsValueRef>(description);
  CHECK_JSRT(env, JsCreateSymbol(js_description, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_create_function(
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

napi_status napi_create_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreateError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

napi_status napi_create_type_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreateTypeError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

napi_status napi_create_range_error(napi_env env, napi_value code, napi_value msg, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, msg, result);
  JsValueRef message = reinterpret_cast<JsValueRef>(msg);

  JsValueRef error{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreateRangeError(message, &error));
  CHECK_NAPI(SetErrorCode(env, error, code, nullptr));

  *result = reinterpret_cast<napi_value>(error);
  return napi_ok;
}

//==============================================================================
// Methods to get the native napi_value from Primitive type
//==============================================================================

napi_status napi_typeof(napi_env env, napi_value value, napi_valuetype *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  JsValueType valueType = JsUndefined;
  CHECK_JSRT(env, JsGetValueType(jsValue, &valueType));

  switch (valueType) {
    case JsUndefined:
      *result = napi_undefined;
      break;
    case JsNull:
      *result = napi_null;
      break;
    case JsNumber:
      *result = napi_number;
      break;
    case JsString:
      *result = napi_string;
      break;
    case JsBoolean:
      *result = napi_boolean;
      break;
    case JsFunction:
      *result = napi_function;
      break;
    case JsSymbol:
      *result = napi_symbol;
      break;
    case JsError:
      *result = napi_object;
      break;

    default:
      bool hasExternalData;
      if (JsHasExternalData(jsValue, &hasExternalData) != JsNoError) {
        hasExternalData = false;
      }

      *result = hasExternalData ? napi_external : napi_object;
      break;
      // TODO: [vmoroz] add detection of napi_bigint
  }

  return napi_ok;
}

napi_status napi_get_value_double(napi_env env, napi_value value, double *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT_EXPECTED(env, JsNumberToDouble(jsValue, result), napi_number_expected);
  return napi_ok;
}

napi_status napi_get_value_int32(napi_env env, napi_value value, int32_t *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  int valueInt;
  CHECK_JSRT_EXPECTED(env, JsNumberToInt(jsValue, &valueInt), napi_number_expected);
  *result = static_cast<int32_t>(valueInt);
  return napi_ok;
}

napi_status napi_get_value_uint32(napi_env env, napi_value value, uint32_t *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  double valueDouble;
  CHECK_JSRT_EXPECTED(env, JsNumberToDouble(jsValue, &valueDouble), napi_number_expected);
  if (std::isfinite(valueDouble)) {
    *result = static_cast<int32_t>(valueDouble);
  } else {
    *result = 0;
  }
  return napi_ok;
}

napi_status napi_get_value_int64(napi_env env, napi_value value, int64_t *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  double valueDouble;
  CHECK_JSRT_EXPECTED(env, JsNumberToDouble(jsValue, &valueDouble), napi_number_expected);

  if (std::isfinite(valueDouble)) {
    *result = static_cast<int64_t>(valueDouble);
  } else {
    *result = 0;
  }

  return napi_ok;
}

napi_status napi_get_value_bool(napi_env env, napi_value value, bool *result) {
  CHECK_ENV_AND_ARG2(env, value, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT_EXPECTED(env, JsBooleanToBool(jsValue, result), napi_boolean_expected);
  return napi_ok;
}

// Copies a JavaScript string into a LATIN-1 string buffer. The result is the
// number of bytes (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in bytes)
// via the result parameter.
// The result argument is optional unless buf is NULL.
napi_status napi_get_value_string_latin1(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
  CHECK_ENV_AND_ARG(env, value);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(env, result);
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, result, CP_LATIN1), napi_string_expected);
  } else {
    size_t count = 0;
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, &count), napi_string_expected);

    if (bufsize <= count) {
      // if bufsize == count there is no space for null terminator
      // Slow path: must implement truncation here.
      std::unique_ptr<char[]> fullBuffer{new char[count]};
      // CHAKRA_VERIFY(fullBuffer != nullptr);

      CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, fullBuffer.get(), count, nullptr), napi_string_expected);
      memmove(buf, fullBuffer.get(), sizeof(char) * bufsize);
      fullBuffer.reset();

      // Truncate string to the start of the last codepoint
      if (bufsize > 0 && (((buf[bufsize - 1] & 0x80) == 0) || UTF8_MULTIBYTE_START(buf[bufsize - 1]))) {
        // Last byte is a single byte codepoint or
        // starts a multibyte codepoint
        bufsize -= 1;
      } else if (bufsize > 1 && UTF8_MULTIBYTE_START(buf[bufsize - 2])) {
        // Second last byte starts a multibyte codepoint,
        bufsize -= 2;
      } else if (bufsize > 2 && UTF8_MULTIBYTE_START(buf[bufsize - 3])) {
        // Third last byte starts a multibyte codepoint
        bufsize -= 3;
      } else if (bufsize > 3 && UTF8_MULTIBYTE_START(buf[bufsize - 4])) {
        // Fourth last byte starts a multibyte codepoint
        bufsize -= 4;
      }

      buf[bufsize] = '\0';

      if (result) {
        *result = bufsize;
      }

      return napi_ok;
    }

    // Fast path, result fits in the buffer
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, buf, bufsize - 1, &count), napi_string_expected);

    buf[count] = 0;

    if (result != nullptr) {
      *result = count;
    }
  }

  return napi_ok;
}

// Copies a JavaScript string into a UTF-8 string buffer. The result is the
// number of bytes (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in bytes)
// via the result parameter.
// The result argument is optional unless buf is NULL.
napi_status napi_get_value_string_utf8(napi_env env, napi_value value, char *buf, size_t bufsize, size_t *result) {
  CHECK_ENV_AND_ARG(env, value);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(env, result);
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, result), napi_string_expected);
  } else {
    size_t count = 0;
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, nullptr, 0, &count), napi_string_expected);

    if (bufsize <= count) {
      // if bufsize == count there is no space for null terminator
      // Slow path: must implement truncation here.
      std::unique_ptr<char[]> fullBuffer{new char[count]};
      // CHAKRA_VERIFY(fullBuffer != nullptr);

      CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, fullBuffer.get(), count, nullptr), napi_string_expected);
      memmove(buf, fullBuffer.get(), sizeof(char) * bufsize);
      fullBuffer.reset();

      // Truncate string to the start of the last codepoint
      if (bufsize > 0 && (((buf[bufsize - 1] & 0x80) == 0) || UTF8_MULTIBYTE_START(buf[bufsize - 1]))) {
        // Last byte is a single byte codepoint or
        // starts a multibyte codepoint
        bufsize -= 1;
      } else if (bufsize > 1 && UTF8_MULTIBYTE_START(buf[bufsize - 2])) {
        // Second last byte starts a multibyte codepoint,
        bufsize -= 2;
      } else if (bufsize > 2 && UTF8_MULTIBYTE_START(buf[bufsize - 3])) {
        // Third last byte starts a multibyte codepoint
        bufsize -= 3;
      } else if (bufsize > 3 && UTF8_MULTIBYTE_START(buf[bufsize - 4])) {
        // Fourth last byte starts a multibyte codepoint
        bufsize -= 4;
      }

      buf[bufsize] = '\0';

      if (result) {
        *result = bufsize;
      }

      return napi_ok;
    }

    // Fast path, result fits in the buffer
    CHECK_JSRT_EXPECTED(env, JsCopyString(jsValue, buf, bufsize - 1, &count), napi_string_expected);

    buf[count] = 0;

    if (result != nullptr) {
      *result = count;
    }
  }

  return napi_ok;
}

// Copies a JavaScript string into a UTF-16 string buffer. The result is the
// number of 2-byte code units (excluding the null terminator) copied into buf.
// A sufficient buffer size should be greater than the length of string,
// reserving space for null terminator.
// If bufsize is insufficient, the string will be truncated and null terminated.
// If buf is NULL, this method returns the length of the string (in 2-byte
// code units) via the result parameter.
// The result argument is optional unless buf is NULL.
napi_status napi_get_value_string_utf16(napi_env env, napi_value value, char16_t *buf, size_t bufsize, size_t *result) {
  CHECK_ENV_AND_ARG(env, value);

  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);

  if (!buf) {
    CHECK_ARG(env, result);

    CHECK_JSRT_EXPECTED(env, JsCopyStringUtf16(jsValue, nullptr, 0, result), napi_string_expected);
  } else {
    size_t copied = 0;
    CHECK_JSRT_EXPECTED(env, JsCopyStringUtf16(jsValue, buf, bufsize - 1, &copied), napi_string_expected);

    if (copied < bufsize - 1) {
      buf[copied] = 0;
    } else {
      buf[bufsize - 1] = 0;
    }

    if (result != nullptr) {
      *result = copied;
    }
  }

  return napi_ok;
}

//==============================================================================
// Methods to coerce values
// These APIs may execute user scripts
//==============================================================================

napi_status napi_coerce_to_bool(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToBoolean(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_coerce_to_number(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToNumber(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_coerce_to_object(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToObject(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_coerce_to_string(napi_env env, napi_value value, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsConvertValueToString(jsValue, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

//==============================================================================
// Methods to work with Objects
//==============================================================================

napi_status napi_get_prototype(napi_env env, napi_value object, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsGetPrototype(obj, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_get_property_names(napi_env env, napi_value object, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef propertyNames{JS_INVALID_REFERENCE};
  // TODO: [vmoroz] Check the V8 implementation to make sure that this implementation is correct.
  CHECK_JSRT(env, JsGetOwnPropertyNames(obj, &propertyNames));
  *result = reinterpret_cast<napi_value>(propertyNames);
  return napi_ok;
}

napi_status napi_set_property(napi_env env, napi_value object, napi_value key, napi_value value) {
  CHECK_ENV_AND_ARG2(env, key, value);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  JsValueRef js_value = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsSetProperty(obj, propertyId, js_value, /*useStrictRules:*/ true));
  return napi_ok;
}

napi_status napi_has_property(napi_env env, napi_value object, napi_value key, bool *result) {
  CHECK_ENV_AND_ARG2(env, key, result);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsHasProperty(obj, propertyId, result));
  return napi_ok;
}

napi_status napi_get_property(napi_env env, napi_value object, napi_value key, napi_value *result) {
  CHECK_ENV_AND_ARG2(env, key, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  CHECK_JSRT(env, JsGetProperty(obj, propertyId, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_delete_property(napi_env env, napi_value object, napi_value key, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  *result = false;

  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsPropertyIdFromKey(key, &propertyId));
  JsValueRef deletePropertyResult{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsDeleteProperty(obj, propertyId, false /* isStrictMode */, &deletePropertyResult));
  CHECK_JSRT(env, JsBooleanToBool(deletePropertyResult, result));
  return napi_ok;
}

napi_status napi_has_own_property(napi_env env, napi_value object, napi_value key, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef hasOwnPropertyResult;
  std::array<JsValueRef, 2> hasOwnPropertyFuncArgs{object, key};
  CHECK_JSRT(
      env,
      JsCallFunction(
          env->has_own_property_function,
          hasOwnPropertyFuncArgs.data(),
          static_cast<unsigned short>(hasOwnPropertyFuncArgs.size()),
          &hasOwnPropertyResult));
  CHECK_JSRT(env, JsBooleanToBool(hasOwnPropertyResult, result));
  return napi_ok;
}

napi_status napi_set_named_property(napi_env env, napi_value object, const char *utf8name, napi_value value) {
  CHECK_ENV_AND_ARG(env, value);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreatePropertyId(utf8name, NAPI_AUTO_LENGTH, &propertyId));
  JsValueRef js_value = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsSetProperty(obj, propertyId, js_value, true));
  return napi_ok;
}

napi_status napi_has_named_property(napi_env env, napi_value object, const char *utf8name, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreatePropertyId(utf8name, strlen(utf8name), &propertyId));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsHasProperty(obj, propertyId, result));
  return napi_ok;
}

napi_status napi_get_named_property(napi_env env, napi_value object, const char *utf8name, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsPropertyIdRef propertyId{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsCreatePropertyId(utf8name, strlen(utf8name), &propertyId));
  CHECK_JSRT(env, JsGetProperty(obj, propertyId, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_set_element(napi_env env, napi_value object, uint32_t index, napi_value value) {
  CHECK_ENV_AND_ARG(env, value);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(index, &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  JsValueRef jsValue = reinterpret_cast<JsValueRef>(value);
  CHECK_JSRT(env, JsSetIndexedProperty(obj, jsIndex, jsValue));
  return napi_ok;
}

napi_status napi_has_element(napi_env env, napi_value object, uint32_t index, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(static_cast<int>(index), &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsHasIndexedProperty(obj, jsIndex, result));
  return napi_ok;
}

napi_status napi_get_element(napi_env env, napi_value object, uint32_t index, napi_value *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(static_cast<int>(index), &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsGetIndexedProperty(obj, jsIndex, reinterpret_cast<JsValueRef *>(result)));
  return napi_ok;
}

napi_status napi_delete_element(napi_env env, napi_value object, uint32_t index, bool *result) {
  CHECK_ENV_AND_ARG(env, result);
  JsValueRef jsIndex{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsIntToNumber(index, &jsIndex));
  JsValueRef obj = reinterpret_cast<JsValueRef>(object);
  CHECK_JSRT(env, JsDeleteIndexedProperty(obj, jsIndex));
  // TODO: [vmoroz] Check the result value
  *result = true;
  return napi_ok;
}

napi_status napi_define_properties(
    napi_env env,
    napi_value object,
    size_t property_count,
    const napi_property_descriptor *properties) {
  CHECK_ENV_AND_ARG(env, object);
  if (property_count > 0) {
    CHECK_ARG(env, properties);
  }

  JsPropertyIdRef configurableProperty{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsGetPropertyIdFromName(L"configurable", &configurableProperty));

  // TODO: [vmoroz] Add cached property ID
  JsPropertyIdRef enumerableProperty{JS_INVALID_REFERENCE};
  CHECK_JSRT(env, JsGetPropertyIdFromName(L"enumerable", &enumerableProperty));

  for (size_t i = 0; i < property_count; i++) {
    const napi_property_descriptor *p = properties + i;

    JsValueRef descriptor{JS_INVALID_REFERENCE};
    CHECK_JSRT(env, JsCreateObject(&descriptor));

    if (p->attributes & napi_configurable) {
      // TODO: [vmoroz] Add cached true/false JsValue
      JsValueRef configurable{JS_INVALID_REFERENCE};
      CHECK_JSRT(env, JsBoolToBoolean(true, &configurable));
      CHECK_JSRT(env, JsSetProperty(descriptor, configurableProperty, configurable, true));
    }

    if (p->attributes & napi_enumerable) {
      JsValueRef enumerable{JS_INVALID_REFERENCE};
      CHECK_JSRT(env, JsBoolToBoolean(true, &enumerable));
      CHECK_JSRT(env, JsSetProperty(descriptor, enumerableProperty, enumerable, true));
    }

    if (p->getter != nullptr || p->setter != nullptr) {
      napi_value property_name;
      CHECK_JSRT(env, JsNameValueFromPropertyDescriptor(p, &property_name));

      if (p->getter != nullptr) {
        JsPropertyIdRef getProperty;
        CHECK_JSRT(env, JsGetPropertyIdFromName(L"get", &getProperty));
        JsValueRef getter;
        CHECK_NAPI(
            CreatePropertyFunction(env, property_name, p->getter, p->data, reinterpret_cast<napi_value *>(&getter)));
        CHECK_JSRT(env, JsSetProperty(descriptor, getProperty, getter, true));
      }

      if (p->setter != nullptr) {
        JsPropertyIdRef setProperty;
        CHECK_JSRT(env, JsGetPropertyIdFromName(L"set", &setProperty));
        JsValueRef setter;
        CHECK_NAPI(
            CreatePropertyFunction(env, property_name, p->setter, p->data, reinterpret_cast<napi_value *>(&setter)));
        CHECK_JSRT(env, JsSetProperty(descriptor, setProperty, setter, true));
      }
    } else if (p->method != nullptr) {
      napi_value property_name;
      CHECK_JSRT(env, JsNameValueFromPropertyDescriptor(p, &property_name));

      JsPropertyIdRef valueProperty;
      CHECK_JSRT(env, JsGetPropertyIdFromName(L"value", &valueProperty));
      JsValueRef method;
      CHECK_NAPI(
          CreatePropertyFunction(env, property_name, p->method, p->data, reinterpret_cast<napi_value *>(&method)));
      CHECK_JSRT(env, JsSetProperty(descriptor, valueProperty, method, true));
    } else {
      RETURN_STATUS_IF_FALSE(env, p->value != nullptr, napi_invalid_arg);

      if (p->attributes & napi_writable) {
        JsPropertyIdRef writableProperty;
        CHECK_JSRT(env, JsGetPropertyIdFromName(L"writable", &writableProperty));
        JsValueRef writable;
        CHECK_JSRT(env, JsBoolToBoolean(true, &writable));
        CHECK_JSRT(env, JsSetProperty(descriptor, writableProperty, writable, true));
      }

      JsPropertyIdRef valueProperty;
      CHECK_JSRT(env, JsGetPropertyIdFromName(L"value", &valueProperty));
      CHECK_JSRT(env, JsSetProperty(descriptor, valueProperty, reinterpret_cast<JsValueRef>(p->value), true));
    }

    JsPropertyIdRef nameProperty;
    CHECK_JSRT(env, JsPropertyIdFromPropertyDescriptor(p, &nameProperty));
    bool result;
    CHECK_JSRT(
        env,
        JsDefineProperty(
            reinterpret_cast<JsValueRef>(object),
            reinterpret_cast<JsPropertyIdRef>(nameProperty),
            reinterpret_cast<JsValueRef>(descriptor),
            &result));
  }

  return napi_ok;
}

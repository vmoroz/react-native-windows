#pragma once
#ifndef SRC_JS_NATIVE_API_EXT_H_
#define SRC_JS_NATIVE_API_EXT_H_

#include "js_native_api.h"

EXTERN_C_START

typedef struct napiext_buffer__ *napiext_buffer;

NAPI_EXTERN napi_status napiext_evaluate_serialized_script(
    napi_env env,
    napiext_buffer scriptBuffer,
    napiext_buffer serializedScriptBuffer,
    const char *sourceUrl,
    napi_value *result);

NAPI_EXTERN napi_status napiext_get_unique_string(napi_env env, napi_value str, napi_value *result);

NAPI_EXTERN napi_status
napiext_get_unique_string_latin1(napi_env env, const char *str, size_t length, napi_value *result);

NAPI_EXTERN napi_status
napiext_get_unique_string_utf8(napi_env env, const char *str, size_t length, napi_value *result);

NAPI_EXTERN napi_status
napiext_get_unique_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result);

EXTERN_C_END

#endif // SRC_JS_NATIVE_API_EXT_H_

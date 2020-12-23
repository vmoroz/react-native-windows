#pragma once
#ifndef SRC_JS_NATIVE_API_EXT_H_
#define SRC_JS_NATIVE_API_EXT_H_

#include "js_native_api.h"

EXTERN_C_START

// TODO: [vmoroz] Add SAL annotations
// TODO: [vmoroz] API docs

NAPI_EXTERN napi_status napiext_get_unique_string(napi_env env, napi_value str, napi_value *result);

NAPI_EXTERN napi_status
napiext_get_unique_string_latin1(napi_env env, const char *str, size_t length, napi_value *result);

NAPI_EXTERN napi_status
napiext_get_unique_string_utf8(napi_env env, const char *str, size_t length, napi_value *result);

NAPI_EXTERN napi_status
napiext_get_unique_string_utf16(napi_env env, const char16_t *str, size_t length, napi_value *result);

NAPI_EXTERN napi_status
napiext_serialize_script(napi_env env, const char *script, uint8_t *buffer, size_t *buffer_size);

NAPI_EXTERN napi_status napiext_run_serialized_script(
    napi_env env,
    const char *script,
    uint8_t *buffer,
    const char *source_url,
    napi_value *result);

EXTERN_C_END

#endif // SRC_JS_NATIVE_API_EXT_H_

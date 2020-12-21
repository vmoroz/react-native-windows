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

EXTERN_C_END

#endif // SRC_JS_NATIVE_API_EXT_H_

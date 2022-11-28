// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef REACT_COMMON_API_H_
#define REACT_COMMON_API_H_

#ifdef __cplusplus
#define REACT_EXTERN_C_START extern "C" {
#define REACT_EXTERN_C_END }
#else
#define REACT_EXTERN_C_START
#define REACT_EXTERN_C_END
#endif

#define REACT_STDCALL __stdcall

//#ifdef REACT_COMMON_EXPORTS
//#define REACT_API __declspec(dllexport) react_status REACT_STDCALL
//#else
//#define REACT_API __declspec(dllimport) react_status REACT_STDCALL
//#endif

#define REACT_API react_status REACT_STDCALL

REACT_EXTERN_C_START

#include <stdint.h>

typedef enum {
  react_status_ok, react_status_error
} react_status;

typedef union react_object_t {
  struct react_object_s* obj_;

#ifdef __cplusplus
  explicit react_object_t(struct react_object_s* obj) : obj_(obj) {}
#endif
} react_object_t;

REACT_API react_object_add_ref(react_object_t obj);
REACT_API react_object_release(react_object_t obj);

REACT_API react_object_create(react_object_t* result);

REACT_EXTERN_C_END

#endif // !REACT_COMMON_API_H_


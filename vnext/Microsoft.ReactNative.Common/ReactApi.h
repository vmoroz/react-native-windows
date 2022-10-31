// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef REACT_API_H_
#define REACT_API_H_

#ifdef __cplusplus
#define REACT_EXTERN_C_START extern "C" {
#define REACT_EXTERN_C_END }
#else
#define REACT_EXTERN_C_START
#define REACT_EXTERN_C_END
#endif

#define REACT_STDCALL __stdcall

REACT_EXTERN_C_START

#include <stdint.h>

typedef enum { react_status_ok, react_status_error } react_status;

typedef enum {
  react_property_type_empty = 0,
  react_property_type_uint8 = 1,
  react_property_type_int16 = 2,
  react_property_type_uint16 = 3,
  react_property_type_int32 = 4,
  react_property_type_uint32 = 5,
  react_property_type_int64 = 6,
  react_property_type_uint64 = 7,
  react_property_type_single = 8,
  react_property_type_double = 9,
  react_property_type_char16 = 10,
  react_property_type_boolean = 11,
  react_property_type_string = 12,
  react_property_type_object = 13,
  react_property_type_datetime = 14,
  react_property_type_timespan = 15,
  react_property_type_guid = 16,
  react_property_type_point = 17,
  react_property_type_size = 18,
  react_property_type_rect = 19,
  react_property_type_uint8_array = react_property_type_uint8 + 1024,
  react_property_type_int16_array = react_property_type_int16 + 1024,
  react_property_type_uint16_array = react_property_type_uint16 + 1024,
  react_property_type_int32_array = react_property_type_int32 + 1024,
  react_property_type_uint32_array = react_property_type_uint32 + 1024,
  react_property_type_int64_array = react_property_type_int64 + 1024,
  react_property_type_uint64_array = react_property_type_uint64 + 1024,
  react_property_type_single_array = react_property_type_single + 1024,
  react_property_type_double_array = react_property_type_double + 1024,
  react_property_type_char16_array = react_property_type_char16 + 1024,
  react_property_type_boolean_array = react_property_type_boolean + 1024,
  react_property_type_string_array = react_property_type_string + 1024,
  react_property_type_object_array = react_property_type_object + 1024,
  react_property_type_datetime_array = react_property_type_datetime + 1024,
  react_property_type_timespan_array = react_property_type_timespan + 1024,
  react_property_type_guid_array = react_property_type_guid + 1024,
  react_property_type_point_array = react_property_type_point + 1024,
  react_property_type_size_array = react_property_type_size + 1024,
  react_property_type_rect_array = react_property_type_rect + 1024
} react_property_type;

typedef struct react_property_namespace_t *react_property_namespace;
typedef struct react_property_name_t *react_property_name;
typedef struct react_property_value_t *react_property_value;
typedef struct react_property_bag_t *react_property_bag;
typedef struct react_string_t *react_string;
typedef struct react_object_t *react_object;

typedef struct {
  int64_t value;
} react_datetime_t;

typedef struct {
  int64_t value;
} react_timespan_t;

typedef union {
  struct {
    uint64_t part1;
    uint64_t part2;
  };
  struct {
    uint32_t data1;
    uint16_t data2;
    uint16_t data3;
    uint8_t data4[8];
  };
} react_guid_t;

typedef struct {
  float x;
  float y;
} react_point_t;

typedef struct {
  float width;
  float height;
} react_size_t;

typedef struct {
  float x;
  float y;
  float width;
  float height;
} react_rect_t;

typedef react_property_value(REACT_STDCALL *react_create_property_value_callback)(void *data);

react_status REACT_STDCALL react_object_add_ref(react_object obj);
react_status REACT_STDCALL react_object_release(react_object obj);

react_status REACT_STDCALL
react_property_namespace_get(const char *name, size_t length, react_property_namespace *result);
react_status REACT_STDCALL react_property_namespace_global(react_property_namespace *result);
react_status REACT_STDCALL react_property_namespace_get_string(react_property_namespace ns, char **str, size_t *length);

react_status REACT_STDCALL react_property_namespace_add_ref(react_property_namespace ns);
react_status REACT_STDCALL react_property_namespace_release(react_property_namespace ns);
react_status REACT_STDCALL react_property_namespace_from_object(react_object obj, react_property_namespace *result);
react_status REACT_STDCALL react_property_namespace_to_object(react_property_namespace ns, react_object *result);

react_status REACT_STDCALL
react_property_name_get(react_property_namespace ns, const char *name, size_t length, react_property_name *result);
react_status REACT_STDCALL
react_property_name_get_namespace(react_property_name name, react_property_namespace *result);
react_status REACT_STDCALL react_property_name_get_local_string(react_property_name name, char **str, size_t *length);

react_status REACT_STDCALL react_property_name_add_ref(react_property_name name);
react_status REACT_STDCALL react_property_name_release(react_property_name name);
react_status REACT_STDCALL react_property_name_from_object(react_object obj, react_property_name *result);
react_status REACT_STDCALL react_property_name_to_object(react_property_name name, react_object *result);

react_status REACT_STDCALL react_property_value_create_empty(react_property_value *result);
react_status REACT_STDCALL react_property_value_create_uint8(uint8_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_int16(int16_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_uint16(uint16_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_int32(int32_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_uint32(uint32_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_int64(int64_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_uint64(uint64_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_single(float value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_double(double value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_char16(char16_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_boolean(bool value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_string(react_string value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_object(react_object value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_datetime(react_datetime_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_timespan(react_timespan_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_guid(react_guid_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_point(react_point_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_size(react_size_t value, react_property_value *result);
react_status REACT_STDCALL react_property_value_create_rect(react_rect_t value, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_uint8_array(uint8_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_int16_array(int16_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_uint16_array(uint16_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_int32_array(int32_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_uint32_array(uint32_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_int64_array(int64_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_uint64_array(uint64_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_single_array(float *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_double_array(double *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_char16_array(char16_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_boolean_array(bool *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_string_array(react_string *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_object_array(react_object *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_datetime_array(react_datetime_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_timespan_array(react_timespan_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_guid_array(react_guid_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_point_array(react_point_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_size_array(react_size_t *arr, size_t arr_length, react_property_value *result);
react_status REACT_STDCALL
react_property_value_create_rect_array(react_rect_t *arr, size_t arr_length, react_property_value *result);

react_status REACT_STDCALL react_property_value_get_type(react_property_value value, react_property_type *result);

react_status REACT_STDCALL react_property_get_uint8(react_property_value value, uint8_t *result);
react_status REACT_STDCALL react_property_get_int16(react_property_value value, int16_t *result);
react_status REACT_STDCALL react_property_get_uint16(react_property_value value, uint16_t *result);
react_status REACT_STDCALL react_property_get_int32(react_property_value value, int32_t *result);
react_status REACT_STDCALL react_property_get_uint32(react_property_value value, uint32_t *result);
react_status REACT_STDCALL react_property_get_int64(react_property_value value, int64_t *result);
react_status REACT_STDCALL react_property_get_uint64(react_property_value value, uint64_t *result);
react_status REACT_STDCALL react_property_get_single(react_property_value value, float *result);
react_status REACT_STDCALL react_property_get_double(react_property_value value, double *result);
react_status REACT_STDCALL react_property_get_char16(react_property_value value, char16_t *result);
react_status REACT_STDCALL react_property_get_boolean(react_property_value value, bool *result);
react_status REACT_STDCALL react_property_get_string(react_property_value value, react_string *result);
react_status REACT_STDCALL react_property_get_object(react_property_value value, react_object *result);
react_status REACT_STDCALL react_property_get_datetime(react_property_value value, react_datetime_t *result);
react_status REACT_STDCALL react_property_get_timespan(react_property_value value, react_timespan_t *result);
react_status REACT_STDCALL react_property_get_guid(react_property_value value, react_guid_t *result);
react_status REACT_STDCALL react_property_get_point(react_property_value value, react_point_t *result);
react_status REACT_STDCALL react_property_get_size(react_property_value value, react_size_t *result);
react_status REACT_STDCALL react_property_get_rect(react_property_value value, react_rect_t *result);
react_status REACT_STDCALL react_property_get_uint8_array(react_property_value value, uint8_t *arr, size_t *arr_length);
react_status REACT_STDCALL react_property_get_int16_array(react_property_value value, int16_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_uint16_array(react_property_value value, uint16_t *arr, size_t *arr_length);
react_status REACT_STDCALL react_property_get_int32_array(react_property_value value, int32_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_uint32_array(react_property_value value, uint32_t *arr, size_t *arr_length);
react_status REACT_STDCALL react_property_get_int64_array(react_property_value value, int64_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_uint64_array(react_property_value value, uint64_t *arr, size_t *arr_length);
react_status REACT_STDCALL react_property_get_single_array(react_property_value value, float *arr, size_t *arr_length);
react_status REACT_STDCALL react_property_get_double_array(react_property_value value, double *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_char16_array(react_property_value value, char16_t *arr, size_t *arr_length);
react_status REACT_STDCALL react_property_get_boolean_array(react_property_value value, bool *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_string_array(react_property_value value, react_string *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_object_array(react_property_value value, react_object *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_datetime_array(react_property_value value, react_datetime_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_timespan_array(react_property_value value, react_timespan_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_guid_array(react_property_value value, react_guid_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_point_array(react_property_value value, react_point_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_size_array(react_property_value value, react_size_t *arr, size_t *arr_length);
react_status REACT_STDCALL
react_property_get_rect_array(react_property_value value, react_rect_t *arr, size_t *arr_length);

react_status REACT_STDCALL react_property_value_add_ref(react_property_value value);
react_status REACT_STDCALL react_property_value_release(react_property_value value);
react_status REACT_STDCALL react_property_value_from_object(react_object obj, react_property_value *result);
react_status REACT_STDCALL react_property_value_to_object(react_property_value value, react_object *result);

react_status REACT_STDCALL react_property_bag_create(react_property_bag *result);
react_status REACT_STDCALL react_property_bag_create_copy(react_property_bag bag, react_property_bag *result);
react_status REACT_STDCALL
react_property_bag_get_value(react_property_bag bag, react_property_name name, react_property_value *result);
react_status REACT_STDCALL react_property_bag_get_or_create_value(
    react_property_bag bag,
    react_property_name name,
    react_create_property_value_callback create_callback,
    void *callback_data,
    react_property_value *result);
react_status REACT_STDCALL react_property_bag_set_value(
    react_property_bag bag,
    react_property_name name,
    react_property_value value,
    react_property_value *previous_value);

react_status REACT_STDCALL react_property_bag_add_ref(react_property_bag bag);
react_status REACT_STDCALL react_property_bag_release(react_property_bag bag);
react_status REACT_STDCALL react_property_bag_from_object(react_object obj, react_property_bag *result);
react_status REACT_STDCALL react_property_bag_to_object(react_property_bag bag, react_object *result);

REACT_EXTERN_C_END

#endif // !REACT_API_H_

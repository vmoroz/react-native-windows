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
typedef enum { react_bool_false, react_bool_true } react_bool;

#define REACT_API react_status REACT_STDCALL

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

typedef enum { react_js_engine_chakra = 0, react_js_engine_hermes = 1, react_js_engine_v8 = 2 } react_js_engine;

typedef struct react_property_namespace_t *react_property_namespace;
typedef struct react_property_name_t *react_property_name;
typedef struct react_property_value_t *react_property_value;
typedef struct react_property_bag_t *react_property_bag;
typedef struct react_string_t *react_string;
typedef struct react_dispatcher_t *react_dispatcher;
typedef struct react_notification_service_t *react_notification_service;
typedef struct react_notification_subscription_t *react_notification_subscription;
typedef struct react_host_t *react_host;
typedef struct react_host_builder_t *react_host_builder;
typedef struct react_extension_package_t *react_extension_package;
typedef struct react_instance_builder_t *react_instance_builder;
typedef struct react_red_box_handler_t *react_red_box_handler;
typedef struct react_log_handler_t *react_log_handler;

typedef union {
  struct react_object_s* obj;
} react_object_t;

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

//typedef react_property_value(REACT_STDCALL *react_create_property_value_callback)(void *data);
//typedef void(REACT_STDCALL *react_destroy_callback)(void *data, void *hint);
//typedef void(REACT_STDCALL *react_dispatcher_callback)(react_dispatcher dispatcher, void *data);
//typedef void(REACT_STDCALL *react_notification_handler_callback)(
//    react_notification_service service,
//    react_notification_subscription subscription,
//    react_object sender,
//    react_object data);
//
REACT_API react_object_add_ref(react_object_t obj);
REACT_API react_object_release(react_object_t obj);
//
//react_status REACT_STDCALL
//react_property_namespace_get(const char *name, size_t length, react_property_namespace *result);
//react_status REACT_STDCALL react_property_namespace_global(react_property_namespace *result);
//react_status REACT_STDCALL react_property_namespace_get_string(react_property_namespace ns, char **str, size_t *length);
//
//react_status REACT_STDCALL react_property_namespace_add_ref(react_property_namespace ns);
//react_status REACT_STDCALL react_property_namespace_release(react_property_namespace ns);
//react_status REACT_STDCALL react_property_namespace_from_object(react_object obj, react_property_namespace *result);
//react_status REACT_STDCALL react_property_namespace_to_object(react_property_namespace ns, react_object *result);
//
//react_status REACT_STDCALL
//react_property_name_get(react_property_namespace ns, const char *name, size_t length, react_property_name *result);
//react_status REACT_STDCALL
//react_property_name_get_namespace(react_property_name name, react_property_namespace *result);
//react_status REACT_STDCALL react_property_name_get_local_string(react_property_name name, char **str, size_t *length);
//
//react_status REACT_STDCALL react_property_name_add_ref(react_property_name name);
//react_status REACT_STDCALL react_property_name_release(react_property_name name);
//react_status REACT_STDCALL react_property_name_from_object(react_object obj, react_property_name *result);
//react_status REACT_STDCALL react_property_name_to_object(react_property_name name, react_object *result);
//
//react_status REACT_STDCALL react_property_value_create_empty(react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_uint8(uint8_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_int16(int16_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_uint16(uint16_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_int32(int32_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_uint32(uint32_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_int64(int64_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_uint64(uint64_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_single(float value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_double(double value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_char16(char16_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_boolean(bool value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_string(react_string value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_object(react_object value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_datetime(react_datetime_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_timespan(react_timespan_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_guid(react_guid_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_point(react_point_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_size(react_size_t value, react_property_value *result);
//react_status REACT_STDCALL react_property_value_create_rect(react_rect_t value, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_uint8_array(uint8_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_int16_array(int16_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_uint16_array(uint16_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_int32_array(int32_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_uint32_array(uint32_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_int64_array(int64_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_uint64_array(uint64_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_single_array(float *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_double_array(double *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_char16_array(char16_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_boolean_array(bool *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_string_array(react_string *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_object_array(react_object *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_datetime_array(react_datetime_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_timespan_array(react_timespan_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_guid_array(react_guid_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_point_array(react_point_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_size_array(react_size_t *arr, size_t arr_length, react_property_value *result);
//react_status REACT_STDCALL
//react_property_value_create_rect_array(react_rect_t *arr, size_t arr_length, react_property_value *result);
//
//react_status REACT_STDCALL react_property_value_get_type(react_property_value value, react_property_type *result);
//
//react_status REACT_STDCALL react_property_get_uint8(react_property_value value, uint8_t *result);
//react_status REACT_STDCALL react_property_get_int16(react_property_value value, int16_t *result);
//react_status REACT_STDCALL react_property_get_uint16(react_property_value value, uint16_t *result);
//react_status REACT_STDCALL react_property_get_int32(react_property_value value, int32_t *result);
//react_status REACT_STDCALL react_property_get_uint32(react_property_value value, uint32_t *result);
//react_status REACT_STDCALL react_property_get_int64(react_property_value value, int64_t *result);
//react_status REACT_STDCALL react_property_get_uint64(react_property_value value, uint64_t *result);
//react_status REACT_STDCALL react_property_get_single(react_property_value value, float *result);
//react_status REACT_STDCALL react_property_get_double(react_property_value value, double *result);
//react_status REACT_STDCALL react_property_get_char16(react_property_value value, char16_t *result);
//react_status REACT_STDCALL react_property_get_boolean(react_property_value value, bool *result);
//react_status REACT_STDCALL react_property_get_string(react_property_value value, react_string *result);
//react_status REACT_STDCALL react_property_get_object(react_property_value value, react_object *result);
//react_status REACT_STDCALL react_property_get_datetime(react_property_value value, react_datetime_t *result);
//react_status REACT_STDCALL react_property_get_timespan(react_property_value value, react_timespan_t *result);
//react_status REACT_STDCALL react_property_get_guid(react_property_value value, react_guid_t *result);
//react_status REACT_STDCALL react_property_get_point(react_property_value value, react_point_t *result);
//react_status REACT_STDCALL react_property_get_size(react_property_value value, react_size_t *result);
//react_status REACT_STDCALL react_property_get_rect(react_property_value value, react_rect_t *result);
//react_status REACT_STDCALL react_property_get_uint8_array(react_property_value value, uint8_t *arr, size_t *arr_length);
//react_status REACT_STDCALL react_property_get_int16_array(react_property_value value, int16_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_uint16_array(react_property_value value, uint16_t *arr, size_t *arr_length);
//react_status REACT_STDCALL react_property_get_int32_array(react_property_value value, int32_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_uint32_array(react_property_value value, uint32_t *arr, size_t *arr_length);
//react_status REACT_STDCALL react_property_get_int64_array(react_property_value value, int64_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_uint64_array(react_property_value value, uint64_t *arr, size_t *arr_length);
//react_status REACT_STDCALL react_property_get_single_array(react_property_value value, float *arr, size_t *arr_length);
//react_status REACT_STDCALL react_property_get_double_array(react_property_value value, double *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_char16_array(react_property_value value, char16_t *arr, size_t *arr_length);
//react_status REACT_STDCALL react_property_get_boolean_array(react_property_value value, bool *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_string_array(react_property_value value, react_string *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_object_array(react_property_value value, react_object *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_datetime_array(react_property_value value, react_datetime_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_timespan_array(react_property_value value, react_timespan_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_guid_array(react_property_value value, react_guid_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_point_array(react_property_value value, react_point_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_size_array(react_property_value value, react_size_t *arr, size_t *arr_length);
//react_status REACT_STDCALL
//react_property_get_rect_array(react_property_value value, react_rect_t *arr, size_t *arr_length);
//
//react_status REACT_STDCALL react_property_value_add_ref(react_property_value value);
//react_status REACT_STDCALL react_property_value_release(react_property_value value);
//react_status REACT_STDCALL react_property_value_from_object(react_object obj, react_property_value *result);
//react_status REACT_STDCALL react_property_value_to_object(react_property_value value, react_object *result);
//
//react_status REACT_STDCALL react_property_bag_create(react_property_bag *result);
//react_status REACT_STDCALL react_property_bag_create_copy(react_property_bag bag, react_property_bag *result);
//react_status REACT_STDCALL
//react_property_bag_get_value(react_property_bag bag, react_property_name name, react_property_value *result);
//react_status REACT_STDCALL react_property_bag_get_or_create_value(
//    react_property_bag bag,
//    react_property_name name,
//    react_create_property_value_callback create_callback,
//    void *callback_data,
//    react_property_value *result);
//react_status REACT_STDCALL react_property_bag_set_value(
//    react_property_bag bag,
//    react_property_name name,
//    react_property_value value,
//    react_property_value *previous_value);
//
//react_status REACT_STDCALL react_property_bag_add_ref(react_property_bag bag);
//react_status REACT_STDCALL react_property_bag_release(react_property_bag bag);
//react_status REACT_STDCALL react_property_bag_to_object(react_property_bag bag, react_object *result);
//react_status REACT_STDCALL react_property_bag_from_object(react_object obj, react_property_bag *result);
//
//react_status REACT_STDCALL react_dispatcher_create(react_dispatcher *result);
//react_status REACT_STDCALL react_dispatcher_post(
//    react_dispatcher dispatcher,
//    react_dispatcher_callback invoke,
//    void *data,
//    react_destroy_callback destroy,
//    void *destroy_hint);
//react_status REACT_STDCALL react_dispatcher_invoke_else_post(
//    react_dispatcher dispatcher,
//    react_dispatcher_callback invoke,
//    void *data,
//    react_destroy_callback destroy,
//    void *destroy_hint);
//
//react_status REACT_STDCALL react_dispatcher_for_ui(react_dispatcher *result);
//
//react_status REACT_STDCALL react_dispatcher_add_ref(react_dispatcher dispatcher);
//react_status REACT_STDCALL react_dispatcher_release(react_dispatcher dispatcher);
//react_status REACT_STDCALL react_dispatcher_to_object(react_dispatcher dispatcher, react_object *result);
//react_status REACT_STDCALL react_dispatcher_from_object(react_object obj, react_dispatcher *dispatcher);
//
//react_status REACT_STDCALL react_property_name_ui_dispatcher(react_property_name *result);
//react_status REACT_STDCALL react_property_name_js_dispatcher(react_property_name *result);
//react_status REACT_STDCALL react_notification_name_js_dispatcher_task_starting(react_property_name *result);
//react_status REACT_STDCALL react_notification_name_js_dispatcher_idle_wait_starting(react_property_name *result);
//react_status REACT_STDCALL react_notification_name_js_dispatcher_idle_wait_completed(react_property_name *result);
//
//react_status REACT_STDCALL react_notification_subscription_get_service(
//    react_notification_subscription subscription,
//    react_notification_service *result);
//react_status REACT_STDCALL
//react_notification_subscription_get_name(react_notification_subscription subscription, react_property_name *result);
//react_status REACT_STDCALL
//react_notification_subscription_get_dispatcher(react_notification_subscription subscription, react_dispatcher *result);
//react_status REACT_STDCALL
//react_notification_subscription_is_subscribed(react_notification_subscription subscription, react_bool *result);
//react_status REACT_STDCALL react_notification_subscription_unsubscribe(react_notification_subscription subscription);
//
//react_status REACT_STDCALL react_notification_subscription_add_ref(react_notification_subscription subscription);
//react_status REACT_STDCALL react_notification_subscription_release(react_notification_subscription subscription);
//react_status REACT_STDCALL
//react_notification_subscription_to_object(react_notification_subscription subscription, react_object *result);
//react_status REACT_STDCALL
//react_notification_subscription_from_object(react_object obj, react_notification_subscription *result);
//
//react_status REACT_STDCALL react_notification_service_create(react_notification_service *result);
//react_status REACT_STDCALL react_notification_service_subscribe(
//    react_notification_service service,
//    react_property_name notification_name,
//    react_dispatcher dispatcher,
//    react_notification_handler_callback handler_callback,
//    void *handler_data,
//    react_destroy_callback handler_destroy_callback,
//    void *handler_destroy_hint);
//react_status REACT_STDCALL react_notification_service_notify(
//    react_notification_service service,
//    react_property_name notification_name,
//    react_object sender,
//    react_object data);
//
//react_status REACT_STDCALL react_notification_service_add_ref(react_notification_service service);
//react_status REACT_STDCALL react_notification_service_release(react_notification_service service);
//react_status REACT_STDCALL
//react_notification_service_to_object(react_notification_service service, react_object *result);
//react_status REACT_STDCALL react_notification_service_from_object(react_object obj, react_notification_service *result);
//
//react_status REACT_STDCALL react_host_builder_create(react_host_builder *result);
//react_status REACT_STDCALL react_host_builder_create_copy(react_host_builder builder, react_host_builder *result);
//react_status REACT_STDCALL
//react_host_builder_add_extension_package(react_host_builder builder, react_extension_package package);
//
//react_status REACT_STDCALL react_host_builder_add_ref(react_host_builder builder);
//react_status REACT_STDCALL react_host_builder_release(react_host_builder builder);
//react_status REACT_STDCALL react_host_builder_to_object(react_host_builder builder, react_object *result);
//react_status REACT_STDCALL react_host_builder_from_object(react_object obj, react_host_builder *result);
//
////  DOC_STRING("Used in @LogHandler to represent different LogLevels")
//// enum LogLevel { Trace = 0, Info = 1, Warning = 2, Error = 3, Fatal = 4 };
////
//// DOC_STRING("delegate to represent a logging function.")
//// delegate void LogHandler(LogLevel level, String message);
////
//// DOC_STRING("A JavaScript engine type that can be set for a React instance in
//// @ReactInstanceSettings.JSIEngineOverride")
////// Keep in sync with enum in IReactInstance.h
//// enum JSIEngine { Chakra = 0, Hermes = 1, V8 = 2 };
////
////[webhosthidden][default_interface] DOC_STRING(
////     "The arguments for the @ReactInstanceSettings.InstanceCreated event.") runtimeclass InstanceCreatedEventArgs {
////   DOC_STRING("Gets the @IReactContext for the React instance that was just created.")
////   IReactContext Context {
////     get;
////   };
//// }
////
////     [webhosthidden][default_interface] DOC_STRING("The arguments for the @ReactInstanceSettings.InstanceLoaded
////     event.")
////         runtimeclass InstanceLoadedEventArgs {
////   DOC_STRING("Gets the @IReactContext for the React instance that finished loading the bundle.")
////   IReactContext Context {
////     get;
////   };
////
////   DOC_STRING("Returns `true` if the JavaScript bundle failed to load.")
////   Boolean Failed {
////     get;
////   };
//// }
////
////[webhosthidden][default_interface] DOC_STRING("The arguments for the @ReactInstanceSettings.InstanceDestroyed event.")
////     runtimeclass InstanceDestroyedEventArgs {
////       DOC_STRING("Gets the @IReactContext for the React instance that just destroyed.")
////       IReactContext Context {
////         get;
////       };
////     }
////
////         [webhosthidden] DOC_STRING("Provides settings to create a React instance.") runtimeclass
////         ReactInstanceSettings {
////   DOC_STRING("Creates new instance of @ReactInstanceSettings")
////   ReactInstanceSettings();
////
////   DOC_STRING(
////       "Gets a @IReactPropertyBag to share values between components and the application.\n"
////       "Use @IReactContext.Properties to access this @IReactPropertyBag from native components and view managers.")
////   IReactPropertyBag Properties {
////     get;
////   };
////
////   DOC_STRING(
////       "Gets a @IReactNotificationService to send notifications between components and the application.\n"
////       "Use @IReactContext.Notifications to access this @IReactNotificationService "
////       "from native components or view managers.")
////   IReactNotificationService Notifications {
////     get;
////   };
////
////   DOC_STRING(
////       "Gets a list of @IReactPackageProvider.\n"
////       "Add an implementation of @IReactPackageProvider to this list to define additional "
////       "native modules and custom view managers to be included in the React instance.\n"
////       "Auto-linking automatically adds @IReactPackageProvider to the application's @.PackageProviders.")
////   IVector<IReactPackageProvider> PackageProviders {
////     get;
////   };
////
////   DOC_STRING(
////       "This controls whether various developer experience features are available for this instance. "
////       "In particular, it enables the developer menu, the default `RedBox` and `LogBox` experience.")
////   Boolean UseDeveloperSupport {
////     get;
////     set;
////   };
////
////   DOC_STRING(
////       "The name of the JavaScript bundle file to load. This should be a relative path from @.BundleRootPath. "
////       "The `.bundle` extension will be appended to the end, when looking for the bundle file.\n"
////       "If using an embedded RCDATA resource, this identifies the resource ID that stores the bundle. See
////       @.BundleRootPath.")
////   DOC_DEFAULT("index.windows")
////   String JavaScriptBundleFile {
////     get;
////     set;
////   };
////
////   DOC_STRING(
////       "Controls whether the instance JavaScript runs in a remote environment such as within a browser.\n"
////       "By default, this is using a browser navigated to http://localhost:8081/debugger-ui served by Metro/Haul.\n"
////       "Debugging will start as soon as the react native instance is loaded.")
////   Boolean UseWebDebugger {
////     get;
////     set;
////   };
////
////   DOC_STRING(
////       "Controls whether the instance triggers the hot module reload logic when it first loads the instance.\n"
////       "Most edits should be visible within a second or two without the instance having to reload.\n"
////       "Non-compatible changes still cause full reloads.\n"
////       "See [Fast Refresh](https://reactnative.dev/docs/fast-refresh) for more information on Fast Refresh.")
////   Boolean UseFastRefresh {
////     get;
////     set;
////   };
////
//
//typedef enum {
//  react_instance_use_direct_debugging = 1 << 0,
//  react_instance_debugger_break_on_start = 1 << 1,
//  react_instance_enable_jit_compilation = 1 << 2,
//  react_instance_enable_bytecode_caching = 1 << 3,
//  react_instance_enable_default_crash_handler = 1 << 4,
//  react_instance_enable_request_inline_source_map = 1 << 5,
//} react_instance_flags;
//
//react_status REACT_STDCALL
//react_instance_builder_set_flags(react_instance_builder instance, react_instance_flags flags);
//react_status REACT_STDCALL
//react_instance_builder_get_flags(react_instance_builder instance, react_instance_flags *result);
//
//react_status REACT_STDCALL
//react_instance_builder_set_byte_code_file_uri(react_instance_builder instance, const char *value, size_t value_length);
//
//react_status REACT_STDCALL
//react_instance_builder_set_debug_bundle_path(react_instance_builder instance, const char *value, size_t value_length);
//
//react_status REACT_STDCALL react_instance_builder_set_bundle_root_path(
//    react_instance_builder instance, const char *value, size_t value_length);
//
//react_status REACT_STDCALL react_instance_builder_set_debugger_port(
//    react_instance_builder instance,
//    uint16_t value);
//
//react_status REACT_STDCALL
//react_instance_builder_set_debugger_runtime_name(react_instance_builder instance, const char *value, size_t value_length);
//
//react_status REACT_STDCALL react_instance_builder_set_red_box_handler(
//    react_instance_builder instance, react_red_box_handler handler);
//
//react_status REACT_STDCALL
//react_instance_builder_set_native_logger(react_instance_builder instance, react_log_handler handler);
//
//react_status REACT_STDCALL
//react_instance_builder_set_ui_dispatcher(react_instance_builder instance, react_dispatcher dispatcher);
//
//react_status REACT_STDCALL
//react_instance_builder_set_source_bundle_host(react_instance_builder instance, const char *value, size_t value_length);
//
//react_status REACT_STDCALL
//react_instance_builder_set_source_bundle_port(react_instance_builder instance, uint16_t port);
//
//react_status REACT_STDCALL
//react_instance_builder_set_js_engine(react_instance_builder instance, react_js_engine engine);
//
//
////   DOC_STRING(
////       "The @InstanceCreated event is triggered right after the React Native instance is created.\n"
////       "\n"
////       "It is triggered on the JSDispatcher thread before any other JSDispatcher work items.\n"
////       "No JavaScript code is loaded in the JavaScript engine yet.\n"
////       "The @InstanceCreatedEventArgs.Context property on the event arguments provides access to the instance
////       context.\n"
////       "\n"
////       "Note that the @InstanceCreated event is triggered in response to the 'InstanceCreated' notification "
////       "raised in the 'ReactNative.InstanceSettings' namespace. "
////       "Consider using @Notifications to handle the notification in a dispatcher different from the JSDispatcher.")
////   event Windows.Foundation.EventHandler<InstanceCreatedEventArgs> InstanceCreated;
////
////   DOC_STRING(
////       "The @InstanceLoaded event is triggered when React Native instance has finished loading the JavaScript
////       bundle.\n"
////       "\n"
////       "It is triggered on the JSDispatcher thread.\n"
////       "If there were errors, then the @InstanceLoadedEventArgs.Failed property on the event arguments will be
////       true.\n" "The error types include:\n"
////       "\n"
////       "* JavaScript syntax errors.\n"
////       "* Global JavaScript exceptions thrown.\n"
////       "\n"
////       "The @InstanceLoadedEventArgs.Context property on the event arguments provides access to the instance
////       context.\n"
////       "\n"
////       "Note that the @InstanceLoaded event is triggered in response to the 'InstanceLoaded' notification "
////       "raised in the 'ReactNative.InstanceSettings' namespace. "
////       "Consider using @Notifications to handle the notification in a dispatcher different from the JSDispatcher.")
////   event Windows.Foundation.EventHandler<InstanceLoadedEventArgs> InstanceLoaded;
////
////   DOC_STRING(
////       "The @InstanceDestroyed event is triggered when React Native instance is destroyed.\n"
////       "\n"
////       "It is triggered on the JSDispatcher thread as the last work item before it shuts down.\n"
////       "No new JSDispatcher work can be executed after that.\n"
////       "The @InstanceDestroyedEventArgs.Context property on the event arguments provides access to the instance
////       context.\n"
////       "\n"
////       "Note that the @InstanceDestroyed event is triggered in response to the 'InstanceDestroyed' notification "
////       "raised in the 'ReactNative.InstanceSettings' namespace. "
////       "Consider using @Notifications to handle the notification in a dispatcher different from the JSDispatcher.")
////   event Windows.Foundation.EventHandler<InstanceDestroyedEventArgs> InstanceDestroyed;
//// }
//
//react_status REACT_STDCALL react_host_add_ref(react_host host);
//react_status REACT_STDCALL react_host_release(react_host host);
//react_status REACT_STDCALL react_host_to_object(react_host host, react_object *result);
//react_status REACT_STDCALL react_host_from_object(react_object obj, react_host *result);
//
////    runtimeclass ReactNativeHost {
////      DOC_STRING("Creates a new instance of @ReactNativeHost")
////      ReactNativeHost();
////
////      DOC_STRING(
////          "Provides access to the list of @IReactPackageProvider's that the React instance will use to provide "
////          "native modules to the application. This can be used to register additional package providers, such as "
////          "package providers from community modules or other shared libraries.")
////      IVector<IReactPackageProvider> PackageProviders {
////        get;
////      };
////
////      DOC_STRING("Provides access to this host's @ReactInstanceSettings to configure the react instance.")
////      ReactInstanceSettings InstanceSettings {
////        get;
////        set;
////      };
////
////      DOC_STRING("Loads a new React instance. It is an alias for @.ReloadInstance method.")
////      Windows.Foundation.IAsyncAction LoadInstance();
////
////      DOC_STRING(
////          "Unloads the current React instance and loads a new one.\n"
////          "The React instance loading creates an instance of the JavaScript engine, "
////          "and launches the provided JavaScript code bundle.\n"
////          "If a React instance is already running in this host, then @.ReloadInstance shuts down the already "
////          "the running React instance, and loads a new React instance.\n"
////          "The React instance lifecycle can be observed with the following events:"
////          "- The @ReactInstanceSettings.InstanceCreated event is raised when the React instance is just created.\n"
////          "- The @ReactInstanceSettings.InstanceLoaded event is raised when the React instance completed"
////          "  loading the JavaScript bundle.\n"
////          "- The @ReactInstanceSettings.InstanceDestroyed event is raised when the React instance is destroyed.")
////      Windows.Foundation.IAsyncAction ReloadInstance();
////
////      DOC_STRING(
////          "Unloads current React instance.\n"
////          "After the React instance is unloaded, all the React resources including the JavaScript engine environment "
////          "are cleaned up.\n"
////          "The React instance destruction can be observed with the @ReactInstanceSettings.InstanceDestroyed event.")
////      Windows.Foundation.IAsyncAction UnloadInstance();
////
////      DOC_STRING("Returns the @ReactNativeHost instance associated with the given @IReactContext.")
////      static ReactNativeHost FromContext(IReactContext reactContext);
////    }
////} // namespace ReactNative

REACT_EXTERN_C_END

#endif // !REACT_API_H_

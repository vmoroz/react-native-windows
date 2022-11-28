#include "ReactCommonApi.h"
#include "ReactObjectImpl.h"

using namespace react;

REACT_API react_object_add_ref(react_object_t obj) {
  reinterpret_cast<ReactObjectImpl*>(obj.obj_)->AddRef();
  return react_status_ok;
}

REACT_API react_object_release(react_object_t obj) {
  reinterpret_cast<ReactObjectImpl*>(obj.obj_)->Release();
  return react_status_ok;
}

REACT_API react_object_create(react_object_t* result) {
  *result = react_object_t(reinterpret_cast<struct react_object_s*>(new ReactObjectImpl()));
  return react_status_ok;
}

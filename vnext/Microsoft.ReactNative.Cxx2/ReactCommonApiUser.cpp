#include "ReactCommonApi.h"
#include "ReactObject.h"

namespace react {

void HandleHolder::CheckedAddRef(void* handle) noexcept {
  react_object_add_ref(react_object_t(reinterpret_cast<react_object_s*>(handle)));
}

/*static*/ void HandleHolder::CheckedRelease(void* handle) noexcept {
  react_object_release(react_object_t(reinterpret_cast<react_object_s*>(handle)));
}

ReactObject::ReactObject() noexcept {
  react_object_t obj { nullptr };
  react_object_create(&obj);
  handle_ = HandleHolder(obj.obj_);
}

} // namespace react

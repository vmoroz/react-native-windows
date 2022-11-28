#include "ReactObjectImpl.h"
#include "ReactObject.h"

namespace react {

void HandleHolder::CheckedAddRef(void* handle) noexcept {
  reinterpret_cast<ReactObjectImpl*>(handle)->AddRef();
}

/*static*/ void HandleHolder::CheckedRelease(void* handle) noexcept {
  if (handle != nullptr)
  {
    reinterpret_cast<ReactObjectImpl*>(handle)->Release();
  }
}

ReactObject::ReactObject() noexcept : handle_(HandleHolder(new ReactObjectImpl())) {}

} // namespace react

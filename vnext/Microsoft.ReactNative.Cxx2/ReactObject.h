// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#ifndef REACT_OBJECT_H_
#define REACT_OBJECT_H_

#include <atomic>
#include <utility>

namespace react {

class HandleHolder
{
public:
  HandleHolder(std::nullptr_t = nullptr) {}
  HandleHolder(void* handle) noexcept : handle_(handle) {}

  HandleHolder(const HandleHolder& other) noexcept : handle_(other.handle_) {
    CheckedAddRef(handle_);
  }

  HandleHolder& operator=(const HandleHolder& other) noexcept {
    if (this != &other) {
      HandleHolder temp(std::move(*this));
      handle_ = other.handle_;
      CheckedAddRef(handle_);
    }
    return *this;
  }

  HandleHolder(HandleHolder&& other) noexcept : handle_(std::exchange(other.handle_, nullptr)) { }

  HandleHolder& operator=(HandleHolder&& other) noexcept {
    if (this != &other) {
      HandleHolder temp(std::move(*this));
      handle_ = std::exchange(other.handle_, nullptr);
    }
    return *this;
  }

  void* GetHandle() const noexcept {
    return handle_;
  }

private:
  static void CheckedAddRef(void* handle) noexcept;
  static void CheckedRelease(void* handle) noexcept;

private:
  void* handle_ { nullptr };
};

class ReactObject {
public:
  struct Handle;

  ReactObject() noexcept;
  explicit ReactObject(std::nullptr_t) {}
  explicit ReactObject(Handle* handle) noexcept : handle_(handle) {}

  Handle* GetHandle() const noexcept {
    return reinterpret_cast<Handle*>(handle_.GetHandle());
  }

  explicit operator bool() const noexcept {
    return handle_.GetHandle() != nullptr;
  }

private:
  HandleHolder handle_;
};

} // namespace react

#endif // !REACT_OBJECT_H_

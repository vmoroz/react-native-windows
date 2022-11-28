// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#ifndef REACT_OBJECT_IMPL_H
#define REACT_OBJECT_IMPL_H

#include <atomic>

namespace react {

class ReactObjectImpl {
public:
  void AddRef() noexcept {
    ++refCount_;
  }

  void Release() noexcept {
    if (--refCount_ == 0) {
      DestroyThis();
    }
  }

  virtual void DestroyThis() noexcept {
    delete this;
  }

  ReactObjectImpl() = default;
  virtual ~ReactObjectImpl() = default;

private:
  std::atomic<uint32_t> refCount_;
};

} // namespace react

#endif // !REACT_OBJECT_IMPL_H



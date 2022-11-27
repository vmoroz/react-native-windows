// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACT_REACTOBJECT_H
#define MICROSOFT_REACT_REACTOBJECT_H

#include <atomic>

namespace Microsoft::React {

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

protected:
  ReactObjectImpl() = default;
  virtual ~ReactObjectImpl() = default;

private:
  std::atomic<uint32_t> refCount_;
};

}

#endif // !MICROSOFT_REACT_REACTOBJECT_H



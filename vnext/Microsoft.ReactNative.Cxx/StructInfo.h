// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACTNATIVE_STRUCTINFO
#define MICROSOFT_REACTNATIVE_STRUCTINFO

#include "winrt/Microsoft.ReactNative.Bridge.h"

namespace winrt::Microsoft::ReactNative::Bridge {

struct FieldInfo;
using FieldMap = std::map<std::wstring, FieldInfo, std::less<>>;
using FieldReaderType =
    void (*)(IJSValueReader & /*reader*/, void * /*obj*/, const uintptr_t * /*fieldPtrStore*/) noexcept;
using FieldWriterType =
    void (*)(IJSValueWriter & /*writer*/, void * /*obj*/, const uintptr_t * /*fieldPtrStore*/) noexcept;

template <class T>
void GetStructInfo(T *) {}

template <class TClass, class TValue>
void FieldReader(IJSValueReader &reader, void *obj, const uintptr_t *fieldPtrStore) noexcept;

template <class TClass, class TValue>
void FieldWriter(IJSValueWriter &writer, void *obj, const uintptr_t *fieldPtrStore) noexcept;

struct FieldInfo {
  template <class TClass, class TValue>
  FieldInfo(TValue TClass::*fieldPtr) noexcept
      : m_fieldReader{FieldReader<TClass, TValue>},
        m_fieldWriter{FieldWriter<TClass, TValue>},
        m_fieldPtrStore{*reinterpret_cast<uintptr_t *>(&fieldPtr)} {
    static_assert(sizeof(m_fieldPtrStore) >= sizeof(fieldPtr));
  }

  void ReadField(IJSValueReader &reader, void *obj) const noexcept {
    m_fieldReader(reader, obj, &m_fieldPtrStore);
  }

  void WriteField(IJSValueWriter &writer, void *obj) const noexcept {
    m_fieldWriter(writer, obj, &m_fieldPtrStore);
  }

 private:
  FieldReaderType m_fieldReader;
  FieldWriterType m_fieldWriter;
  const uintptr_t m_fieldPtrStore;
};

template <class TClass, class TValue>
void FieldReader(IJSValueReader &reader, void *obj, const uintptr_t *fieldPtrStore) noexcept {
  using FieldPtrType = TValue TClass::*;
  ReadValue(reader, /*out*/ static_cast<TClass *>(obj)->*(*reinterpret_cast<const FieldPtrType *>(fieldPtrStore)));
}

template <class TClass, class TValue>
void FieldWriter(IJSValueWriter const & writer, void * obj, const uintptr_t * fieldPtrStore) noexcept {
  using FieldPtrType = TValue TClass::*;
  WriteValue(writer, static_cast<TClass *>(obj)->*(*reinterpret_cast<const FieldPtrType *>(fieldPtrStore)));
}

template <class T>
struct StructInfo {
  static const FieldMap FieldMap;
};

template <class T>
/*static*/ const FieldMap StructInfo<T>::FieldMap = GetStructInfo(static_cast<T *>(nullptr));

} // namespace winrt::Microsoft::ReactNative::Bridge

#endif // MICROSOFT_REACTNATIVE_STRUCTINFO

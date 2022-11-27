// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACT_REACTPROPERTYBAG_H
#define MICROSOFT_REACT_REACTPROPERTYBAG_H

#include "ReactObject.h"
#include <mutex>
#include <map>

namespace Microsoft::React {

//class ReactPropertyBag final : ReactObject {
//public:
//  ReactPropertyBag() = default;
//
//  IInspectable Get(IReactPropertyName const& name) noexcept;
//  IInspectable GetOrCreate(IReactPropertyName const& name, ReactCreatePropertyValue const& createValue) noexcept;
//  IInspectable Set(IReactPropertyName const& name, IInspectable const& value) noexcept;
//
//  void CopyFrom(IReactPropertyBag const&) noexcept;
//
//  static IReactPropertyNamespace GlobalNamespace() noexcept;
//  static IReactPropertyNamespace GetNamespace(hstring const& namespaceName) noexcept;
//  static IReactPropertyName GetName(IReactPropertyNamespace const& ns, hstring const& localName) noexcept;
//  static IReactPropertyBag CreatePropertyBag() noexcept;
//
//private:
//  std::mutex m_mutex;
//  std::map<IReactPropertyName, IInspectable> m_entries;
//};

} // namespace Microsoft::React

#endif // !MICROSOFT_REACT_REACTPROPERTYBAG_H

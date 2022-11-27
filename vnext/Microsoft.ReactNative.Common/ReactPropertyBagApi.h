// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#ifndef MICROSOFT_REACT_REACTPROPERTYBAG_H
#define MICROSOFT_REACT_REACTPROPERTYBAG_H

#include <chrono>
#include <functional>
#include <string_view>
#include <type_traits>

//#include <array_view>

template <typename T>
struct array_view
{

};

//
// ReactPropertyBag is a thread-safe storage of property values.
// Internally the value is IInspectable and the key is a name object that represents an
// atomized string name. Each name object is defined in the context of a namespace object.
// The null or empty namespace object is a global namespace object.
// The property name is unique for the same namespace object.
// Different namespaces may have properties with the same local names.
//
// The ReactPropertyBag struct is a wrapper around the ABI-safe IReactPropertyBag interface.
// The IReactPropertyBag represents all values as IInspectable object which can wrap any type.
// On top of the untyped IReactPropertyBag values, the ReactPropertyBag offers a set of typed
// property accessors: Get, GetOrCreate, Remove, and Set.
//
// To simplify access to properties we offer ReactPropertyId type that helps to
// wrap up property name and type into one variable.
//
// For example, we can define a property to access an integer value:
//
//     const React::ReactPropertyId<int> MyIntProperty{L"MyInt"};
//
// then we can use it to set property in settings properties during initialization:
//
//     settings.Properties().Set(MyIntProperty, 42);
//
// The property can be accessed later in a native modules:
//
//     std::optional<int> myInt = context.Properties().Get(MyIntProperty);
//
// Note that types inherited from IInspectable are returned
// directly because their null value may indicated absent property value.
// For other types we return std::optional<T>. It has std::nullopt value in case if
// no property value is found or if it has a wrong type.
//
// To pass values through the ABI boundary the non-IInspectable types must be WinRT types
// which are described here:
// https://docs.microsoft.com/uwp/api/windows.foundation.propertytype
//
// In case if we do not have a requirement to pass values across the DLL/EXE boundary,
// we can use the ReactNonAbiValue<T> wrapper to store non-ABI safe values.
// The type ReactNonAbiValue<T> is a smart pointer similar to winrt::com_ptr
// or winrt::Windows::Foundation::IInspactable. It is treated as IInspectable type and
// **is not** wrapped in std::optional. It can be casted to bool to check if it is null.
//
// For example, we can define a property to use in our DLL or EXE to store std::string:
//
//     const React::ReactPropertyId<React::ReactNonAbValue<std::string>> MyStringProperty{L"MyString"};
//
// then we can use it to set and get property value:
//
//     context.Properties().Set(MyStringProperty, "Hello");
//     assert("Hello" == context.Properties().Get(MyStringProperty);
//
// The first line above creates a new React::ReactNonAbValue<std::string> value that internally.
// allocates a ref-counted wrapper React::implementation::ReactNonAbValue<std::string> in the heap.
// Then this value is stored in the IReactPropertyBag and can be safely retrieved in the
// same DLL or EXE module. Using it from a different module may cause a security bug or a crash.
//
// Note that for passing the ABI-safe strings we must use the winrt::hstring:
//
//     const React::ReactPropertyId<winrt::hstring> MyAbiSafeStringProperty{L"MyAbiSafeString"};
//

//#include <winrt/Microsoft.ReactNative.h>
//#include <winrt/Windows.Foundation.h>
#include <optional>
//#include "ReactHandleHelper.h"
//#include "ReactNonAbiValue.h"
#include <string_view>

namespace Microsoft::React {

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

  ReactObject(std::nullptr_t = nullptr) {}
  explicit ReactObject(Handle* handle) noexcept : handle_(handle) {}

  Handle* GetHandle() const noexcept {
    return reinterpret_cast<Handle*>(handle_.GetHandle());
  }

  explicit operator bool() const noexcept {
    return handle_.GetHandle() != nullptr;
  }

  static ReactObject BoxValue(uint8_t value);
  static ReactObject BoxValue(uint16_t value);
  static ReactObject BoxValue(uint32_t value);
  static ReactObject BoxValue(uint64_t value);
  static ReactObject BoxValue(int16_t value);
  static ReactObject BoxValue(int32_t value);
  static ReactObject BoxValue(int64_t value);
  static ReactObject BoxValue(float value);
  static ReactObject BoxValue(double value);
  static ReactObject BoxValue(char16_t value);
  static ReactObject BoxValue(bool value);
  static ReactObject BoxValue(std::string_view value);
  static ReactObject BoxValue(std::u16string_view value);
  static ReactObject BoxValue(const ReactObject& value);

  static ReactObject BoxValue(const array_view<uint8_t>& value);
  static ReactObject BoxValue(const array_view<uint16_t>& value);
  static ReactObject BoxValue(const array_view<uint32_t>& value);
  static ReactObject BoxValue(const array_view<uint64_t>& value);
  static ReactObject BoxValue(const array_view<int16_t>& value);
  static ReactObject BoxValue(const array_view<int32_t>& value);
  static ReactObject BoxValue(const array_view<int64_t>& value);
  static ReactObject BoxValue(const array_view<float>& value);
  static ReactObject BoxValue(const array_view<double>& value);
  static ReactObject BoxValue(const array_view<char16_t>& value);
  static ReactObject BoxValue(const array_view<bool>& value);
  static ReactObject BoxValue(const array_view<std::string_view>& value);
  static ReactObject BoxValue(const array_view<std::u16string_view>& value);
  static ReactObject BoxValue(const array_view<ReactObject>& value);

  template <typename T>
  auto UnboxValue() noexcept {
    if constexpr (std::is_base_of_v<ReactObject, T>) {
      return T::FromObject(*this);
    }
    if constexpr (std::is_enum_v<T>) {
      std::underlying_type_t<T>* valuePtr { nullptr };
      if (TryUnboxValue(&valuePtr) && valuePtr) {
        return std::optional<T>(static_cast<T>(*valuePtr));
      }
    }
    if constexpr (std::is_same_v<T, GUID>) {
      guid* valuePtr;
      if (TryUnboxValue(&valuePtr) && valuePtr) {
        return std::optional<T>(*reinterpret_cast<T*>(valuePtr));
      }
    }
    if constexpr (IsBoxableV<T>) {
      T* valuePtr;
      if (TryUnboxValue(&valuePtr) && valuePtr) {
        return std::optional<T>(*reinterpret_cast<T*>(valuePtr));
      }
    }

    return std::optional<T>();
  }

private:
  HandleHolder handle_;
};


//// Box value to an ABI-safe object.
//template <class T, class TValue, std::enable_if_t<!IsReactNonAbiValueV<T> || std::is_same_v<T, TValue>, int> = 0>
//static ReactObject ToObject(const TValue& value) noexcept {
//  // We box WinRT types and return IInspectable-inherited values as-is.
//  // The ReactNonAbiValue<U> is treated as IInspectable-inherited value if TValue=='ReactNonAbiValue<U>'.
//  return ReactObject::BoxValue(value);
//}
//
//// Box value to an ABI-safe object.
//template <class T, class TValue, std::enable_if_t<IsReactNonAbiValueV<T> && !std::is_same_v<T, TValue>, int> = 0>
//static ReactObject ToObject(TValue&& value) noexcept {
//  // Create ReactNonAbiValue<U> with newly allocated wrapper for U and pass TValue as an argument to the U
//  // constructor. For example, we can pass TValue=='const char*' to U=='std::string' where
//  // T=='ReactNonAbiValue<std::string>'.
//  return T { std::in_place, std::forward<TValue>(value) };
//}
//
//// Box value to an ABI-safe object.
//template <class T>
//static ReactObject ToObject(std::optional<T> const& value) noexcept {
//  return value ? ToObject<T>(*value) : nullptr;
//}
//
//// Unbox value from an ABI-safe object.
//template <class T>
//static auto FromObject(const ReactObject& obj) noexcept {
//  // The code mostly borrowed from the winrt::unbox_value_or implementation to return
//  // empty std::optional in case if obj is null or has a wrong type.
//  if constexpr (std::is_base_of_v<ReactObject, T>) {
//    return obj.try_as<T>();
//#ifndef __APPLE__
//  }
//  else if constexpr (impl::has_category_v<T>) {
//    if (obj) {
//#ifdef WINRT_IMPL_IUNKNOWN_DEFINED
//      if constexpr (std::is_same_v<T, GUID>) {
//        if (auto temp = obj.try_as<Windows::Foundation::IReference<guid>>()) {
//          return std::optional<T>{temp.Value()};
//        }
//      }
//#endif
//      if (auto temp = obj.try_as<Windows::Foundation::IReference<T>>()) {
//        return std::optional<T>{temp.Value()};
//      }
//
//      if constexpr (std::is_enum_v<T>) {
//        if (auto temp = obj.try_as<Windows::Foundation::IReference<std::underlying_type_t<T>>>()) {
//          return std::optional<T>{static_cast<T>(temp.Value())};
//        }
//      }
//    }
//
//    return std::optional<T>{};
//#endif
//  }
//}


template <class T>
struct ReactNonAbiValue : ReactObject {
  // Create a new instance of implementation::ReactNonAbiValue with args and keep a ref-counted pointer to it.
  template <class... TArgs>
  ReactNonAbiValue(std::in_place_t, TArgs &&...args) noexcept
    : ReactNonAbiValue(MakeNonAbiValue(std::forward<TArgs>(args)...)) {}

  // Create an empty ReactNonAbiValue.
  ReactNonAbiValue(std::nullptr_t = nullptr) noexcept {}

  // Create a ReactNonAbiValue with taking the ownership from the provided pointer.
  ReactNonAbiValue(Handle* handle) noexcept : ReactObject(handle) {}

  //// Get a pointer to the value from the object it implements IReactNonAbiValue.
  //// The method is unsafe because it provides no protection in case if the object has a value of different type.
  //// Treat this method as if you would cast to a value type from 'void*' type.
  //// The method returns nullptr if obj doe snot implement the IReactNonAbiValue interface.
  //static T* GetPtrUnsafe(IInspectable const& obj) noexcept {
  //  if (IReactNonAbiValue temp = obj.try_as<IReactNonAbiValue>()) {
  //    return reinterpret_cast<T*>(temp.GetPtr());
  //  }
  //  else {
  //    return nullptr;
  //  }
  //}

  // Get pointer to the stored value.
  // Return nullptr if ReactNonAbiValue is empty.
  T* GetPtr() const noexcept {
    return reinterpret_cast<T*>(GetVoidPtr());
  }

  // Get pointer to the stored value.
  // Return nullptr if ReactNonAbiValue is empty.
  T* operator->() const noexcept {
    return GetPtr();
  }

  // Get a reference to the stored value.
  // Crash the app if ReactNonAbiValue is empty.
  T& operator*() const noexcept {
    return *GetPtr();
  }

  // Get a reference to the stored value.
  // Crash the app if ReactNonAbiValue is empty.
  T& Value() const noexcept {
    return *GetPtr();
  }

  // Call the call operator() for the stored value.
  // Crash the app if ReactNonAbiValue is empty.
  template <typename... TArgs>
  auto operator()(TArgs &&...args) const {
    return (*GetPtr())(std::forward<TArgs>(args)...);
  }

private:
  template <class... TArgs>
  Handle* MakeNonAbiValue(std::forward<TArgs>(args)...) {
    void* storage { nullptr };
    Handle* value = CreateValue(sizeof(T), &DestroyValue, &storage);
    ::new(storage)T(std::forward<TArgs>(args)...);
    return value;
  }

  using DestroyPtr = void (*)(void* obj);

  static Handle* CreateValue(size_t size, DestroyPtr destroy, void** storage);

  static void DestroyValue(void* obj) {
    reinterpret_cast<T*>(obj)->~T();
  }

  void* GetVoidPtr() const noexcept;
};

// Type traits to check if type T is a IsReactNonAbiValue.
template <class T>
struct IsReactNonAbiValue : std::false_type {};
template <class T>
struct IsReactNonAbiValue<ReactNonAbiValue<T>> : std::true_type {};

// A shortcut for the value of the IsReactNonAbiValue type traits.
template <class T>
constexpr bool IsReactNonAbiValueV = IsReactNonAbiValue<T>::value;

// ReactPropertyNamespace encapsulates the IReactPropertyNamespace.
// It represents an atomic property namespace object.
class ReactPropertyNamespace {
public:
  struct Handle;

  ReactPropertyNamespace(std::nullptr_t = nullptr) {}
  explicit ReactPropertyNamespace(Handle* handle) noexcept : handle_(handle) {}

  static ReactPropertyNamespace FromString(std::string_view namespaceName) noexcept;
  static ReactPropertyNamespace Local() noexcept;

  std::string_view NamespaceName() const noexcept;

  Handle* GetHandle() const noexcept {
    return reinterpret_cast<Handle*>(handle_.GetHandle());
  }

  explicit operator bool() const noexcept {
    return handle_.GetHandle() != nullptr;
  }

private:
  HandleHolder handle_;
};

// ReactPropertyName encapsulates the IReactPropertyName.
// It represents an atomic property name object that defines a LocalName
// within the referenced Namespace.
class ReactPropertyName {
public:
  struct Handle;

  ReactPropertyName(std::nullptr_t = nullptr) noexcept {}
  explicit ReactPropertyName(Handle* handle) noexcept : handle_(handle) {}

  explicit ReactPropertyName(std::string_view localName) noexcept;
  ReactPropertyName(const ReactPropertyNamespace& ns, std::string_view localName) noexcept;
  ReactPropertyName(std::string_view namespaceName, std::string_view localName) noexcept;

  ReactPropertyNamespace Namespace() const noexcept;

  std::string_view NamespaceName() const noexcept;
  std::string_view LocalName() const noexcept;

  Handle* GetHandle() const noexcept {
    return reinterpret_cast<Handle*>(handle_.GetHandle());
  }

  explicit operator bool() const noexcept {
    return handle_.GetHandle() != nullptr;
  }

private:
  HandleHolder handle_;
};

// Encapsulates the IReactPropertyName and the property type
template <typename T>
struct ReactPropertyId : ReactPropertyName {
  using PropertyType = T;
  using ReactPropertyName::ReactPropertyName;
};

// ReactPropertyBag is a wrapper for IReactPropertyBag to store strongly-typed properties in a thread-safe way.
// Types inherited from IInspectable are stored directly.
// Values of other types are boxed with help of winrt::box_value.
// Non-WinRT types are wrapped with the help of BoxedValue template.
class ReactPropertyBag {
public:
  struct Handle;

  // Create a new empty instance of ReactPropertyBag.
  ReactPropertyBag(std::nullptr_t = nullptr) noexcept {}

  // Creates a new instance of ReactPropertyBag with the provided handle.
  explicit ReactPropertyBag(Handle* handle) noexcept : handle_(handle) {}

  // Property result type is either T or std::optional<T>.
  // T is returned for types inherited from IInspectable.
  // The std::optional<T> is returned for all other types.
  template <class T>
  using ResultType = std::conditional_t<std::is_base_of_v<ReactObject, T>, T, std::optional<T>>;

  // Get property value by property name.
  template <class T>
  ResultType<T> Get(const ReactPropertyId<T>& propertyId) noexcept {
    ReactObject obj = GetHandle() ? GetValue(GetHandle(), propertyId.GetHandle()) : nullptr;
    return obj.UnboxValue<T>(propertyValue);
  }

  // Ensure that property is created by calling valueCreator if needed, and return value by property name.
  // The TCreateValue must return either T or std::optional<T>. It must have no parameters.
  template <class T, class TCreateValue>
  ResultType<T> GetOrCreate(
    const ReactPropertyId<T>& propertyId,
    const TCreateValue& createValue) noexcept {
    ReactObject value = GetHandle()
      ? GetOrCreateValue(GetHandle(), propertyId.GetHandle(), [&createValue]() noexcept { return ReactObject::BoxValue(createValue()); })
      : nullptr;
    return obj.UnboxValue<T>(value);
  }

  // Set property value by property name.
  template <typename T, typename TValue>
  void Set(const ReactPropertyId<T>& propertyId, const TValue& value) const noexcept {
    ReactObject value = ReactObject::BoxValue(value);
    SetValue(GetHandle(), propertyId.GetHandle(), value.GetHandle());
  }

  // Removes property value by property name.
  template <typename T>
  void Remove(const ReactPropertyId<T>& propertyId) const noexcept {
    RemoveValue(GetHandle(), propertyId.GetHandle());
  }

  Handle* GetHandle() const noexcept {
    return reinterpret_cast<Handle*>(handle_.GetHandle());
  }

  explicit operator bool() const noexcept {
    return handle_.GetHandle() != nullptr;
  }

private:
  ReactObject GetValue(const ReactPropertyName& property) noexcept;
  ReactObject GetOrCreateValue(const ReactPropertyName& property, const std::function<ReactObject()>& createValue) noexcept;
  void SetValue(const ReactPropertyName& property, const ReactObject& value) noexcept;
  void RemoveValue(const ReactPropertyName& property) noexcept;

private:
  HandleHolder handle_;
};

} // namespace Microsoft::React

#endif // MICROSOFT_REACT_REACTPROPERTYBAG_H

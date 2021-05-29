// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// IMPORTANT: Before updating this file
// please read react-native-windows repo:
// vnext/Microsoft.ReactNative.Cxx/README.md

#pragma once
#include <limits>
#include <string_view>
#include <tuple>
#include "winrt/Microsoft.ReactNative.h"

//
// The macros below are internal implementation details for macro defined in NativeModules.h
//

// Register struct as a ReactNative module.
#define INTERNAL_REACT_MODULE(moduleStruct, ...)                                                            \
  struct moduleStruct;                                                                                      \
                                                                                                            \
  /* Module registration is a template to allow static singleton definition in the header file. */          \
  template <class TDummy>                                                                                   \
  struct moduleStruct##_Registration final : winrt::Microsoft::ReactNative::ReactModuleRegistration {       \
    moduleStruct##_Registration() noexcept                                                                  \
        : winrt::Microsoft::ReactNative::ReactModuleRegistration(GetModuleInfo()) {}                        \
                                                                                                            \
    /* Read optional named attributes. */                                                                   \
    static winrt::Microsoft::ReactNative::ReactModuleInfo GetModuleInfo() noexcept {                        \
      using namespace winrt::Microsoft::ReactNative;                                                        \
      /* Aliases for the most common dispatcher property names. */                                          \
      [[maybe_unused]] auto UIDispatcher = ReactDispatcherHelper::UIDispatcherProperty();                   \
      [[maybe_unused]] auto JSDispatcher = ReactDispatcherHelper::JSDispatcherProperty();                   \
      /* Named arguments with default values. */                                                            \
      ReactNamedArg<std::wstring_view> moduleName{L## #moduleStruct};                                       \
      ReactNamedArg<std::wstring_view> eventEmitterName{L""};                                               \
      ReactNamedArg<IReactPropertyName> dispatcherName{JSDispatcher};                                       \
      /* The expanded variadic macro arguments will appear in the argTuple             */                   \
      /* either at the predefined position or wrapped into the ReactNamedArg template. */                   \
      auto argTuple = std::tuple{__VA_ARGS__};                                                              \
      return ReactModuleInfo{                                                                               \
          moduleName.Get<0>(argTuple), eventEmitterName.Get<1>(argTuple), dispatcherName.Get<2>(argTuple)}; \
    }                                                                                                       \
                                                                                                            \
    winrt::Microsoft::ReactNative::ReactModuleProvider MakeModuleProvider() const noexcept override {       \
      return winrt::Microsoft::ReactNative::MakeModuleProvider<moduleStruct>();                             \
    }                                                                                                       \
                                                                                                            \
    static const moduleStruct##_Registration Registration;                                                  \
  };                                                                                                        \
                                                                                                            \
  template <class TDummy>                                                                                   \
  const moduleStruct##_Registration<TDummy> moduleStruct##_Registration<TDummy>::Registration;              \
                                                                                                            \
  /* Instantiate the module registration type. */                                                           \
  template struct moduleStruct##_Registration<int>;                                                         \
                                                                                                            \
  inline winrt::Microsoft::ReactNative::ReactModuleInfo const &GetReactModuleInfo(                          \
      moduleStruct * /*valueAsTypeTag*/) noexcept {                                                         \
    return moduleStruct##_Registration<int>::Registration.ModuleInfo();                                     \
  }                                                                                                         \
                                                                                                            \
  template <class TVisitor>                                                                                 \
  constexpr void VisitReactModuleMembers(moduleStruct * /*valueAsTypeTag*/, TVisitor &visitor) noexcept {   \
    visitor.VisitModuleMembers(winrt::Microsoft::ReactNative::ReactAttributeId<__COUNTER__>{});             \
  }

// Provide meta data information about a struct member.
// For each member with a 'custom attribute' macro we create a static method to provide meta data.
// The member Id is generated as a ReactMemberId<__COUNTER__> type.
// To enumerate the static methods, we can increment ReactMemberId while static member exists.
//
// This macro supports use of named parameters such as FOO(param1 = value).
// It is done by expanding the macro variadic args into the tuple constructor.
// The memberName, moduleName, useJSDispatcher are the names of the named arguments,
// and the index0, index1, index3 are the indexes of the positional arguments.
// Note that the positional arguments have a higher priority over the named arguments.
#define INTERNAL_REACT_MEMBER(                                                                                \
    memberKind, member, memberName, moduleName, useJSDispatcher, index0, index1, index2, ...)                 \
  template <class TStruct, class TVisitor>                                                                    \
  constexpr static void GetReactMemberAttribute(                                                              \
      TVisitor &visitor, winrt::Microsoft::ReactNative::ReactAttributeId<__COUNTER__> attributeId) noexcept { \
    using namespace winrt::Microsoft::ReactNative;                                                            \
    const auto invalid = std::numeric_limits<size_t>::max(); /* used as an invalid index */                   \
    /* Named arguments with default values. */                                                                \
    ReactNamedArg<std::wstring_view> memberName{L## #member};                                                 \
    ReactNamedArg<std::wstring_view> moduleName{L""};                                                         \
    ReactNamedArg<bool> useJSDispatcher{false};                                                               \
    /* The expanded variadic macro arguments will appear in the argTuple             */                       \
    /* either at the predefined position or wrapped into the ReactNamedArg template. */                       \
    auto argTuple = std::make_tuple(__VA_ARGS__);                                                             \
    visitor.Visit(                                                                                            \
        &TStruct::member,                                                                                     \
        attributeId,                                                                                          \
        winrt::Microsoft::ReactNative::React##memberKind##Attribute{                                          \
            memberName.Get<index0>(argTuple),                                                                 \
            moduleName.Get<index1>(argTuple),                                                                 \
            useJSDispatcher.Get<index2>(argTuple)});                                                          \
  }

namespace winrt::Microsoft::ReactNative {

// Supports optional named arguments for macro-attributes.
template <typename T>
struct ReactNamedArg {
  constexpr ReactNamedArg(T value) : m_value(std::move(value)) {}

  constexpr std::nullptr_t operator=(T value) noexcept {
    m_value = std::move(value);
    return nullptr;
  }

  template <size_t Index, typename TArgTuple>
  static constexpr bool IsPositionalArg() {
    if constexpr (Index >= std::tuple_size_v<TArgTuple>) {
      return false;
    } else if constexpr (std::is_same_v<nullptr_t, std::tuple_element_t<Index, TArgTuple>>) {
      return false;
    } else if constexpr (Index > 0) {
      static_assert(IsPositionalArg<Index - 1, TArgTuple>(), "Positional arg must not follow a named arg.");
    }
    return true;
  }

  template <size_t Index, typename TArgTuple>
  constexpr T Get(TArgTuple const &argTuple) {
    if constexpr (IsPositionalArg<Index, TArgTuple>()) {
      return std::get<Index>(argTuple);
    } else {
      return m_value;
    }
  }

 private:
  T m_value;
};

// The module information required for its registration.
struct ReactModuleInfo {
  std::wstring_view ModuleName;
  std::wstring_view EventEmitterName;
  IReactPropertyName DispatcherName;

  ReactModuleInfo(
      std::wstring_view moduleName,
      std::wstring_view eventEmitterName = L"",
      IReactPropertyName const &dispatcherName = nullptr) noexcept
      : ModuleName{moduleName}, EventEmitterName{eventEmitterName}, DispatcherName{dispatcherName} {
    if (EventEmitterName.empty()) {
      EventEmitterName = L"RCTDeviceEventEmitter";
    }
  }
};

// The base abstract class for module registration.
struct ReactModuleRegistration {
  ReactModuleRegistration(ReactModuleInfo &&moduleInfo) noexcept;

  virtual ReactModuleProvider MakeModuleProvider() const noexcept = 0;

  static ReactModuleRegistration const *Head() noexcept {
    return s_head;
  }

  ReactModuleRegistration const *Next() const noexcept {
    return m_next;
  }

  ReactModuleInfo const &ModuleInfo() const noexcept {
    return m_moduleInfo;
  }

 private:
  ReactModuleInfo const m_moduleInfo;
  ReactModuleRegistration const *m_next{nullptr};

  static const ReactModuleRegistration *s_head;
};

// Adds all registered modules to the package builder.
void AddAttributedModules(ReactPackageBuilder const &packageBuilder) noexcept;
void AddAttributedModules(IReactPackageBuilder const &packageBuilder) noexcept;

// Tries to add a registered module with the moduleName. It returns true if succeeded, or false otherwise.
bool TryAddAttributedModule(ReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept;
bool TryAddAttributedModule(IReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept;

} // namespace winrt::Microsoft::ReactNative

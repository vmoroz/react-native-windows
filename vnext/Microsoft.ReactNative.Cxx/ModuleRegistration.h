// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
// IMPORTANT: Before updating this file
// please read react-native-windows repo:
// vnext/Microsoft.ReactNative.Cxx/README.md

#pragma once
#include <string>
#include <tuple>
#include "ReactPropertyBag.h"
#include "winrt/Microsoft.ReactNative.h"

// We implement optional parameter macros based on the StackOverflow discussion:
// https://stackoverflow.com/questions/3046889/optional-parameters-with-c-macros
// Please refer to it if you want to understand it works.
#define INTERNAL_REACT_GET_ARG_4(arg1, arg2, arg3, arg4, ...) arg4
#define INTERNAL_REACT_GET_ARG_5(arg1, arg2, arg3, arg4, arg5, ...) arg5
#define INTERNAL_REACT_RECOMPOSER_4(argsWithParentheses) INTERNAL_REACT_GET_ARG_4 argsWithParentheses
#define INTERNAL_REACT_RECOMPOSER_5(argsWithParentheses) INTERNAL_REACT_GET_ARG_5 argsWithParentheses

//
// The macros below are internal implementation details for macro defined in nativeModules.h
//

// Register struct as a ReactNative module.
#define INTERNAL_REACT_MODULE_N_ARGS(moduleStruct, ...)                                                    \
  struct moduleStruct;                                                                                     \
                                                                                                           \
  /* Module registration is a template to allow static singleton definition in th header file. */          \
  template <class TDummy>                                                                                  \
  struct moduleStruct##_ModuleRegistration final : winrt::Microsoft::ReactNative::ModuleRegistration {     \
    moduleStruct##_ModuleRegistration() noexcept                                                           \
        : winrt::Microsoft::ReactNative::ModuleRegistration(GetModuleInfo()) {}                            \
                                                                                                           \
    /* This method takes care about the optional named attributes. */                                      \
    static winrt::Microsoft::ReactNative::ReactModuleInfo GetModuleInfo() noexcept {                       \
      /* Aliases for the most common dispatcher property names. */                                         \
      [[maybe_unused]] auto UIDispatcher = winrt::Microsoft::ReactNative::ReactPropertyName(               \
          winrt::Microsoft::ReactNative::ReactDispatcherHelper::UIDispatcherProperty());                   \
      [[maybe_unused]] auto JSDispatcher = winrt::Microsoft::ReactNative::ReactPropertyName(               \
          winrt::Microsoft::ReactNative::ReactDispatcherHelper::JSDispatcherProperty());                   \
      /* ReactModuleInfo default values. */                                                                \
      using winrt::Microsoft::ReactNative::ReactModuleInfo;                                                \
      auto info = ReactModuleInfo(L## #moduleStruct, L"", JSDispatcher);                                   \
      /* Use lambda arguments to get named macro arguments. */                                             \
      [&]([[maybe_unused]] winrt::Microsoft::ReactNative::ReactNamedArg<std::wstring> moduleName,          \
          [[maybe_unused]] winrt::Microsoft::ReactNative::ReactNamedArg<std::wstring> eventEmitterName,    \
          [[maybe_unused]] winrt::Microsoft::ReactNative::ReactNamedArg<                                   \
              winrt::Microsoft::ReactNative::ReactPropertyName> dispatcherName) {                          \
        auto argTuple = std::make_tuple(__VA_ARGS__);                                                      \
        ReactModuleInfo::GetPositionalArg<0>(argTuple, /*ref*/ info.ModuleName);                           \
        ReactModuleInfo::GetPositionalArg<1>(argTuple, /*ref*/ info.EventEmitterName);                     \
        ReactModuleInfo::GetPositionalArg<2>(argTuple, /*ref*/ info.DispatcherName);                       \
      }(info.ModuleName, info.EventEmitterName, info.DispatcherName);                                      \
      return info;                                                                                         \
    }                                                                                                      \
                                                                                                           \
    winrt::Microsoft::ReactNative::ReactModuleProvider MakeModuleProvider() const noexcept override {      \
      return winrt::Microsoft::ReactNative::MakeModuleProvider<moduleStruct>(this->ModuleInfo());          \
    }                                                                                                      \
                                                                                                           \
    static const moduleStruct##_ModuleRegistration Registration;                                           \
  };                                                                                                       \
                                                                                                           \
  template <class TDummy>                                                                                  \
  const moduleStruct##_ModuleRegistration<TDummy> moduleStruct##_ModuleRegistration<TDummy>::Registration; \
                                                                                                           \
  /* Instantiate the module registration type. */                                                          \
  template struct moduleStruct##_ModuleRegistration<int>;                                                  \
                                                                                                           \
  template <class TRegistry>                                                                               \
  constexpr void RegisterReactModule(moduleStruct *, TRegistry &registry) noexcept {                       \
    registry.RegisterModule(winrt::Microsoft::ReactNative::ReactAttributeId<__COUNTER__>{});               \
  }

#define INTERNAL_REACT_MODULE_1_ARG(moduleStruct) INTERNAL_REACT_MODULE_N_ARGS(moduleStruct, )

#define INTERNAL_REACT_MODULE(...)   \
  INTERNAL_REACT_RECOMPOSER_5(       \
      (__VA_ARGS__,                  \
       INTERNAL_REACT_MODULE_N_ARGS, \
       INTERNAL_REACT_MODULE_N_ARGS, \
       INTERNAL_REACT_MODULE_N_ARGS, \
       INTERNAL_REACT_MODULE_1_ARG, ))

// Provide meta data information about struct member.
// For each member with a 'custom attribute' macro we create a static method to provide meta data.
// The member Id is generated as a ReactMemberId<__COUNTER__> type.
// To enumerate the static methods, we can increment ReactMemberId while static member exists.
#define INTERNAL_REACT_MEMBER_4_ARGS(memberKind, member, jsMemberName, jsModuleName)                          \
  template <class TStruct, class TVisitor>                                                                    \
  constexpr static void GetReactMemberAttribute(                                                              \
      TVisitor &visitor, winrt::Microsoft::ReactNative::ReactAttributeId<__COUNTER__> attributeId) noexcept { \
    visitor.Visit(                                                                                            \
        &TStruct::member,                                                                                     \
        attributeId,                                                                                          \
        winrt::Microsoft::ReactNative::React##memberKind##Attribute{jsMemberName, jsModuleName});             \
  }

#define INTERNAL_REACT_MEMBER_3_ARGS(memberKind, member, jsMemberName) \
  INTERNAL_REACT_MEMBER_4_ARGS(memberKind, member, jsMemberName, L"")

#define INTERNAL_REACT_MEMBER_2_ARGS(memberKind, member) INTERNAL_REACT_MEMBER_3_ARGS(memberKind, member, L## #member)

#define INTERNAL_REACT_MEMBER(...) \
  INTERNAL_REACT_RECOMPOSER_4(     \
      (__VA_ARGS__, INTERNAL_REACT_MEMBER_4_ARGS, INTERNAL_REACT_MEMBER_3_ARGS, INTERNAL_REACT_MEMBER_2_ARGS, ))

namespace winrt::Microsoft::ReactNative {

template <typename T>
struct ReactNamedArg {
  ReactNamedArg(T &value) : m_value(value) {}

  ReactNamedArg<T> &operator=(T value) noexcept {
    m_value = std::move(value);
    return *this;
  }

 private:
  T &m_value;
};

template <typename T>
struct IsReactNamedArg : std::false_type {};

template <typename T>
struct IsReactNamedArg<ReactNamedArg<T>> : std::true_type {};

struct ReactModuleInfo {
  std::wstring ModuleName;
  std::wstring EventEmitterName;
  ReactPropertyName DispatcherName;

  ReactModuleInfo(
      std::wstring &&moduleName,
      std::wstring &&eventEmitterName = L"",
      ReactPropertyName const &dispatcherName = nullptr) noexcept
      : ModuleName(std::move(moduleName)),
        EventEmitterName(std::move(eventEmitterName)),
        DispatcherName(dispatcherName) {
    if (EventEmitterName.empty()) {
      EventEmitterName = L"RCTDeviceEventEmitter";
    }
  }

  template <size_t Index, typename TArgTuple>
  static constexpr bool IsPositionalArg() {
    if constexpr (Index >= std::tuple_size_v<TArgTuple>) {
      return false;
    } else if constexpr (IsReactNamedArg<std::tuple_element_t<Index, TArgTuple>>::value) {
      return false;
    } else if constexpr (Index > 0) {
      static_assert(IsPositionalArg<Index - 1, TArgTuple>(), "Positional arg must not follow a named arg.");
    }
    return true;
  }

  template <size_t Index, typename TArgTuple, typename TValue>
  static void GetPositionalArg(TArgTuple &argTuple, TValue &result) {
    if constexpr (IsPositionalArg<Index, TArgTuple>()) {
      result = std::get<Index>(std::move(argTuple));
    }
  }
};

struct ModuleRegistration {
  ModuleRegistration(ReactModuleInfo &&moduleInfo) noexcept;

  virtual ReactModuleProvider MakeModuleProvider() const noexcept = 0;

  static ModuleRegistration const *Head() noexcept {
    return s_head;
  }

  ModuleRegistration const *Next() const noexcept {
    return m_next;
  }

  ReactModuleInfo const &ModuleInfo() const noexcept {
    return m_moduleInfo;
  }

 private:
  ReactModuleInfo m_moduleInfo;
  ModuleRegistration const *m_next{nullptr};

  static const ModuleRegistration *s_head;
};

void AddAttributedModules(ReactPackageBuilder const &packageBuilder) noexcept;
void AddAttributedModules(IReactPackageBuilder const &packageBuilder) noexcept;

bool TryAddAttributedModule(ReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept;
bool TryAddAttributedModule(IReactPackageBuilder const &packageBuilder, std::wstring_view moduleName) noexcept;

} // namespace winrt::Microsoft::ReactNative

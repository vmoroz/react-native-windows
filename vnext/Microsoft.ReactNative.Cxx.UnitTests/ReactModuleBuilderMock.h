// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#include <functional>
#include "JSValue.h"

namespace winrt::Microsoft::ReactNative::Bridge {

struct ReactModuleBuilderMockDelegate {
  bool IsResolveCallbackCalled() const noexcept {
    return m_isResolveCallbackCalled;
  }

  void IsResolveCallbackCalled(bool value) noexcept {
    m_isResolveCallbackCalled = value;
  }

  bool IsRejectCallbackCalled() const noexcept {
    return m_isRejectCallbackCalled;
  }

  void IsRejectCallbackCalled(bool value) noexcept {
    m_isRejectCallbackCalled = value;
  }

 private:
  bool m_isResolveCallbackCalled;
  bool m_isRejectCallbackCalled;
};

struct ReactModuleBuilderMock : implements<ReactModuleBuilderMock, IReactModuleBuilder> {
  ReactModuleBuilderMock(ReactModuleBuilderMockDelegate &delegate) noexcept;

 public: // IReactModuleBuilder
  void SetEventEmitterName(hstring const &name) noexcept;
  void AddMethod(hstring const &name, MethodReturnType returnType, MethodDelegate const &method) noexcept;
  void AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept;
  void AddConstantProvider(ConstantProvider const &constantProvider) noexcept;
  void AddEventHandlerSetter(hstring const &name, ReactEventHandlerSetter const &eventHandlerSetter) noexcept;

 public:
  template <class... TArgs>
  void Call0(std::wstring const &methodName, TArgs &&... args) noexcept;

  template <class TResult, class... TArgs>
  void Call1(std::wstring const &methodName, TArgs &&... args, std::function<void(TResult)> const &resolve) noexcept;

  template <class TResult, class TError, class... TArgs>
  void Call2(
      std::wstring const &methodName,
      TArgs &&... args,
      std::function<void(TResult)> const &resolve,
      std::function<void(TError)> const &reject) noexcept;

  template <class TResult, class... TArgs>
  void CallSync(std::wstring const &methodName, TArgs &&... args, TResult &result) noexcept;

  JSValueObject GetConstants() noexcept;

  template <class T>
  void SetEventHandler(std::wstring const &eventName, std::function<void(T)> const &eventHandler) noexcept;

 private:
  MethodDelegate GetMethod0(std::wstring const &methodName) const noexcept;
  MethodDelegate GetMethod1(std::wstring const &methodName) const noexcept;
  MethodDelegate GetMethod2(std::wstring const &methodName) const noexcept;
  SyncMethodDelegate GetSyncMethod(std::wstring const &methodName) const noexcept;

  static IJSValueWriter ArgWriter(JSValue &jsValue) noexcept;
  template <class... TArgs>
  static IJSValueReader ArgReader(TArgs &&... args) noexcept;
  static IJSValueReader CreateArgReader(
      std::function<IJSValueWriter(IJSValueWriter const &)> const &argWriter) noexcept;

  template <class T>
  MethodResultCallback ResolveCallback(JSValue const &jsValue, std::function<void(T)> const &resolve) noexcept;
  template <class T>
  MethodResultCallback RejectCallback(JSValue const &jsValue, std::function<void(T)> const &reject) noexcept;

 private:
  std::wstring m_eventEmitterName;
  std::map<std::wstring, std::tuple<MethodReturnType, MethodDelegate>> m_methods;
  std::map<std::wstring, SyncMethodDelegate> m_syncMethods;
  std::vector<ConstantProvider> m_constProviders;
  std::map<std::wstring, std::function<void(IJSValueReader &)>> m_eventHandlers;
  ReactModuleBuilderMockDelegate &m_delegate;
};

//===========================================================================
// ReactModuleBuilderMock inline implementation
//===========================================================================

inline ReactModuleBuilderMock::ReactModuleBuilderMock(ReactModuleBuilderMockDelegate &delegate) noexcept
    : m_delegate{delegate} {}

template <class... TArgs>
void ReactModuleBuilderMock::Call0(std::wstring const &methodName, TArgs &&... args) noexcept {
  if (auto method = GetMethod0(methodName)) {
    JSValue jsValue;
    method(ArgReader(std::forward<TArgs>(args)...), ArgWriter(jsValue), nullptr, nullptr);
  }
}

template <class TResult, class... TArgs>
void ReactModuleBuilderMock::Call1(
    std::wstring const &methodName,
    TArgs &&... args,
    std::function<void(TResult)> const &resolve) noexcept {
  if (auto method = GetMethod1(methodName)) {
    JSValue jsValue;
    method(ArgReader(std::forward<TArgs>(args)...), ArgWriter(jsValue), ResolveCallback(jsValue, resolve), nullptr);
  }
}

template <class TResult, class TError, class... TArgs>
void ReactModuleBuilderMock::Call2(
    std::wstring const &methodName,
    TArgs &&... args,
    std::function<void(TResult)> const &resolve,
    std::function<void(TError)> const &reject) noexcept {
  if (auto method = GetMethod2(methodName)) {
    JSValue jsValue;
    method(
        ArgReader(std::forward<TArgs>(args)...),
        ArgWriter(jsValue),
        ResolveCallback(jsValue, resolve),
        RejectCallback(jsValue, reject));
  }
}

template <class TResult, class... TArgs>
void ReactModuleBuilderMock::CallSync(std::wstring const &methodName, TArgs &&... args, TResult &result) noexcept {
  if (auto method = GetSyncMethod(methodName)) {
    JSValue jsValue;
    method(ArgReader(std::forward<TArgs>(args)...), ArgWriter(jsValue));
    result = jsValue.To<TResult>();
  }
}

template <class... TArgs>
inline /*static*/ IJSValueReader ReactModuleBuilderMock::ArgReader(TArgs &&... args) noexcept {
  return CreateArgReader(
      [](IJSValueWriter const &writer) noexcept { return WriteArgs(writer, std::forward<TArgs>(args)...); });
}

template <class T>
MethodResultCallback ReactModuleBuilderMock::ResolveCallback(
    JSValue const &jsValue,
    std::function<void(T)> const &resolve) noexcept {
  return [ this, &jsValue ](IJSValueWriter const & /*writer*/) noexcept {
    resolve(jsValue.To<T>());
    m_delegate.IsResolveCallbackCalled(true);
  };
}

template <class T>
MethodResultCallback ReactModuleBuilderMock::RejectCallback(
    JSValue const &jsValue,
    std::function<void(T)> const &reject) noexcept {
  return [ this, &jsValue ](IJSValueWriter const & /*writer*/) noexcept {
    reject(jsValue.To<T>());
    m_delegate.IsResolveCallbackCalled(true);
  };
}

} // namespace winrt::Microsoft::ReactNative::Bridge

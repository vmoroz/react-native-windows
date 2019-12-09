// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "NativeModules.h"
#include "ReactModuleBuilderMock.h"
#include "StructInfo.h"
#include "catch.hpp"

namespace winrt::Microsoft::ReactNative::Bridge {

template <class T>
struct ReactModule : implements<ReactModule<T>, IReactModule> {
  ReactModule() noexcept {}

  int64_t GetImpl() noexcept {
    return reinterpret_cast<int64_t>(&m_module);
  }

  static T &GetImpl(IReactModule &module) noexcept {
    return *reinterpret_cast<T *>(module.GetImpl());
  }

 private:
  T m_module{};
};

template <class TFunc>
struct ModuleMethodInfo;

template <class TModule, class TResult, class... TArgs>
struct ModuleMethodInfo<TResult (TModule::*)(TArgs...) noexcept> {
  using ModuleType = TModule;
  using MethodType = TResult (TModule::*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs)>;

  template <class>
  struct Invoker;

  template <size_t... I>
  struct Invoker<std::index_sequence<I...>> {
    // Async method with return value
    static MethodDelegate GetFunc(ModuleType *module, MethodType method) noexcept {
      return [ module, method ](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &callback,
          MethodResultCallback const &) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        TResult result = (module->*method)(std::get<I>(typedArgs)...);
        WriteArgs(argWriter, result);
        callback(argWriter);
      };
    }
  };

  static MethodDelegate GetMethodDelegate(void *module, MethodType method) noexcept {
    return Invoker<IndexSequence>::GetFunc(static_cast<ModuleType *>(module), method);
  }
};

struct ReactModuleBuilder {
  ReactModuleBuilder(void *module, IReactModuleBuilder const &moduleBuilder) noexcept
      : m_module{module}, m_moduleBuilder{moduleBuilder} {}

  template <class TModule, int I>
  void RegisterModule(
      const wchar_t * /*moduleName*/,
      const wchar_t *eventEmitterName,
      winrt::Microsoft::ReactNative::Bridge::ReactMemberId<I>) noexcept {
    m_moduleBuilder.SetEventEmitterName(eventEmitterName);
    RegisterMembers<TModule, I>();
  }

  template <class TClass, int I>
  auto HasRegisterMember(ReactModuleBuilder &builder, ReactMemberId<I> id)
      -> decltype(TClass::template RegisterMember<TClass>(builder, id), std::true_type{});
  template <class TClass>
  auto HasRegisterMember(...) -> std::false_type;

  template <class TModule, int I>
  void RegisterMembers() noexcept {
    if constexpr (decltype(HasRegisterMember<TModule>(*this, ReactMemberId<I + 1>{}))::value) {
      TModule::template RegisterMember<TModule>(*this, ReactMemberId<I + 1>{});
      RegisterMembers<TModule, I + 1>();
    }
  }

  template <class TClass, class TMethod>
  void RegisterMethod(TMethod method, wchar_t const *name) noexcept {
    auto methodDelegate = ModuleMethodInfo<TMethod>::GetMethodDelegate(m_module, method);
    m_moduleBuilder.AddMethod(name, MethodReturnType::Callback, methodDelegate);
  }

 private:
  void *m_module;
  IReactModuleBuilder m_moduleBuilder;
};

template <class TModule>
inline ReactModuleProvider MakeModuleProvider() noexcept {
  return [](IReactModuleBuilder const &moduleBuilder) noexcept {
    auto moduleObject = make<ReactModule<TModule>>();
    auto module = &ReactModule<TModule>::GetImpl(moduleObject);
    ReactModuleBuilder builder{module, moduleBuilder};
    RegisterModule(builder, module);
    return moduleObject;
  };
}

struct SimpleNativeModule;

template <class TRegistry>
void RegisterModule(TRegistry &registry, SimpleNativeModule *) noexcept {
  registry.RegisterModule<SimpleNativeModule>(
      L"SimpleNativeModule",
      L"SimpleNativeModule",
      winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>{});
}

struct SimpleNativeModule {
  template <class TClass, class TRegistry>
  static void RegisterMember(
      TRegistry &registry,
      winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept {
    registry.RegisterMethod<TClass>(&TClass::Add, L"Add");
  }

  int Add(int x, int y) noexcept {
    return x + y;
  }
};

struct NativeModuleTestFixture {
  NativeModuleTestFixture() {
    m_moduleBuilder = make<ReactModuleBuilderImpl>(m_builderMock);
    auto provider = MakeModuleProvider<SimpleNativeModule>();
    m_moduleObject = provider(m_moduleBuilder);
    auto reactModule = m_moduleObject.as<IReactModule>();
    m_module = &ReactModule<SimpleNativeModule>::GetImpl(reactModule);
  }

 protected:
  ReactModuleBuilderMock m_builderMock{};
  IReactModuleBuilder m_moduleBuilder;
  Windows::Foundation::IInspectable m_moduleObject{nullptr};
  SimpleNativeModule *m_module;
};

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_Add", "NativeModuleTest") {
  m_builderMock.Call1(L"Add", std::function<void(int)>([](int result) noexcept { REQUIRE(result == 8); }), 3, 5);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

} // namespace winrt::Microsoft::ReactNative::Bridge

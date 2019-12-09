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

namespace Internal {

// Checks if provided type has a callback-like signature TFunc<void(TArgs...)
template <class T>
struct IsCallback : std::false_type {};
template <template <class...> class TFunc, class... TArgs>
struct IsCallback<TFunc<void(TArgs...)>> : std::true_type {};
#if defined(__cpp_noexcept_function_type) || (_HAS_NOEXCEPT_FUNCTION_TYPES == 1)
template <template <class...> class TFunc, class... TArgs>
struct IsCallback<TFunc<void(TArgs...) noexcept>> : std::true_type {};
#endif

// Finds how many callback the function has
template <class TArgTuple>
struct GetCallbackCount;

template <>
struct GetCallbackCount<std::tuple<>> {
  constexpr static size_t Value = 0;
};

template <class TArg>
struct GetCallbackCount<std::tuple<TArg>> {
  constexpr static size_t Value = IsCallback<TArg>::value ? 1 : 0;
};

template <class TArg0, class TArg1, class... TArgs>
struct GetCallbackCount<std::tuple<TArg0, TArg1, TArgs...>> {
  using TupleType = std::tuple<TArg0, TArg1, TArgs...>;
  constexpr static size_t TupleSize = std::tuple_size_v<TupleType>;
  constexpr static size_t Value = (IsCallback<std::tuple_element_t<TupleSize - 2, TupleType>>::value ? 1 : 0) +
      (IsCallback<std::tuple_element_t<TupleSize - 1, TupleType>>::value ? 1 : 0);
};

template <class T>
struct ThreadLocalHolder {
  ThreadLocalHolder(T *value) noexcept : m_savedValue{tl_value} {
    tl_value = value;
  }

  ~ThreadLocalHolder() noexcept {
    tl_value = m_savedValue;
  }

  static T *Get() noexcept {
    return tl_value;
  }

 private:
  T *m_savedValue{nullptr};
  static thread_local T *tl_value;
};

template <class T>
/*static*/ thread_local T *ThreadLocalHolder<T>::tl_value{nullptr};

} // namespace Internal

template <class TFunc>
struct ModuleMethodInfo;

template <class TModule, class... TArgs>
struct ModuleMethodInfo<void (TModule::*)(TArgs...) noexcept> {
  constexpr static size_t CallbackCount =
      Internal::GetCallbackCount<std::tuple<std::remove_reference_t<TArgs>...>>::Value;
  using ModuleType = TModule;
  using MethodType = void (TModule::*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs) - CallbackCount>;

  template <class>
  struct Invoker;

  template <size_t... I>
  struct Invoker<std::index_sequence<I...>> {
    // Fire and forget method
    static MethodDelegate GetFunc(ModuleType *module, MethodType method, std::integral_constant<size_t, 0>) noexcept {
      return [ module, method ](
          IJSValueReader const &argReader,
          IJSValueWriter const & /*argWriter*/,
          MethodResultCallback const &,
          MethodResultCallback const &) mutable noexcept {
        std::tuple<std::remove_reference_t<TArgs>...> typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        (module->*method)(std::get<I>(std::move(typedArgs))...);
      };
    }

    // template <class T>
    // struct CallbackCreator;

    // template <template <class...> class TCallback, class... TArgs>
    // struct CallbackCreator<TCallback<void(TArgs...)>> {
    //  static TCallback<void(TArgs...)> Create(
    //      const IJSValueWriter &argWriter,
    //      const MethodResultCallback &callback) noexcept {
    //    return TCallback([ callback = std::move(callback), argWriter ](TArgs... args) noexcept {
    //      WriteArgs(argWriter, std::move(args)...);
    //      callback(argWriter);
    //    });
    //  }
    //};

    // template <class T, class = void>
    // struct RejectCallbackCreator;

    // template <template <class...> class TCallback, class TArg>
    // struct RejectCallbackCreator<
    //    TCallback<void(TArg)>,
    //    std::enable_if_t<std::is_assignable_v<std::string, TArg> || std::is_assignable_v<std::wstring, TArg>>> {
    //  static TCallback<void(TArg)> Create(
    //      const IJSValueWriter &argWriter,
    //      const MethodResultCallback &callback) noexcept {
    //    return TCallback([ callback = std::move(callback), argWriter ](TArg arg) noexcept {
    //      argWriter.WriteArrayBegin();
    //      argWriter.WriteObjectBegin();
    //      argWriter.WritePropertyName(L"message");
    //      WriteValue(argWriter, arg);
    //      argWriter.WriteObjectEnd();
    //      argWriter.WriteArrayEnd();
    //      callback(argWriter);
    //    });
    //  }
    //};

    // template <template <class...> class TCallback>
    // struct RejectCallbackCreator<TCallback<void()>, void> : CallbackCreator<TCallback<void()>> {};

    // template <template <class...> class TCallback, class TArg>
    // struct RejectCallbackCreator<
    //    TCallback<void(TArg)>,
    //    std::enable_if_t<!std::is_assignable_v<std::string, TArg> && !std::is_assignable_v<std::wstring, TArg>>>
    //    : CallbackCreator<TCallback<void(TArg)>> {};

    // template <template <class...> class TCallback, class TArg0, class TArg1, class... TArgs>
    // struct RejectCallbackCreator<TCallback<void(TArg0, TArg1, TArgs...)>, void>
    //    : CallbackCreator<TCallback<void(TArg0, TArg1, TArgs...)>> {};

    //// Method with one callback
    // static MethodDelegate GetFunc(ModuleType *module, MethodType method, std::integral_constant<size_t, 1>) noexcept
    // {
    //  return [ module, method ](
    //      IJSValueReader const &argReader,
    //      IJSValueWriter const &argWriter,
    //      MethodResultCallback const &callback,
    //      MethodResultCallback const &) mutable noexcept {
    //    using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
    //    ArgTuple typedArgs{};
    //    ReadArgs(argReader, std::get<I>(typedArgs)...);
    //    (module->*method)(
    //        std::get<I>(std::move(typedArgs))...,
    //        CallbackCreator<std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>::Create(
    //            argWriter, std::move(callback)));
    //  };
    //}

    //// Method with two callbacks
    // static MethodDelegate GetFunc(ModuleType *module, MethodType method, std::integral_constant<size_t, 2>) noexcept
    // {
    //  return [ module, method ](
    //      IJSValueReader const &argReader,
    //      IJSValueWriter const &argWriter,
    //      MethodResultCallback const &callback1,
    //      MethodResultCallback const &callback2) mutable noexcept {
    //    using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
    //    ArgTuple typedArgs{};
    //    ReadArgs(argReader, std::get<I>(typedArgs)...);
    //    (module->*method)(
    //        std::get<I>(std::move(typedArgs))...,
    //        CallbackCreator<std::tuple_element_t<sizeof...(TArgs) - 2, ArgTuple>>::Create(
    //            argWriter, std::move(callback1)),
    //        RejectCallbackCreator<std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>::Create(
    //            argWriter, std::move(callback2)));
    //  };
    //}
  };

  static MethodDelegate GetMethodDelegate(void *module, MethodType method, MethodReturnType &returnType) noexcept {
    returnType = MethodReturnType::Void;
    // if constexpr (CallbackCount == 1) {
    //  returnType = MethodReturnType::Callback;
    //} else if constexpr (CallbackCount == 2) {
    //  if (isAsync) {
    //    returnType = MethodReturnType::TwoCallbacks;
    //  } else {
    //    returnType = MethodReturnType::Promise;
    //  }
    //}
    return Invoker<IndexSequence>::GetFunc(
        static_cast<ModuleType *>(module), method, std::integral_constant<size_t, CallbackCount>{});
  }
};

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
        TResult result = (module->*method)(std::get<I>(std::move(typedArgs))...);
        WriteArgs(argWriter, result);
        callback(argWriter);
      };
    }
  };

  static MethodDelegate GetMethodDelegate(void *module, MethodType method, MethodReturnType &returnType) noexcept {
    returnType = MethodReturnType::Callback;
    return Invoker<IndexSequence>::GetFunc(static_cast<ModuleType *>(module), method);
  }
};

template <class TResult, class... TArgs>
struct ModuleMethodInfo<TResult (*)(TArgs...) noexcept> {
  using MethodType = TResult (*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs)>;

  template <class>
  struct Invoker;

  template <size_t... I>
  struct Invoker<std::index_sequence<I...>> {
    // Async method with return value
    static MethodDelegate GetFunc(MethodType method) noexcept {
      return [method](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &callback,
          MethodResultCallback const &) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        TResult result = (*method)(std::get<I>(typedArgs)...);
        WriteArgs(argWriter, result);
        callback(argWriter);
      };
    }
  };

  static MethodDelegate GetMethodDelegate(void * /*module*/, MethodType method, MethodReturnType &returnType) noexcept {
    returnType = MethodReturnType::Callback;
    return Invoker<IndexSequence>::GetFunc(method);
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
    MethodReturnType returnType;
    auto methodDelegate = ModuleMethodInfo<TMethod>::GetMethodDelegate(m_module, method, /*out*/ returnType);
    m_moduleBuilder.AddMethod(name, returnType, methodDelegate);
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

#define REACT_METHOD(method)                                                                             \
  template <class TClass, class TRegistry>                                                               \
  static void RegisterMember(                                                                            \
      TRegistry &registry, winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept { \
    registry.RegisterMethod<TClass>(&TClass::method, L## #method);                                       \
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
  REACT_METHOD(Add)
  int Add(int x, int y) noexcept {
    return x + y;
  }

  REACT_METHOD(Negate)
  int Negate(int x) noexcept {
    return -x;
  }

  REACT_METHOD(SayHello)
  std::string SayHello() noexcept {
    return "Hello";
  }

  REACT_METHOD(StaticAdd)
  static int StaticAdd(int x, int y) noexcept {
    return x + y;
  }

  REACT_METHOD(StaticNegate)
  static int StaticNegate(int x) noexcept {
    return -x;
  }

  REACT_METHOD(StaticSayHello)
  static std::string StaticSayHello() noexcept {
    return "Hello";
  }

  REACT_METHOD(SayHello0)
  void SayHello0() noexcept {
    Message = "Hello_0";
  }

  std::string Message;
  static std::string StaticMessage;
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

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_Negate", "NativeModuleTest") {
  m_builderMock.Call1(L"Negate", std::function<void(int)>([](int result) noexcept { REQUIRE(result == -3); }), 3);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_SayHello", "NativeModuleTest") {
  m_builderMock.Call1(L"SayHello", std::function<void(const std::string &)>([](const std::string &result) noexcept {
                        REQUIRE(result == "Hello");
                      }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticAdd", "NativeModuleTest") {
  m_builderMock.Call1(
      L"StaticAdd", std::function<void(int)>([](int result) noexcept { REQUIRE(result == 25); }), 20, 5);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticNegate", "NativeModuleTest") {
  m_builderMock.Call1(L"StaticNegate", std::function<void(int)>([](int result) noexcept { REQUIRE(result == -7); }), 7);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticSayHello", "NativeModuleTest") {
  m_builderMock.Call1(
      L"StaticSayHello",
      std::function<void(const std::string &)>([](const std::string &result) noexcept { REQUIRE(result == "Hello"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_SayHello0", "NativeModuleTest") {
  m_builderMock.Call0(L"SayHello0");
  REQUIRE(m_module->Message == "Hello_0");
}

} // namespace winrt::Microsoft::ReactNative::Bridge

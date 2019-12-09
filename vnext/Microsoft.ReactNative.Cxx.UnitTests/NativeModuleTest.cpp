// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include <sstream>
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
  constexpr static size_t Value = IsCallback<std::remove_const_t<std::remove_reference_t<TArg>>>::value ? 1 : 0;
};

template <class TArg0, class TArg1, class... TArgs>
struct GetCallbackCount<std::tuple<TArg0, TArg1, TArgs...>> {
  using TupleType = std::tuple<TArg0, TArg1, TArgs...>;
  constexpr static size_t TupleSize = std::tuple_size_v<TupleType>;
  constexpr static size_t Value =
      (IsCallback<std::remove_const_t<std::remove_reference_t<std::tuple_element_t<TupleSize - 2, TupleType>>>>::value
           ? 1
           : 0) +
      (IsCallback<std::remove_const_t<std::remove_reference_t<std::tuple_element_t<TupleSize - 1, TupleType>>>>::value
           ? 1
           : 0);
};

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

    template <class T>
    struct CallbackCreator;

    template <template <class...> class TCallback, class... TArgs>
    struct CallbackCreator<TCallback<void(TArgs...)>> {
      static TCallback<void(TArgs...)> Create(
          const IJSValueWriter &argWriter,
          const MethodResultCallback &callback) noexcept {
        return TCallback([ callback = std::move(callback), argWriter ](TArgs... args) noexcept {
          WriteArgs(argWriter, std::move(args)...);
          callback(argWriter);
        });
      }
    };

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

    // Method with one callback
    static MethodDelegate GetFunc(ModuleType *module, MethodType method, std::integral_constant<size_t, 1>) noexcept {
      return [ module, method ](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &callback,
          MethodResultCallback const &) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        auto cb = CallbackCreator<std::remove_const_t<std::remove_reference_t<
            std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>>>::Create(argWriter, callback);
        (module->*method)(std::get<I>(std::move(typedArgs))..., std::move(cb));
      };
    }

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
    if constexpr (CallbackCount == 1) {
      returnType = MethodReturnType::Callback;
    }
    // else if constexpr (CallbackCount == 2) {
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

template <class... TArgs>
struct ModuleMethodInfo<void (*)(TArgs...) noexcept> {
  constexpr static size_t CallbackCount =
      Internal::GetCallbackCount<std::tuple<std::remove_reference_t<TArgs>...>>::Value;
  using MethodType = void (*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs) - CallbackCount>;

  template <class>
  struct Invoker;

  template <size_t... I>
  struct Invoker<std::index_sequence<I...>> {
    // Fire and forget method
    static MethodDelegate GetFunc(MethodType method, std::integral_constant<size_t, 0>) noexcept {
      return [method](
          IJSValueReader const &argReader,
          IJSValueWriter const & /*argWriter*/,
          MethodResultCallback const &,
          MethodResultCallback const &) mutable noexcept {
        std::tuple<std::remove_reference_t<TArgs>...> typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        (*method)(std::get<I>(std::move(typedArgs))...);
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

  static MethodDelegate GetMethodDelegate(void * /*module*/, MethodType method, MethodReturnType &returnType) noexcept {
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
    return Invoker<IndexSequence>::GetFunc(method, std::integral_constant<size_t, CallbackCount>{});
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

REACT_STRUCT(Point)
struct Point {
  REACT_FIELD(X)
  int X;

  REACT_FIELD(Y)
  int Y;
};

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

  REACT_METHOD(PrintPoint)
  void PrintPoint(Point pt) noexcept {
    std::stringstream ss;
    ss << "Point: (" << pt.X << ", " << pt.Y << ")";
    Message = ss.str();
  }

  REACT_METHOD(PrintLine)
  void PrintLine(Point start, Point end) noexcept {
    std::stringstream ss;
    ss << "Line: (" << start.X << ", " << start.Y << ")-(" << end.X << ", " << end.Y << ")";
    Message = ss.str();
  }

  REACT_METHOD(StaticSayHello1)
  static void StaticSayHello1() noexcept {
    StaticMessage = "Hello_1";
  }

  REACT_METHOD(StaticPrintPoint)
  static void StaticPrintPoint(Point pt) noexcept {
    std::stringstream ss;
    ss << "Static Point: (" << pt.X << ", " << pt.Y << ")";
    StaticMessage = ss.str();
  }

  REACT_METHOD(StaticPrintLine)
  static void StaticPrintLine(Point start, Point end) noexcept {
    std::stringstream ss;
    ss << "Static Line: (" << start.X << ", " << start.Y << ")-(" << end.X << ", " << end.Y << ")";
    StaticMessage = ss.str();
  }

  REACT_METHOD(AddCallback)
  void AddCallback(int x, int y, std::function<void(int)> const &resolve) noexcept {
    resolve(x + y);
  }

  REACT_METHOD(NegateCallback)
  void NegateCallback(int x, std::function<void(int)> const &resolve) noexcept {
    resolve(-x);
  }

  REACT_METHOD(SayHelloCallback)
  void SayHelloCallback(std::function<void(const std::string &)> const &resolve) noexcept {
    resolve("Hello_2");
  }

  std::string Message;
  static std::string StaticMessage;
};

/*static*/ std::string SimpleNativeModule::StaticMessage;

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

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_PrintPoint", "NativeModuleTest") {
  m_builderMock.Call0(L"PrintPoint", Point{/*X =*/3, /*Y =*/5});
  REQUIRE(m_module->Message == "Point: (3, 5)");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_PrintLine", "NativeModuleTest") {
  m_builderMock.Call0(L"PrintLine", Point{/*X =*/3, /*Y =*/5}, Point{/*X =*/6, /*Y =*/8});
  REQUIRE(m_module->Message == "Line: (3, 5)-(6, 8)");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticSayHello1", "NativeModuleTest") {
  m_builderMock.Call0(L"StaticSayHello1");
  REQUIRE(SimpleNativeModule::StaticMessage == "Hello_1");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticPrintPoint", "NativeModuleTest") {
  m_builderMock.Call0(L"StaticPrintPoint", Point{/*X =*/13, /*Y =*/15});
  REQUIRE(SimpleNativeModule::StaticMessage == "Static Point: (13, 15)");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticPrintLine", "NativeModuleTest") {
  m_builderMock.Call0(L"StaticPrintLine", Point{/*X =*/13, /*Y =*/15}, Point{/*X =*/16, /*Y =*/18});
  REQUIRE(SimpleNativeModule::StaticMessage == "Static Line: (13, 15)-(16, 18)");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_AddCallback", "NativeModuleTest") {
  m_builderMock.Call1(
      L"AddCallback", std::function<void(int)>([](int result) noexcept { REQUIRE(result == -1); }), 7, -8);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_NegateCallback", "NativeModuleTest") {
  m_builderMock.Call1(
      L"NegateCallback", std::function<void(int)>([](int result) noexcept { REQUIRE(result == -4); }), 4);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_SayHelloCallback", "NativeModuleTest") {
  m_builderMock.Call1(L"NegateCallback", std::function<void(const std::string &)>([
                      ](const std::string &result) noexcept { REQUIRE(result == "Hello_2"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

} // namespace winrt::Microsoft::ReactNative::Bridge

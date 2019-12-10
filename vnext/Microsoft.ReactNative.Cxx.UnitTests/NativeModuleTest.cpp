// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include <sstream>
#include "NativeModules.h"
#include "ReactModuleBuilderMock.h"
#include "ReactPromise.h"
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

template <class T>
struct IsPromise : std::false_type {};
template <class T>
struct IsPromise<ReactPromise<T>> : std::true_type {};

} // namespace Internal

template <class TFunc>
struct ModuleMethodInfo;

template <class TModule, class... TArgs>
struct ModuleMethodInfo<void (TModule::*)(TArgs...) noexcept> {
  constexpr static bool HasPromise() noexcept {
    if constexpr (sizeof...(TArgs) > 0) {
      return Internal::IsPromise<std::remove_const_t<std::remove_reference_t<
          std::tuple_element_t<sizeof...(TArgs) - 1, std::tuple<std::remove_reference_t<TArgs>...>>>>>::value;
    }
    return false;
  }

  constexpr static size_t CallbackCount =
      Internal::GetCallbackCount<std::tuple<std::remove_reference_t<TArgs>...>>::Value;
  using ModuleType = TModule;
  using MethodType = void (TModule::*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs) - (HasPromise() ? 1 : CallbackCount)>;

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

    template <class T, class TDummy>
    struct RejectCallbackCreator;

    template <template <class...> class TCallback, class TArg>
    struct RejectCallbackCreator<
        TCallback<void(TArg)>,
        std::enable_if_t<std::is_assignable_v<std::string, TArg> || std::is_assignable_v<std::wstring, TArg>>> {
      static TCallback<void(TArg)> Create(
          const IJSValueWriter &argWriter,
          const MethodResultCallback &callback) noexcept {
        return TCallback([ callback = std::move(callback), argWriter ](TArg arg) noexcept {
          argWriter.WriteArrayBegin();
          argWriter.WriteObjectBegin();
          argWriter.WritePropertyName(L"message");
          WriteValue(argWriter, arg);
          argWriter.WriteObjectEnd();
          argWriter.WriteArrayEnd();
          callback(argWriter);
        });
      }
    };

    template <template <class...> class TCallback>
    struct RejectCallbackCreator<TCallback<void()>, void> : CallbackCreator<TCallback<void()>> {};

    template <template <class...> class TCallback, class TArg>
    struct RejectCallbackCreator<
        TCallback<void(TArg)>,
        std::enable_if_t<!std::is_assignable_v<std::string, TArg> && !std::is_assignable_v<std::wstring, TArg>>>
        : CallbackCreator<TCallback<void(TArg)>> {};

    template <template <class...> class TCallback, class TArg0, class TArg1, class... TArgs>
    struct RejectCallbackCreator<TCallback<void(TArg0, TArg1, TArgs...)>, void>
        : CallbackCreator<TCallback<void(TArg0, TArg1, TArgs...)>> {};

    template <class T>
    struct PromiseCreator;

    template <class T>
    struct PromiseCreator<ReactPromise<T>> {
      static ReactPromise<T> Create(
          const IJSValueWriter &argWriter,
          const MethodResultCallback &resolve,
          const MethodResultCallback &reject) noexcept {
        return ReactPromise<T>(argWriter, resolve, reject);
      }
    };

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

    // Method with two callbacks
    static MethodDelegate GetFunc(ModuleType *module, MethodType method, std::integral_constant<size_t, 2>) noexcept {
      return [ module, method ](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &resolve,
          MethodResultCallback const &reject) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        auto resolveCallback = CallbackCreator<std::remove_const_t<
            std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 2, ArgTuple>>>>::Create(argWriter, resolve);
        auto rejectCallback = RejectCallbackCreator<
            std::remove_const_t<std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>>,
            void>::Create(argWriter, reject);
        (module->*method)(std::get<I>(std::move(typedArgs))..., std::move(resolveCallback), std::move(rejectCallback));
      };
    }

    // Method with Promise
    static MethodDelegate GetFunc(ModuleType *module, MethodType method, std::integral_constant<size_t, 3>) noexcept {
      return [ module, method ](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &resolve,
          MethodResultCallback const &reject) mutable noexcept {
        using AllArgsTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        using ArgsTuple = std::tuple<std::tuple_element_t<I, AllArgsTuple>...>;
        using PromiseArg = std::remove_const_t<std::tuple_element_t<sizeof...(TArgs) - 1, AllArgsTuple>>;
        ArgsTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        auto promise = PromiseCreator<PromiseArg>::Create(argWriter, resolve, reject);
        (module->*method)(std::get<I>(std::move(typedArgs))..., std::move(promise));
      };
    }
  };

  static MethodDelegate GetMethodDelegate(void *module, MethodType method, MethodReturnType &returnType) noexcept {
    if constexpr (HasPromise()) {
      returnType = MethodReturnType::Promise;
    } else if constexpr (CallbackCount == 2) {
      returnType = MethodReturnType::TwoCallbacks;
    } else if constexpr (CallbackCount == 1) {
      returnType = MethodReturnType::Callback;
    } else {
      returnType = MethodReturnType::Void;
    }

    constexpr int selector = HasPromise() ? 3 : CallbackCount;
    return Invoker<IndexSequence>::GetFunc(
        static_cast<ModuleType *>(module), method, std::integral_constant<size_t, selector>{});
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
  constexpr static bool HasPromise() noexcept {
    if constexpr (sizeof...(TArgs) > 0) {
      return Internal::IsPromise<std::remove_const_t<std::remove_reference_t<
          std::tuple_element_t<sizeof...(TArgs) - 1, std::tuple<std::remove_reference_t<TArgs>...>>>>>::value;
    }
    return false;
  }

  constexpr static size_t CallbackCount =
      Internal::GetCallbackCount<std::tuple<std::remove_reference_t<TArgs>...>>::Value;
  using MethodType = void (*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs) - (HasPromise() ? 1 : CallbackCount)>;

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

    template <class T, class TDummy>
    struct RejectCallbackCreator;

    template <template <class...> class TCallback, class TArg>
    struct RejectCallbackCreator<
        TCallback<void(TArg)>,
        std::enable_if_t<std::is_assignable_v<std::string, TArg> || std::is_assignable_v<std::wstring, TArg>>> {
      static TCallback<void(TArg)> Create(
          const IJSValueWriter &argWriter,
          const MethodResultCallback &callback) noexcept {
        return TCallback([ callback = std::move(callback), argWriter ](TArg arg) noexcept {
          argWriter.WriteArrayBegin();
          argWriter.WriteObjectBegin();
          argWriter.WritePropertyName(L"message");
          WriteValue(argWriter, arg);
          argWriter.WriteObjectEnd();
          argWriter.WriteArrayEnd();
          callback(argWriter);
        });
      }
    };

    template <template <class...> class TCallback>
    struct RejectCallbackCreator<TCallback<void()>, void> : CallbackCreator<TCallback<void()>> {};

    template <template <class...> class TCallback, class TArg>
    struct RejectCallbackCreator<
        TCallback<void(TArg)>,
        std::enable_if_t<!std::is_assignable_v<std::string, TArg> && !std::is_assignable_v<std::wstring, TArg>>>
        : CallbackCreator<TCallback<void(TArg)>> {};

    template <template <class...> class TCallback, class TArg0, class TArg1, class... TArgs>
    struct RejectCallbackCreator<TCallback<void(TArg0, TArg1, TArgs...)>, void>
        : CallbackCreator<TCallback<void(TArg0, TArg1, TArgs...)>> {};

    template <class T>
    struct PromiseCreator;

    template <class T>
    struct PromiseCreator<ReactPromise<T>> {
      static ReactPromise<T> Create(
          const IJSValueWriter &argWriter,
          const MethodResultCallback &resolve,
          const MethodResultCallback &reject) noexcept {
        return ReactPromise<T>(argWriter, resolve, reject);
      }
    };

    // Method with one callback
    static MethodDelegate GetFunc(MethodType method, std::integral_constant<size_t, 1>) noexcept {
      return [method](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &callback,
          MethodResultCallback const &) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        auto cb = CallbackCreator<std::remove_const_t<std::remove_reference_t<
            std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>>>::Create(argWriter, callback);
        (*method)(std::get<I>(std::move(typedArgs))..., std::move(cb));
      };
    }

    // Method with two callbacks
    static MethodDelegate GetFunc(MethodType method, std::integral_constant<size_t, 2>) noexcept {
      return [method](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &resolve,
          MethodResultCallback const &reject) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        auto resolveCallback = CallbackCreator<std::remove_const_t<
            std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 2, ArgTuple>>>>::Create(argWriter, resolve);
        auto rejectCallback = RejectCallbackCreator<
            std::remove_const_t<std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>>,
            void>::Create(argWriter, reject);
        (*method)(std::get<I>(std::move(typedArgs))..., std::move(resolveCallback), std::move(rejectCallback));
      };
    }

    // Method with Promise
    static MethodDelegate GetFunc(MethodType method, std::integral_constant<size_t, 3>) noexcept {
      return [method](
          IJSValueReader const &argReader,
          IJSValueWriter const &argWriter,
          MethodResultCallback const &resolve,
          MethodResultCallback const &reject) mutable noexcept {
        using AllArgsTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        using ArgsTuple = std::tuple<std::tuple_element_t<I, AllArgsTuple>...>;
        using PromiseArg = std::remove_const_t<std::tuple_element_t<sizeof...(TArgs) - 1, AllArgsTuple>>;
        ArgsTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        auto promise = PromiseCreator<PromiseArg>::Create(argWriter, resolve, reject);
        (*method)(std::get<I>(std::move(typedArgs))..., std::move(promise));
      };
    }
  };

  static MethodDelegate GetMethodDelegate(void * /*module*/, MethodType method, MethodReturnType &returnType) noexcept {
    if constexpr (HasPromise()) {
      returnType = MethodReturnType::Promise;
    } else if constexpr (CallbackCount == 2) {
      returnType = MethodReturnType::TwoCallbacks;
    } else if constexpr (CallbackCount == 1) {
      returnType = MethodReturnType::Callback;
    } else {
      returnType = MethodReturnType::Void;
    }

    constexpr int selector = HasPromise() ? 3 : CallbackCount;
    return Invoker<IndexSequence>::GetFunc(method, std::integral_constant<size_t, selector>{});
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

template <class TFunc>
struct ModuleSyncMethodInfo;

template <class TModule, class TResult, class... TArgs>
struct ModuleSyncMethodInfo<TResult (TModule::*)(TArgs...) noexcept> {
  using ModuleType = TModule;
  using MethodType = TResult (TModule::*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs)>;

  template <class>
  struct Invoker;

  template <size_t... I>
  struct Invoker<std::index_sequence<I...>> {
    static SyncMethodDelegate GetFunc(ModuleType *module, MethodType method) noexcept {
      return [ module, method ](IJSValueReader const &argReader, IJSValueWriter const &argWriter) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        TResult result = (module->*method)(std::get<I>(std::move(typedArgs))...);
        WriteArgs(argWriter, result);
      };
    }
  };

  static SyncMethodDelegate GetMethodDelegate(void *module, MethodType method) noexcept {
    return Invoker<IndexSequence>::GetFunc(static_cast<ModuleType *>(module), method);
  }
};

template <class TResult, class... TArgs>
struct ModuleSyncMethodInfo<TResult (*)(TArgs...) noexcept> {
  using MethodType = TResult (*)(TArgs...) noexcept;
  using IndexSequence = std::make_index_sequence<sizeof...(TArgs)>;

  template <class>
  struct Invoker;

  template <size_t... I>
  struct Invoker<std::index_sequence<I...>> {
    static SyncMethodDelegate GetFunc(MethodType method) noexcept {
      return [method](IJSValueReader const &argReader, IJSValueWriter const &argWriter) mutable noexcept {
        using ArgTuple = std::tuple<std::remove_reference_t<TArgs>...>;
        ArgTuple typedArgs{};
        ReadArgs(argReader, std::get<I>(typedArgs)...);
        TResult result = (*method)(std::get<I>(std::move(typedArgs))...);
        WriteArgs(argWriter, result);
      };
    }
  };

  static SyncMethodDelegate GetMethodDelegate(void * /*module*/, MethodType method) noexcept {
    return Invoker<IndexSequence>::GetFunc(method);
  }
};

template <class TField>
struct ModuleConstFieldInfo;

template <class TModule, class TValue>
struct ModuleConstFieldInfo<TValue TModule::*> {
  using ModuleType = TModule;
  using FieldType = TValue TModule::*;

  static ConstantProvider GetConstantProvider(void *module, wchar_t const *name, FieldType field) noexcept {
    return [ module = static_cast<ModuleType *>(module), name = std::wstring{name}, field ](
        IJSValueWriter const &argWriter) mutable noexcept {
      WriteProperty(argWriter, name, module->*field);
    };
  }
};

template <class TValue>
struct ModuleConstFieldInfo<TValue *> {
  using FieldType = TValue *;

  static ConstantProvider GetConstantProvider(void * /*module*/, wchar_t const *name, FieldType field) noexcept {
    return [ name = std::wstring{name}, field ](IJSValueWriter const &argWriter) mutable noexcept {
      WriteProperty(argWriter, name, *field);
    };
  }
};

struct ReactConstantProvider {
  ReactConstantProvider(IJSValueWriter const &writer) noexcept : m_writer{writer} {}

  template <class T>
  void Add(const wchar_t *name, const T &value) noexcept {
    WriteProperty(m_writer, name, value);
  }

 private:
  IJSValueWriter m_writer;
};

template <class TMethod>
struct ModuleConstantInfo;

template <class TModule>
struct ModuleConstantInfo<void (TModule::*)(ReactConstantProvider &) noexcept> {
  using ModuleType = TModule;
  using MethodType = void (TModule::*)(ReactConstantProvider &) noexcept;

  static ConstantProvider GetConstantProvider(void *module, MethodType method) noexcept {
    return [ module = static_cast<ModuleType *>(module), method ](IJSValueWriter const &argWriter) mutable noexcept {
      ReactConstantProvider constantProvider{argWriter};
      (module->*method)(constantProvider);
    };
  }
};

template <>
struct ModuleConstantInfo<void (*)(ReactConstantProvider &) noexcept> {
  using MethodType = void (*)(ReactConstantProvider &) noexcept;

  static ConstantProvider GetConstantProvider(void * /*module*/, MethodType method) noexcept {
    return [method](IJSValueWriter const &argWriter) mutable noexcept {
      ReactConstantProvider constantProvider{argWriter};
      (*method)(constantProvider);
    };
  }
};

template <class TField>
struct ModuleEventFieldInfo;

template <class TModule, template <class> class TFunc, class TArg>
struct ModuleEventFieldInfo<TFunc<void(TArg)> TModule::*> {
  using ModuleType = TModule;
  using EventType = TFunc<void(TArg)>;
  using FieldType = EventType TModule::*;

  static ReactEventHandlerSetter GetEventHandlerSetter(void *module, FieldType field) noexcept {
    return [ module = static_cast<ModuleType *>(module), field ](const ReactEventHandler &eventHandler) noexcept {
      module->*field = [eventHandler](TArg arg) noexcept {
        eventHandler([&](const IJSValueWriter &argWriter) noexcept { WriteValue(argWriter, arg); });
      };
    };
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

  template <class TClass, class TMethod>
  void RegisterSyncMethod(TMethod method, wchar_t const *name) noexcept {
    auto syncMethodDelegate = ModuleSyncMethodInfo<TMethod>::GetMethodDelegate(m_module, method);
    m_moduleBuilder.AddSyncMethod(name, syncMethodDelegate);
  }

  template <class TClass, class TField>
  void RegisterConstant(TField field, wchar_t const *name) noexcept {
    auto constantProvider = ModuleConstFieldInfo<TField>::GetConstantProvider(m_module, name, field);
    m_moduleBuilder.AddConstantProvider(constantProvider);
  }

  template <class TClass, class TMethod>
  void RegisterConstantProvider(TMethod method) noexcept {
    auto constantProvider = ModuleConstantInfo<TMethod>::GetConstantProvider(m_module, method);
    m_moduleBuilder.AddConstantProvider(constantProvider);
  }

  template <class TClass, class TField>
  void RegisterEvent(TField field, wchar_t const *name) noexcept {
    auto eventHandlerSetter = ModuleEventFieldInfo<TField>::GetEventHandlerSetter(m_module, field);
    m_moduleBuilder.AddEventHandlerSetter(name, eventHandlerSetter);
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

#define REACT_SYNC_METHOD(method)                                                                        \
  template <class TClass, class TRegistry>                                                               \
  static void RegisterMember(                                                                            \
      TRegistry &registry, winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept { \
    registry.RegisterSyncMethod<TClass>(&TClass::method, L## #method);                                   \
  }

#define REACT_CONSTANT(field)                                                                            \
  template <class TClass, class TRegistry>                                                               \
  static void RegisterMember(                                                                            \
      TRegistry &registry, winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept { \
    registry.RegisterConstant<TClass>(&TClass::field, L## #field);                                       \
  }

#define REACT_CONSTANT2(field, name)                                                                     \
  template <class TClass, class TRegistry>                                                               \
  static void RegisterMember(                                                                            \
      TRegistry &registry, winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept { \
    registry.RegisterConstant<TClass>(&TClass::field, name);                                             \
  }

#define REACT_CONSTANT_PROVIDER(method)                                                                  \
  template <class TClass, class TRegistry>                                                               \
  static void RegisterMember(                                                                            \
      TRegistry &registry, winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept { \
    registry.RegisterConstantProvider<TClass>(&TClass::method);                                          \
  }

#define REACT_EVENT(field)                                                                               \
  template <class TClass, class TRegistry>                                                               \
  static void RegisterMember(                                                                            \
      TRegistry &registry, winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept { \
    registry.RegisterEvent<TClass>(&TClass::field, L## #field);                                          \
  }

#define REACT_EVENT2(field, name)                                                                        \
  template <class TClass, class TRegistry>                                                               \
  static void RegisterMember(                                                                            \
      TRegistry &registry, winrt::Microsoft::ReactNative::Bridge::ReactMemberId<__COUNTER__>) noexcept { \
    registry.RegisterEvent<TClass>(&TClass::field, name);                                                \
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

  REACT_METHOD(StaticAddCallback)
  static void StaticAddCallback(int x, int y, std::function<void(int)> const &resolve) noexcept {
    resolve(x + y);
  }

  REACT_METHOD(StaticNegateCallback)
  static void StaticNegateCallback(int x, std::function<void(int)> const &resolve) noexcept {
    resolve(-x);
  }

  REACT_METHOD(StaticSayHelloCallback)
  static void StaticSayHelloCallback(std::function<void(const std::string &)> const &resolve) noexcept {
    resolve("Static Hello_2");
  }

  REACT_METHOD(DivideCallbacks)
  void DivideCallbacks(
      int x,
      int y,
      std::function<void(int)> const &resolve,
      std::function<void(const std::string &)> const &reject) noexcept {
    if (y != 0) {
      resolve(x / y);
    } else {
      reject("Division by 0");
    }
  }

  REACT_METHOD(NegateCallbacks)
  void NegateCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(const std::string &)> const &reject) noexcept {
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(ResolveSayHelloCallbacks)
  void ResolveSayHelloCallbacks(
      std::function<void(const std::string &)> const &resolve,
      std::function<void(const std::string &)> const & /*reject*/) noexcept {
    resolve("Hello_3");
  }

  REACT_METHOD(RejectSayHelloCallbacks)
  void RejectSayHelloCallbacks(
      std::function<void(const std::string &)> const & /*resolve*/,
      std::function<void(const std::string &)> const &reject) noexcept {
    reject("Goodbye");
  }

  REACT_METHOD(StaticDivideCallbacks)
  static void StaticDivideCallbacks(
      int x,
      int y,
      std::function<void(int)> const &resolve,
      std::function<void(const std::string &)> const &reject) noexcept {
    if (y != 0) {
      resolve(x / y);
    } else {
      reject("Division by 0");
    }
  }

  REACT_METHOD(StaticNegateCallbacks)
  static void StaticNegateCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(const std::string &)> const &reject) noexcept {
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(StaticResolveSayHelloCallbacks)
  static void StaticResolveSayHelloCallbacks(
      std::function<void(const std::string &)> const &resolve,
      std::function<void(const std::string &)> const & /*reject*/) noexcept {
    resolve("Hello_3");
  }

  REACT_METHOD(StaticRejectSayHelloCallbacks)
  static void StaticRejectSayHelloCallbacks(
      std::function<void(const std::string &)> const & /*resolve*/,
      std::function<void(const std::string &)> const &reject) noexcept {
    reject("Goodbye");
  }

  REACT_METHOD(DividePromise)
  void DividePromise(int x, int y, ReactPromise<int> &&result) noexcept {
    if (y != 0) {
      result.Resolve(x / y);
    } else {
      ReactError error{};
      error.Message = "Division by 0";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(NegatePromise)
  void NegatePromise(int x, ReactPromise<int> &&result) noexcept {
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(ResolveSayHelloPromise)
  void ResolveSayHelloPromise(ReactPromise<std::string> &&result) noexcept {
    result.Resolve("Hello_4");
  }

  REACT_METHOD(RejectSayHelloPromise)
  void RejectSayHelloPromise(ReactPromise<std::string> &&result) noexcept {
    ReactError error{};
    error.Message = "Promise rejected";
    result.Reject(std::move(error));
  }

  REACT_METHOD(StaticDividePromise)
  static void StaticDividePromise(int x, int y, ReactPromise<int> &&result) noexcept {
    if (y != 0) {
      result.Resolve(x / y);
    } else {
      ReactError error{};
      error.Message = "Division by 0";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticNegatePromise)
  static void StaticNegatePromise(int x, ReactPromise<int> &&result) noexcept {
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticResolveSayHelloPromise)
  static void StaticResolveSayHelloPromise(ReactPromise<std::string> &&result) noexcept {
    result.Resolve("Hello_4");
  }

  REACT_METHOD(StaticRejectSayHelloPromise)
  static void StaticRejectSayHelloPromise(ReactPromise<std::string> &&result) noexcept {
    ReactError error{};
    error.Message = "Promise rejected";
    result.Reject(std::move(error));
  }

  REACT_SYNC_METHOD(AddSync)
  int AddSync(int x, int y) noexcept {
    return x + y;
  }

  REACT_SYNC_METHOD(NegateSync)
  int NegateSync(int x) noexcept {
    return -x;
  }

  REACT_SYNC_METHOD(SayHelloSync)
  std::string SayHelloSync() noexcept {
    return "Hello";
  }

  REACT_SYNC_METHOD(StaticAddSync)
  static int StaticAddSync(int x, int y) noexcept {
    return x + y;
  }

  REACT_SYNC_METHOD(StaticNegateSync)
  static int StaticNegateSync(int x) noexcept {
    return -x;
  }

  REACT_SYNC_METHOD(StaticSayHelloSync)
  static std::string StaticSayHelloSync() noexcept {
    return "Hello";
  }

  REACT_CONSTANT(Constant1)
  const std::string Constant1{"MyConstant1"};

  REACT_CONSTANT2(Constant2, L"const2")
  const std::string Constant2{"MyConstant2"};

  REACT_CONSTANT2(Constant3, L"const3")
  static constexpr Point Constant3{/*X =*/2, /*Y =*/3};

  REACT_CONSTANT(Constant4)
  static constexpr Point Constant4{/*X =*/3, /*Y =*/4};

  REACT_CONSTANT_PROVIDER(Constant5)
  void Constant5(ReactConstantProvider &provider) noexcept {
    provider.Add(L"const51", Point{/*X =*/12, /*Y =*/14});
    provider.Add(L"const52", "MyConstant52");
  }

  REACT_CONSTANT_PROVIDER(Constant6)
  static void Constant6(ReactConstantProvider &provider) noexcept {
    provider.Add(L"const61", Point{/*X =*/15, /*Y =*/17});
    provider.Add(L"const62", "MyConstant62");
  }

  REACT_EVENT(OnIntResult1)
  std::function<void(int)> OnIntResult1;

  REACT_EVENT2(OnPointResult2, L"onPointResult2")
  std::function<void(const Point &)> OnPointResult2;

  std::string Message;
  static std::string StaticMessage;
};

/*static*/ std::string SimpleNativeModule::StaticMessage;

// Common base class used by all unit tests below.
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
  m_builderMock.Call1(L"SayHelloCallback", std::function<void(const std::string &)>([
                      ](const std::string &result) noexcept { REQUIRE(result == "Hello_2"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticAddCallback", "NativeModuleTest") {
  m_builderMock.Call1(
      L"StaticAddCallback", std::function<void(int)>([](int result) noexcept { REQUIRE(result == 60); }), 4, 56);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticNegateCallback", "NativeModuleTest") {
  m_builderMock.Call1(
      L"StaticNegateCallback", std::function<void(int)>([](int result) noexcept { REQUIRE(result == -33); }), 33);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticSayHelloCallback", "NativeModuleTest") {
  m_builderMock.Call1(L"StaticSayHelloCallback", std::function<void(const std::string &)>([
                      ](const std::string &result) noexcept { REQUIRE(result == "Static Hello_2"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_DivideCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"DivideCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      2);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_DivideCallbacksError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"DivideCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      0);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_NegateCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"NegateCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      5);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_NegateCallbacksError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"NegateCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      -5);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_ResolveSayHelloCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"ResolveSayHelloCallbacks",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_3"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Goodbye"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_RejectSayHelloCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"RejectSayHelloCallbacks",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_3"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Goodbye"); }));
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticDivideCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticDivideCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      2);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticDivideCallbacksError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticDivideCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      0);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticNegateCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticNegateCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      5);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticNegateCallbacksError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticNegateCallbacks",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      -5);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticResolveSayHelloCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticResolveSayHelloCallbacks",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_3"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Goodbye"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticRejectSayHelloCallbacks", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticRejectSayHelloCallbacks",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_3"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Goodbye"); }));
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_DividePromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"DividePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      2);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_DividePromiseError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"DividePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      0);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_NegatePromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"NegatePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      5);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_NegatePromiseError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"NegatePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      -5);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_ResolveSayHelloPromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"ResolveSayHelloPromise",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_4"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Promise rejected"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_RejectSayHelloPromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"RejectSayHelloPromise",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_4"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Promise rejected"); }));
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticDividePromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticDividePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      2);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticDividePromiseError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticDividePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == 3); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Division by 0"); }),
      6,
      0);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticNegatePromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticNegatePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      5);
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticNegatePromiseError", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticNegatePromise",
      std::function<void(int)>([](int result) noexcept { REQUIRE(result == -5); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Already negative"); }),
      -5);
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticResolveSayHelloPromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticResolveSayHelloPromise",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_4"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Promise rejected"); }));
  REQUIRE(m_builderMock.IsResolveCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodCall_StaticRejectSayHelloPromise", "NativeModuleTest") {
  m_builderMock.Call2(
      L"StaticRejectSayHelloPromise",
      std::function<void(const std::string &)>(
          [](const std::string &result) noexcept { REQUIRE(result == "Hello_4"); }),
      std::function<void(JSValue const &)>(
          [](JSValue const &error) noexcept { REQUIRE(error["message"] == "Promise rejected"); }));
  REQUIRE(m_builderMock.IsRejectCallbackCalled());
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodSyncCall_AddSync", "NativeModuleTest") {
  int result;
  m_builderMock.CallSync(L"AddSync", /*out*/ result, 3, 5);
  REQUIRE(result == 8);
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodSyncCall_NegateSync", "NativeModuleTest") {
  int result;
  m_builderMock.CallSync(L"NegateSync", /*out*/ result, 7);
  REQUIRE(result == -7);
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodSyncCall_SayHelloSync", "NativeModuleTest") {
  std::string result;
  m_builderMock.CallSync(L"SayHelloSync", /*out*/ result);
  REQUIRE(result == "Hello");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodSyncCall_StaticAddSync", "NativeModuleTest") {
  int result;
  m_builderMock.CallSync(L"StaticAddSync", /*out*/ result, 3, 5);
  REQUIRE(result == 8);
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodSyncCall_StaticNegateSync", "NativeModuleTest") {
  int result;
  m_builderMock.CallSync(L"StaticNegateSync", /*out*/ result, 7);
  REQUIRE(result == -7);
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestMethodSyncCall_StaticSayHelloSync", "NativeModuleTest") {
  std::string result;
  m_builderMock.CallSync(L"StaticSayHelloSync", /*out*/ result);
  REQUIRE(result == "Hello");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestConstants", "NativeModuleTest") {
  auto constants = m_builderMock.GetConstants();
  REQUIRE(constants["Constant1"] == "MyConstant1");
  REQUIRE(constants["const2"] == "MyConstant2");
  REQUIRE(constants["const3"]["X"] == 2);
  REQUIRE(constants["const3"]["Y"] == 3);
  REQUIRE(constants["Constant4"]["X"] == 3);
  REQUIRE(constants["Constant4"]["Y"] == 4);
  REQUIRE(constants["const51"]["X"] == 12);
  REQUIRE(constants["const51"]["Y"] == 14);
  REQUIRE(constants["const52"] == "MyConstant52");
  REQUIRE(constants["const61"]["X"] == 15);
  REQUIRE(constants["const61"]["Y"] == 17);
  REQUIRE(constants["const62"] == "MyConstant62");
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestEvent_EventField1", "NativeModuleTest") {
  bool eventRaised = false;
  m_builderMock.SetEventHandler(L"OnIntResult1", std::function<void(int)>([&eventRaised](int eventArg) noexcept {
                                  REQUIRE(eventArg == 42);
                                  eventRaised = true;
                                }));

  m_module->OnIntResult1(42);
  REQUIRE(eventRaised == true);
}

TEST_CASE_METHOD(NativeModuleTestFixture, "TestEvent_EventField2", "NativeModuleTest") {
  bool eventRaised = false;
  m_builderMock.SetEventHandler(
      L"onPointResult2", std::function<void(const Point &)>([&eventRaised](const Point &eventArg) noexcept {
        REQUIRE(eventArg.X == 4);
        REQUIRE(eventArg.Y == 2);
        eventRaised = true;
      }));

  m_module->OnPointResult2(Point{/*X =*/4, /*Y =*/2});
  REQUIRE(eventRaised == true);
}

} // namespace winrt::Microsoft::ReactNative::Bridge

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once
#include "winrt/Microsoft.ReactNative.h"

#include "JSValueReader.h"
#include "JSValueWriter.h"
#include "ModuleRegistration.h"
#include "ReactPromise.h"

#include <type_traits>

// REACT_MODULE(moduleStruct, [opt] moduleName, [opt] eventEmitterName)
// Arguments:
// - moduleStruct (required) - the struct name the macro is attached to.
// - moduleName (optional) - the module name visible to JavaScript. Default is the moduleStruct name.
// - eventEmitterName (optional) - the default event emitter name used by REACT_EVENT.
//   Default is the RCTDeviceEventEmitter.
//
// REACT_MODULE annotates a C++ struct as a ReactNative module.
// It can be any struct which can be instantiated using a default constructor.
// Note that it must be a 'struct', not 'class' because macro does a forward declaration using the 'struct' keyword.
#define REACT_MODULE(/* moduleStruct, [opt] moduleName, [opt] eventEmitterName */...) \
  INTERNAL_REACT_MODULE(__VA_ARGS__)(__VA_ARGS__)

// REACT_INIT(method)
// Arguments:
// - method (required) - the method name the macro is attached to.
//
// REACT_INIT annotates a method that is called when a native module is initialized.
// It must have 'IReactContext const &' parameter.
// It must be an instance method.
#define REACT_INIT(method) INTERNAL_REACT_MEMBER_2_ARGS(InitMethod, method)

// REACT_METHOD(method, [opt] methodName)
// Arguments:
// - method (required) - the method name the macro is attached to.
// - methodName (optional) - the method name visible to JavaScript. Default is the method name.
//
// REACT_METHOD annotates a method to export to JavaScript.
// It declares an asynchronous method. To return a value:
// - Return void and have a Callback as a last parameter. The Callback type can be any std::function like type. E.g.
//   Func<void(Args...)>.
// - Return void and have two callbacks as last parameters. One is used to return value and another an error.
// - Return void and have a ReactPromise as a last parameter. In JavaScript the method returns Promise.
// - Return non-void value. In JavaScript it is treated as a method with one Callback. Return std::pair<Error, Value> to
//   be able to communicate error condition.
// It can be an instance or static method.
#define REACT_METHOD(/* method, [opt] methodName */...) INTERNAL_REACT_MEMBER(__VA_ARGS__)(AsyncMethod, __VA_ARGS__)

// REACT_SYNC_METHOD(method, [opt] methodName)
// Arguments:
// - method (required) - the method name the macro is attached to.
// - methodName (optional) - the method name visible to JavaScript. Default is the method name.
//
// REACT_SYNC_METHOD annotates a method that is called synchronously.
// It must be used rarely because it may cause out-of-order execution when used along with asynchronous methods.
// The method must return non-void value type.
// It can be an instance or static method.
#define REACT_SYNC_METHOD(/* method, [opt] methodName */...) INTERNAL_REACT_MEMBER(__VA_ARGS__)(SyncMethod, __VA_ARGS__)

// REACT_CONSTANT_PROVIDER(method)
// Arguments:
// - method (required) - the method name the macro is attached to.
//
// REACT_CONSTANT_PROVIDER annotates a method that defines constants.
// It must have 'ReactConstantProvider &' parameter.
// It can be an instance or static method.
#define REACT_CONSTANT_PROVIDER(method) INTERNAL_REACT_MEMBER_2_ARGS(ConstantMethod, method)

// REACT_CONSTANT(field, [opt] constantName)
// Arguments:
// - field (required) - the field name the macro is attached to.
// - constantName (optional) - the constant name visible to JavaScript. Default is the field name.
//
// REACT_CONSTANT annotates a field that defines a constant.
// It can be an instance or static field.
#define REACT_CONSTANT(/* field, [opt] constantName */...) \
  INTERNAL_REACT_MEMBER(__VA_ARGS__)(ConstantField, __VA_ARGS__)

// REACT_EVENT(field, [opt] eventName, [opt] eventEmitterName)
// Arguments:
// - field (required) - the field name the macro is attached to.
// - eventName (optional) - the JavaScript event name. Default is the field name.
// - eventEmitterName (optional) - the JavaScript module event emitter name. Default is module's eventEmitterName which
//   is by default 'RCTDeviceEventEmitter'.
//
// REACT_EVENT annotates a field that helps raise a JavaScript event.
// The field type can be any std::function like type. E.g. Func<void(Args...)>.
// It must be an instance field.
#define REACT_EVENT(/* field, [opt] eventName, [opt] eventEmitterName */...) \
  INTERNAL_REACT_MEMBER(__VA_ARGS__)(EventField, __VA_ARGS__)

// REACT_FUNCTION(field, [opt] functionName, [opt] moduleName)
// Arguments:
// - field (required) - the field name the macro is attached to.
// - functionName (optional) - the JavaScript function name. Default is the field name.
// - moduleName (optional) - the JavaScript module name. Default is module's moduleName which is by default the class
//   name.
//
// REACT_FUNCTION annotates a field that helps calling a JavaScript function.
// The field type can be any std::function like type. E.g. Func<void(Args...)>.
// It must be an instance field.
#define REACT_FUNCTION(/* field, [opt] functionName, [opt] moduleName */...) \
  INTERNAL_REACT_MEMBER(__VA_ARGS__)(FunctionField, __VA_ARGS__)

namespace winrt::Microsoft::ReactNative {

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

// Finds how many callbacks the function signature has.
template <class... TArgs>
constexpr size_t GetCallbackCount() noexcept {
  using TupleType = std::tuple<std::remove_const_t<std::remove_reference_t<TArgs>>...>;
  if constexpr (sizeof...(TArgs) >= 2) {
    return (IsCallback<std::tuple_element_t<sizeof...(TArgs) - 1u, TupleType>>::value ? 1 : 0) +
        (IsCallback<std::tuple_element_t<sizeof...(TArgs) - 2u, TupleType>>::value ? 1 : 0);
  } else if constexpr (sizeof...(TArgs) == 1) {
    return IsCallback<std::tuple_element_t<0, TupleType>>::value ? 1 : 0;
  } else {
    return 0;
  }
}

template <class T>
struct IsPromise : std::false_type {};
template <class T>
struct IsPromise<ReactPromise<T>> : std::true_type {};

template <class TResult, class TArg>
constexpr void ValidateCoroutineArg() noexcept {
  if constexpr (std::is_same_v<TResult, fire_and_forget>) {
    static_assert(
        !std::is_reference_v<TArg> && !std::is_pointer_v<TArg>,
        "Coroutine parameter must be passed by value for safe access: " __FUNCSIG__);
  }
}

} // namespace Internal

//==============================================================================
// Module registration helpers
//==============================================================================

template <class TMethod>
struct ModuleInitMethodInfo;

template <class TModule>
struct ModuleInitMethodInfo<void (TModule::*)(IReactContext const &) noexcept> {
  using ModuleType = TModule;
  using MethodType = void (TModule::*)(IReactContext const &) noexcept;

  static InitializerDelegate GetInitializer(void *module, MethodType method) noexcept {
    return [ module = static_cast<ModuleType *>(module), method ](IReactContext const &reactContext) noexcept {
      (module->*method)(reactContext);
    };
  }
};

template <class TFunc>
struct ModuleMethodInfo;

// Instance asynchronous method
template <class TModule, class TResult, class... TArgs>
struct ModuleMethodInfo<TResult (TModule::*)(TArgs...) noexcept> {
  constexpr static bool HasPromise() noexcept {
    if constexpr (sizeof...(TArgs) > 0) {
      return Internal::IsPromise<std::remove_const_t<std::remove_reference_t<
          std::tuple_element_t<sizeof...(TArgs) - 1, std::tuple<std::remove_reference_t<TArgs>...>>>>>::value;
    }
    return false;
  }

  constexpr static size_t CallbackCount = Internal::GetCallbackCount<TArgs...>();
  using ModuleType = TModule;
  using MethodType = TResult (TModule::*)(TArgs...) noexcept;
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
        // Some native modules use first callback as failure, others use second. We make them both to
        // behave the same way and let developers to assign meaning to the first and second callbacks.
        auto firstCallback = CallbackCreator<std::remove_const_t<
            std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 2, ArgTuple>>>>::Create(argWriter, resolve);
        auto secondCallback = CallbackCreator<std::remove_const_t<
            std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>>>::Create(argWriter, reject);
        (module->*method)(std::get<I>(std::move(typedArgs))..., std::move(firstCallback), std::move(secondCallback));
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

    // Async method with return value
    static MethodDelegate GetFunc(ModuleType *module, MethodType method, std::integral_constant<size_t, 4>) noexcept {
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
    (Internal::ValidateCoroutineArg<TResult, TArgs>(), ...);
    constexpr bool isVoidResult = std::is_void_v<TResult> || std::is_same_v<TResult, winrt::fire_and_forget>;
    if constexpr (!isVoidResult) {
      returnType = MethodReturnType::Callback;
    } else if constexpr (HasPromise()) {
      returnType = MethodReturnType::Promise;
    } else if constexpr (CallbackCount == 2) {
      returnType = MethodReturnType::TwoCallbacks;
    } else if constexpr (CallbackCount == 1) {
      returnType = MethodReturnType::Callback;
    } else {
      returnType = MethodReturnType::Void;
    }

    constexpr int selector = !isVoidResult ? 4 : HasPromise() ? 3 : CallbackCount;
    return Invoker<IndexSequence>::GetFunc(
        static_cast<ModuleType *>(module), method, std::integral_constant<size_t, selector>{});
  }
};

// Static asynchronous method
template <class TResult, class... TArgs>
struct ModuleMethodInfo<TResult (*)(TArgs...) noexcept> {
  constexpr static bool HasPromise() noexcept {
    if constexpr (sizeof...(TArgs) > 0) {
      return Internal::IsPromise<std::remove_const_t<std::remove_reference_t<
          std::tuple_element_t<sizeof...(TArgs) - 1, std::tuple<std::remove_reference_t<TArgs>...>>>>>::value;
    }
    return false;
  }

  constexpr static size_t CallbackCount = Internal::GetCallbackCount<TArgs...>();
  using MethodType = TResult (*)(TArgs...) noexcept;
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
        // Some native modules use first callback as failure, others use second. We make them both to
        // behave the same way and let developers to assign meaning to the first and second callbacks.
        auto firstCallback = CallbackCreator<std::remove_const_t<
            std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 2, ArgTuple>>>>::Create(argWriter, resolve);
        auto secondCallback = CallbackCreator<std::remove_const_t<
            std::remove_reference_t<std::tuple_element_t<sizeof...(TArgs) - 1, ArgTuple>>>>::Create(argWriter, reject);
        (*method)(std::get<I>(std::move(typedArgs))..., std::move(firstCallback), std::move(secondCallback));
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

    // Async method with return value
    static MethodDelegate GetFunc(MethodType method, std::integral_constant<size_t, 4>) noexcept {
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
    (Internal::ValidateCoroutineArg<TResult, TArgs>(), ...);
    constexpr bool isVoidResult = std::is_void_v<TResult> || std::is_same_v<TResult, winrt::fire_and_forget>;
    if constexpr (!isVoidResult) {
      returnType = MethodReturnType::Callback;
    } else if constexpr (HasPromise()) {
      returnType = MethodReturnType::Promise;
    } else if constexpr (CallbackCount == 2) {
      returnType = MethodReturnType::TwoCallbacks;
    } else if constexpr (CallbackCount == 1) {
      returnType = MethodReturnType::Callback;
    } else {
      returnType = MethodReturnType::Void;
    }

    constexpr int selector = !isVoidResult ? 4 : HasPromise() ? 3 : CallbackCount;
    return Invoker<IndexSequence>::GetFunc(method, std::integral_constant<size_t, selector>{});
  }
};

template <class TFunc>
struct ModuleSyncMethodInfo;

// Instance synchronous method
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

// Static synchronous method
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

  static ConstantProviderDelegate GetConstantProvider(void *module, std::wstring_view name, FieldType field) noexcept {
    return
        [ module = static_cast<ModuleType *>(module), name, field ](IJSValueWriter const &argWriter) mutable noexcept {
      WriteProperty(argWriter, name, module->*field);
    };
  }
};

template <class TValue>
struct ModuleConstFieldInfo<TValue *> {
  using FieldType = TValue *;

  static ConstantProviderDelegate
  GetConstantProvider(void * /*module*/, std::wstring_view name, FieldType field) noexcept {
    return [ name, field ](IJSValueWriter const &argWriter) mutable noexcept {
      WriteProperty(argWriter, name, *field);
    };
  }
};

struct ReactConstantProvider {
  ReactConstantProvider(IJSValueWriter const &writer) noexcept : m_writer{writer} {}

  template <class T>
  void Add(std::wstring_view name, const T &value) noexcept {
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

  static ConstantProviderDelegate GetConstantProvider(void *module, MethodType method) noexcept {
    return [ module = static_cast<ModuleType *>(module), method ](IJSValueWriter const &argWriter) mutable noexcept {
      ReactConstantProvider constantProvider{argWriter};
      (module->*method)(constantProvider);
    };
  }
};

template <>
struct ModuleConstantInfo<void (*)(ReactConstantProvider &) noexcept> {
  using MethodType = void (*)(ReactConstantProvider &) noexcept;

  static ConstantProviderDelegate GetConstantProvider(void * /*module*/, MethodType method) noexcept {
    return [method](IJSValueWriter const &argWriter) mutable noexcept {
      ReactConstantProvider constantProvider{argWriter};
      (*method)(constantProvider);
    };
  }
};

template <class TField>
struct ModuleEventFieldInfo;

template <class TModule, template <class> class TFunc, class... TArgs>
struct ModuleEventFieldInfo<TFunc<void(TArgs...)> TModule::*> {
  using ModuleType = TModule;
  using EventType = TFunc<void(TArgs...)>;
  using FieldType = EventType TModule::*;

  static InitializerDelegate GetEventHandlerInitializer(
      void *module,
      FieldType field,
      std::wstring_view eventName,
      std::wstring_view eventEmitterName) noexcept {
    return [ module = static_cast<ModuleType *>(module), field, eventName, eventEmitterName ](
        IReactContext const &reactContext) noexcept {
      module->*field = [ reactContext, eventEmitterName, eventName ](TArgs... args) noexcept {
        reactContext.EmitJSEvent(
            eventEmitterName, eventName, [&args...]([[maybe_unused]] IJSValueWriter const &argWriter) noexcept {
              (void)argWriter; // [[maybe_unused]] above does not work
              (WriteValue(argWriter, args), ...);
            });
      };
    };
  }
};

template <class TField>
struct ModuleFunctionFieldInfo;

template <class TModule, template <class> class TFunc, class... TArgs>
struct ModuleFunctionFieldInfo<TFunc<void(TArgs...)> TModule::*> {
  using ModuleType = TModule;
  using FunctionType = TFunc<void(TArgs...)>;
  using FieldType = FunctionType TModule::*;

  static InitializerDelegate GetFunctionInitializer(
      void *module,
      FieldType field,
      std::wstring_view functionName,
      std::wstring_view moduleName) noexcept {
    return [ module = static_cast<ModuleType *>(module), field, functionName, moduleName ](
        IReactContext const &reactContext) noexcept {
      module->*field = [ reactContext, functionName, moduleName ](TArgs... args) noexcept {
        reactContext.CallJSFunction(moduleName, functionName, [&args...](IJSValueWriter const &argWriter) noexcept {
          WriteArgs(argWriter, args...);
        });
      };
    };
  }
};

template <int I>
using ReactAttributeId = std::integral_constant<int, I>;

template <class TModule>
struct ReactMemberInfoIterator {
  template <int StartIndex, class TVisitor>
  constexpr void ForEachMember(TVisitor &visitor) noexcept {
    ForEachMember<StartIndex>(visitor, static_cast<std::make_index_sequence<10> *>(nullptr));
  }

  template <int I, class TVisitor>
  constexpr void GetMemberInfo(TVisitor &visitor) noexcept {
    if constexpr (decltype(HasGetReactMemberAttribute(visitor, ReactAttributeId<I>{}))::value) {
      TModule::template GetReactMemberAttribute<TModule>(visitor, ReactAttributeId<I>{});
    }
  }

 private:
  template <class TVisitor, int I>
  static auto HasGetReactMemberAttribute(TVisitor &visitor, ReactAttributeId<I> id)
      -> decltype(TModule::template GetReactMemberAttribute<TModule>(visitor, id), std::true_type{});
  static auto HasGetReactMemberAttribute(...) -> std::false_type;

  // Visit members in groups of 10 to avoid deep recursion.
  template <int StartIndex, class TVisitor, int... I>
  constexpr void ForEachMember(TVisitor &visitor, std::index_sequence<I...> *) noexcept {
    if constexpr (decltype(HasGetReactMemberAttribute(visitor, ReactAttributeId<StartIndex>{}))::value) {
      (GetMemberInfo<StartIndex + I>(visitor), ...);
      ForEachMember<StartIndex + sizeof...(I)>(visitor, static_cast<std::make_index_sequence<10> *>(nullptr));
    }
  }
};

enum class ReactMemberKind {
  InitMethod,
  AsyncMethod,
  SyncMethod,
  ConstantMethod,
  ConstantField,
  EventField,
  FunctionField,
};

template <ReactMemberKind MemberKind>
struct ReactMemberAttribute : std::integral_constant<ReactMemberKind, MemberKind> {
  constexpr ReactMemberAttribute(std::wstring_view jsMemberName, std::wstring_view jsModuleName) noexcept
      : JSMemberName{jsMemberName}, JSModuleName{jsModuleName} {}

  std::wstring_view JSMemberName;
  std::wstring_view JSModuleName;
};

using ReactInitMethodAttribute = ReactMemberAttribute<ReactMemberKind::InitMethod>;
using ReactAsyncMethodAttribute = ReactMemberAttribute<ReactMemberKind::AsyncMethod>;
using ReactSyncMethodAttribute = ReactMemberAttribute<ReactMemberKind::SyncMethod>;
using ReactConstantMethodAttribute = ReactMemberAttribute<ReactMemberKind::ConstantMethod>;
using ReactConstantFieldAttribute = ReactMemberAttribute<ReactMemberKind::ConstantField>;
using ReactEventFieldAttribute = ReactMemberAttribute<ReactMemberKind::EventField>;
using ReactFunctionFieldAttribute = ReactMemberAttribute<ReactMemberKind::FunctionField>;

template <class T>
struct IsReactMemberAttribute : std::false_type {};
template <ReactMemberKind MemberKind>
struct IsReactMemberAttribute<ReactMemberAttribute<MemberKind>> : std::true_type {};

template <class TModule>
struct ReactModuleBuilder {
  ReactModuleBuilder(TModule *module, IReactModuleBuilder const &moduleBuilder) noexcept
      : m_module{module}, m_moduleBuilder{moduleBuilder} {}

  template <int I>
  void RegisterModule(std::wstring_view moduleName, std::wstring_view eventEmitterName, ReactAttributeId<I>) noexcept {
    RegisterModuleName(moduleName, eventEmitterName);
    ReactMemberInfoIterator<TModule>{}.ForEachMember<I + 1>(*this);
  }

  void RegisterModuleName(std::wstring_view moduleName, std::wstring_view eventEmitterName = L"") noexcept {
    m_moduleName = moduleName;
    m_eventEmitterName = !eventEmitterName.empty() ? eventEmitterName : L"RCTDeviceEventEmitter";
  }

  void CompleteRegistration() noexcept {
    // Add REACT_INIT initializers after REACT_EVENT and REACT_FUNCTION initializers.
    // This way REACT_INIT method is invoked after event and function fields are initialized.
    for (auto &initializer : m_initializers) {
      m_moduleBuilder.AddInitializer(initializer);
    }
  }

  template <class TMember, class TAttribute, int I>
  void Visit(
      [[maybe_unused]] TMember member,
      ReactAttributeId<I> /*attributeId*/,
      [[maybe_unused]] TAttribute attributeInfo) noexcept {
    if constexpr (std::is_same_v<TAttribute, ReactInitMethodAttribute>) {
      RegisterInitMethod(member);
    } else if constexpr (std::is_same_v<TAttribute, ReactAsyncMethodAttribute>) {
      RegisterMethod(member, attributeInfo.JSMemberName);
    } else if constexpr (std::is_same_v<TAttribute, ReactSyncMethodAttribute>) {
      RegisterSyncMethod(member, attributeInfo.JSMemberName);
    } else if constexpr (std::is_same_v<TAttribute, ReactConstantMethodAttribute>) {
      RegisterConstantMethod(member);
    } else if constexpr (std::is_same_v<TAttribute, ReactConstantFieldAttribute>) {
      RegisterConstantField(member, attributeInfo.JSMemberName);
    } else if constexpr (std::is_same_v<TAttribute, ReactEventFieldAttribute>) {
      RegisterEventField(member, attributeInfo.JSMemberName, attributeInfo.JSModuleName);
    } else if constexpr (std::is_same_v<TAttribute, ReactFunctionFieldAttribute>) {
      RegisterFunctionField(member, attributeInfo.JSMemberName, attributeInfo.JSModuleName);
    }
  }

  template <class TMethod>
  void RegisterInitMethod(TMethod method) noexcept {
    auto initializer = ModuleInitMethodInfo<TMethod>::GetInitializer(m_module, method);
    m_initializers.push_back(std::move(initializer));
  }

  template <class TMethod>
  void RegisterMethod(TMethod method, std::wstring_view name) noexcept {
    MethodReturnType returnType;
    auto methodDelegate = ModuleMethodInfo<TMethod>::GetMethodDelegate(m_module, method, /*out*/ returnType);
    m_moduleBuilder.AddMethod(name, returnType, methodDelegate);
  }

  template <class TMethod>
  void RegisterSyncMethod(TMethod method, std::wstring_view name) noexcept {
    auto syncMethodDelegate = ModuleSyncMethodInfo<TMethod>::GetMethodDelegate(m_module, method);
    m_moduleBuilder.AddSyncMethod(name, syncMethodDelegate);
  }

  template <class TMethod>
  void RegisterConstantMethod(TMethod method) noexcept {
    auto constantProvider = ModuleConstantInfo<TMethod>::GetConstantProvider(m_module, method);
    m_moduleBuilder.AddConstantProvider(constantProvider);
  }

  template <class TField>
  void RegisterConstantField(TField field, std::wstring_view name) noexcept {
    auto constantProvider = ModuleConstFieldInfo<TField>::GetConstantProvider(m_module, name, field);
    m_moduleBuilder.AddConstantProvider(constantProvider);
  }

  template <class TField>
  void
  RegisterEventField(TField field, std::wstring_view eventName, std::wstring_view eventEmitterName = L"") noexcept {
    auto eventHandlerInitializer = ModuleEventFieldInfo<TField>::GetEventHandlerInitializer(
        m_module, field, eventName, !eventEmitterName.empty() ? eventEmitterName : m_eventEmitterName);
    m_moduleBuilder.AddInitializer(eventHandlerInitializer);
  }

  template <class TField>
  void RegisterFunctionField(TField field, std::wstring_view name, std::wstring_view moduleName = L"") noexcept {
    auto functionInitializer = ModuleFunctionFieldInfo<TField>::GetFunctionInitializer(
        m_module, field, name, !moduleName.empty() ? moduleName : m_moduleName);
    m_moduleBuilder.AddInitializer(functionInitializer);
  }

 private:
  void *m_module;
  IReactModuleBuilder m_moduleBuilder;
  std::wstring_view m_moduleName{L""};
  std::wstring_view m_eventEmitterName{L""};
  std::vector<InitializerDelegate> m_initializers;
};

struct VerificationResult {
  size_t MethodNameCount{0};
  size_t MatchCount{0};
  int MatchedMemberId{0};
};

template <class TModule>
struct ReactModuleVerifier {
  static constexpr VerificationResult GetAsyncMethodCount(std::wstring_view name) noexcept {
    return GetMemberCount(name, ReactMemberKind::AsyncMethod);
  }

  static constexpr VerificationResult GetSyncMethodCount(std::wstring_view name) noexcept {
    return GetMemberCount(name, ReactMemberKind::SyncMethod);
  }

  static constexpr VerificationResult GetMemberCount(std::wstring_view name, ReactMemberKind memberKind) noexcept {
    ReactModuleVerifier verifier{name, memberKind};
    GetReactModuleInfo(static_cast<TModule *>(nullptr), verifier);
    return verifier.m_result;
  }

  constexpr ReactModuleVerifier(std::wstring_view memberName, ReactMemberKind memberKind) noexcept
      : m_memberName{memberName}, m_memberKind{memberKind} {}

  template <int I>
  constexpr void RegisterModule(std::wstring_view /*_*/, std::wstring_view /*_*/, ReactAttributeId<I>) noexcept {
    ReactMemberInfoIterator<TModule>{}.ForEachMember<I + 1>(*this);
  }

  template <class TMember, class TAttribute, int I>
  constexpr void Visit(
      TMember /*member*/,
      [[maybe_unused]] ReactAttributeId<I> attributeId,
      [[maybe_unused]] TAttribute attributeInfo) noexcept {
    if constexpr (IsReactMemberAttribute<TAttribute>::value) {
      if (attributeInfo.JSMemberName == m_memberName) {
        if (IsMethod(attributeInfo())) {
          ++m_result.MethodNameCount;
        }

        if (attributeInfo() == m_memberKind) {
          m_result.MatchedMemberId = attributeId();
          ++m_result.MatchCount;
        }
      }
    }
  }

  static bool constexpr IsMethod(ReactMemberKind memberKind) noexcept {
    return memberKind == ReactMemberKind::AsyncMethod || memberKind == ReactMemberKind::SyncMethod;
  }

 private:
  std::wstring_view m_memberName;
  ReactMemberKind m_memberKind;
  VerificationResult m_result;
};

template <class... TArgs>
struct MethodSpecArgs {};

template <class TModule, int I, class TSpecArgs>
struct ReactMethodVerifier {
  static constexpr bool Verify() noexcept {
    ReactMethodVerifier verifier;
    ReactMemberInfoIterator<TModule>{}.GetMemberInfo<I>(verifier);
    return verifier.m_result;
  }

  template <class TMember, class TAttribute, int I>
  constexpr void
  Visit([[maybe_unused]] TMember member, ReactAttributeId<I> /*attributeId*/, TAttribute /*attributeInfo*/) noexcept {
    m_result = true;
  }

 private:
  bool m_result{false};
};

template <class T>
struct BoxedValue : implements<BoxedValue<T>, IBoxedValue> {
  BoxedValue() noexcept {}

  int64_t GetPtr() noexcept {
    return reinterpret_cast<int64_t>(&m_value);
  }

  static T &GetImpl(IBoxedValue &module) noexcept;

 private:
  T m_value{};
};

template <class T>
inline T &BoxedValue<T>::GetImpl(IBoxedValue &module) noexcept {
  return *reinterpret_cast<T *>(module.GetPtr());
}

template <class TModule>
inline ReactModuleProvider MakeModuleProvider() noexcept {
  return [](IReactModuleBuilder const &moduleBuilder) noexcept {
    auto moduleObject = make<BoxedValue<TModule>>();
    auto module = &BoxedValue<TModule>::GetImpl(moduleObject);
    ReactModuleBuilder builder{module, moduleBuilder};
    GetReactModuleInfo(module, builder);
    builder.CompleteRegistration();
    return moduleObject;
  };
}

template <class TModule, class TModuleSpec>
inline ReactModuleProvider MakeTurboModuleProvider() noexcept {
  TModuleSpec::template ValidateModule<TModule>();
  return [](IReactModuleBuilder const &moduleBuilder) noexcept {
    auto moduleObject = make<BoxedValue<TModule>>();
    auto module = &BoxedValue<TModule>::GetImpl(moduleObject);
    ReactModuleBuilder builder{module, moduleBuilder};
    GetReactModuleInfo(module, builder);
    builder.CompleteRegistration();
    return moduleObject;
  };
}

} // namespace winrt::Microsoft::ReactNative

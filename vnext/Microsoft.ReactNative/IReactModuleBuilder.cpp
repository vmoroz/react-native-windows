// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "IReactModuleBuilder.h"
#include "ReactModuleBuilder.g.cpp"
#include <eventWaitHandle/eventWaitHandle.h>
#include <strsafe.h>
#include "DynamicReader.h"
#include "DynamicWriter.h"
#include "ReactHost/MsoUtils.h"

using namespace facebook::xplat::module;

namespace winrt::Microsoft::ReactNative::implementation {

#ifdef DEBUG
// Starting with RNW 0.64, native modules are called on the JS thread.
// Modules will usually call Windows APIs, and those APIs might expect to be called in the UI thread.
// Developers can dispatch work to the UI thread via reactContext.UIDispatcher().Post(). When they fail to do so,
// cppwinrt will grab the failure HRESULT (RPC_E_WRONG_THREAD), convert it to a C++ exception, and throw it.
// However, native module methods are noexcept, meaning the CRT will call std::terminate and not propagate exceptions up
// the stack. Developers are then left with a crashing app and no good way to debug it. To improve developers'
// experience, we can replace the terminate handler temporarily while we call out to the native method. In the terminate
// handler, we can inspect the exception that was thrown and give an error message before going down.
struct TerminateExceptionGuard final {
  TerminateExceptionGuard() {
    m_oldHandler = std::get_terminate();
    std::set_terminate([]() {
      auto ex = std::current_exception();
      if (ex) {
        try {
          std::rethrow_exception(ex);
        } catch (const winrt::hresult_error &hr) {
          wchar_t buf[1024] = {};
          StringCchPrintf(
              buf,
              std::size(buf),
              L"An unhandled exception (0x%x) occurred in a native module. "
              L"The exception message was:\n\n%s",
              hr.code(),
              hr.message().c_str());
          auto messageBox = reinterpret_cast<decltype(&MessageBoxW)>(
              GetProcAddress(GetModuleHandle(L"ext-ms-win-ntuser-dialogbox-l1-1-0.dll"), "MessageBoxW"));
          if (hr.code() == RPC_E_WRONG_THREAD) {
            StringCchCat(
                buf,
                std::size(buf),
                L"\n\nIt's likely that the native module called a Windows API that needs to be called from the UI thread. "
                L"For more information, see https://aka.ms/RNW-UIAPI");
          }
          messageBox(nullptr, buf, L"Unhandled exception in native module", MB_ICONERROR | MB_OK);
        } catch (...) {
        }
      }
    });
  }
  ~TerminateExceptionGuard() {
    std::set_terminate(m_oldHandler);
  }

 private:
  std::terminate_handler m_oldHandler;
};
#define REACT_TERMINATE_GUARD(x) TerminateExceptionGuard x
#else
#define REACT_TERMINATE_GUARD(x)
#endif

//===========================================================================
// ReactModuleBuilder implementation
//===========================================================================

ReactModuleBuilder::ReactModuleBuilder(
    IReactContext const &reactContext,
    IReactPropertyName const &dispatcherName) noexcept
    : m_reactContext{reactContext}, m_dispatcherName{dispatcherName} {
  m_jsDispatcher = reactContext.Properties().Get(ReactDispatcherHelper::JSDispatcherProperty()).as<IReactDispatcher>();
  if (dispatcherName && dispatcherName != ReactDispatcherHelper::JSDispatcherProperty()) {
    m_moduleDispatcher = reactContext.Properties().Get(dispatcherName).as<IReactDispatcher>();
  }
}

void ReactModuleBuilder::AddInitializer(InitializerDelegate const &initializer) noexcept {
  AddDispatchedInitializer(initializer, ReactInitializerType::Method, true);
}

void ReactModuleBuilder::AddConstantProvider(ConstantProviderDelegate const &constantProvider) noexcept {
  AddDispatchedConstantProvider(constantProvider, true);
}

void ReactModuleBuilder::AddMethod(
    hstring const &name,
    MethodReturnType const &returnType,
    MethodDelegate const &method) noexcept {
  AddDispatchedMethod(name, returnType, method, false);
}

void ReactModuleBuilder::AddSyncMethod(hstring const &name, SyncMethodDelegate const &method) noexcept {
  AddDispatchedSyncMethod(name, method, true);
}

void ReactModuleBuilder::AddDispatchedInitializer(
    InitializerDelegate const &initializer,
    ReactInitializerType const &initializerType,
    bool useJSDispatcher) noexcept {
  m_initializers.push_back(InitializerEntry{initializer, initializerType, useJSDispatcher});
}

void ReactModuleBuilder::AddDispatchedFinalizer(FinalizerDelegate const &finalizer, bool useJSDispatcher) noexcept {
  m_finalizers.push_back(FinalizerEntry{finalizer, useJSDispatcher});
}

void ReactModuleBuilder::AddDispatchedConstantProvider(
    ConstantProviderDelegate const &constantProvider,
    bool useJSDispatcher) noexcept {
  m_constantProviders.push_back(ConstantProviderEntry{constantProvider, useJSDispatcher});
}

void ReactModuleBuilder::AddDispatchedMethod(
    hstring const &name,
    MethodReturnType const &returnType,
    MethodDelegate const &method,
    bool useJSDispatcher) noexcept {
  using FuncType = decltype(std::declval<CxxModule::Method>().func);
  auto cxxMethodCallback =
      FuncType([method](folly::dynamic args, CxxModule::Callback resolve, CxxModule::Callback reject) noexcept {
        auto argReader = make<DynamicReader>(args);
        auto resultWriter = make<DynamicWriter>();
        auto resolveCallback = MakeMethodResultCallback(std::move(resolve));
        auto rejectCallback = MakeMethodResultCallback(std::move(reject));

        REACT_TERMINATE_GUARD(term);

        method(argReader, resultWriter, resolveCallback, rejectCallback);
      });

  if (m_moduleDispatcher && useJSDispatcher) {
    cxxMethodCallback =
        FuncType([cxxMethodCallback, jsDispatcher = m_jsDispatcher](
                     folly::dynamic args, CxxModule::Callback resolve, CxxModule::Callback reject) mutable noexcept {
          jsDispatcher.Post([cxxMethodCallback = std::move(cxxMethodCallback),
                             args = std::move(args),
                             resolve = std::move(resolve),
                             reject = std::move(reject)]() mutable noexcept {
            cxxMethodCallback(std::move(args), std::move(resolve), std::move(reject));
          });
        });
  }

  CxxModule::Method cxxMethod(to_string(name), std::move(cxxMethodCallback));

  switch (returnType) {
    case MethodReturnType::Callback:
      cxxMethod.callbacks = 1;
      cxxMethod.isPromise = false;
      break;
    case MethodReturnType::TwoCallbacks:
      cxxMethod.callbacks = 2;
      cxxMethod.isPromise = false;
      break;
    case MethodReturnType::Promise:
      cxxMethod.callbacks = 2;
      cxxMethod.isPromise = true;
      break;
    default:
      cxxMethod.callbacks = 0;
      cxxMethod.isPromise = false;
      break;
  }

  m_methods.push_back(std::move(cxxMethod));
}

void ReactModuleBuilder::AddDispatchedSyncMethod(
    hstring const &name,
    SyncMethodDelegate const &method,
    bool useJSDispatcher) noexcept {
  using SyncFuncType = decltype(std::declval<CxxModule::Method>().syncFunc);
  auto cxxMethodCallback = SyncFuncType([method](folly::dynamic args) noexcept {
    auto argReader = make<DynamicReader>(args);
    auto resultWriter = make<DynamicWriter>();
    method(argReader, resultWriter);
    return get_self<DynamicWriter>(resultWriter)->TakeValue();
  });

  if (m_moduleDispatcher && !useJSDispatcher) {
    cxxMethodCallback =
        SyncFuncType([dispatcher = m_moduleDispatcher, cxxMethodCallback](folly::dynamic args) noexcept {
          folly::dynamic result;
          RunSync(dispatcher, [cxxMethodCallback, &result, &args]() mutable {
            result = cxxMethodCallback(std::move(args));
          });
          return result;
        });
  }

  m_methods.emplace_back(to_string(name), std::move(cxxMethodCallback), CxxModule::SyncTag);
}

/*static*/ MethodResultCallback ReactModuleBuilder::MakeMethodResultCallback(CxxModule::Callback &&callback) noexcept {
  if (callback) {
    return [callback = std::move(callback)](const IJSValueWriter &outputWriter) noexcept {
      if (outputWriter) {
        folly::dynamic argArray = outputWriter.as<DynamicWriter>()->TakeValue();
        callback(std::vector<folly::dynamic>(argArray.begin(), argArray.end()));
      } else {
        callback(std::vector<folly::dynamic>{});
      }
    };
  }

  return {};
}

/*static*/ void ReactModuleBuilder::RunSync(
    IReactDispatcher const &dispatcher,
    ReactDispatcherCallback const &callback) noexcept {
  Mso::ManualResetEvent event;
  auto cancellationGuard = [&event](int * /*ptr*/) { event.Set(); };
  auto guard = std::unique_ptr<int, decltype(cancellationGuard)>{static_cast<int *>(nullptr), cancellationGuard};
  dispatcher.Post([&callback, &event, guard = std::move(guard)]() noexcept {
    callback();
    event.Set();
  });
  event.Wait();
}

std::unique_ptr<CxxModule> ReactModuleBuilder::MakeCxxModule(
    std::string const &name,
    IInspectable const &nativeModule) noexcept {
  InitializeModule();
  return std::make_unique<ABICxxModule>(nativeModule, Mso::Copy(name), GetFinalizer(),  GetConstantProvider(), Mso::Copy(m_methods));
}

void ReactModuleBuilder::InitializeModule() noexcept {
  for (auto const &initializer : m_initializers) {
    if (initializer.InitializerType == ReactInitializerType::Field &&
        (!m_moduleDispatcher || initializer.UseJSDispatcher)) {
      initializer.Delegate(m_reactContext);
    }
  }

  for (auto const &initializer : m_initializers) {
    if (initializer.InitializerType == ReactInitializerType::Method &&
        (!m_moduleDispatcher || initializer.UseJSDispatcher)) {
      initializer.Delegate(m_reactContext);
    }
  }

  if (m_moduleDispatcher) {
    m_moduleDispatcher.Post([initializers = std::move(m_initializers), reactContext = m_reactContext]() {
      for (auto const &initializer : initializers) {
        if (initializer.InitializerType == ReactInitializerType::Field && !initializer.UseJSDispatcher) {
          initializer.Delegate(reactContext);
        }
      }

      for (auto const &initializer : initializers) {
        if (initializer.InitializerType == ReactInitializerType::Method && !initializer.UseJSDispatcher) {
          initializer.Delegate(reactContext);
        }
      }
    });
  }
}

ABICxxModule::Finalizer ReactModuleBuilder::GetFinalizer() noexcept {
  return []() {};
}

ABICxxModule::ConstantProvider ReactModuleBuilder::GetConstantProvider() noexcept {
  auto writeConstants = [constantProviders = m_constantProviders](auto &&result, auto &&predicate) {
    IJSValueWriter argWriter = winrt::make<DynamicWriter>();
    argWriter.WriteObjectBegin();
    for (auto const &constantProvider : constantProviders) {
      if (predicate(constantProvider)) {
        constantProvider.Delegate(argWriter);
      }
    }
    argWriter.WriteObjectEnd();
    folly::dynamic constants = argWriter.as<DynamicWriter>()->TakeValue();
    if (constants.isObject()) {
      for (auto &item : constants.items()) {
        result[item.first.asString()] = std::move(item.second);
      }
    }
  };

  return [writeConstants, moduleDispatcher = m_moduleDispatcher]() {
    std::map<std::string, folly::dynamic> result;

    writeConstants(result, [moduleDispatcher](ConstantProviderEntry const &constantProvider) {
      return !moduleDispatcher || constantProvider.UseJSDispatcher;
    });

    if (moduleDispatcher) {
      RunSync(moduleDispatcher, [writeConstants, &result]() {
        writeConstants(
            result, [](ConstantProviderEntry const &constantProvider) { return !constantProvider.UseJSDispatcher; });
      });
    }

    return result;
  };
}

} // namespace winrt::Microsoft::ReactNative::implementation

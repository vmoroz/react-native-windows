// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "ABICxxModule.h"
#include <strsafe.h>
#include "DynamicReader.h"
#include "DynamicWriter.h"
#include "IReactContext.h"
#include "MsoUtils.h"
#include "eventWaitHandle/eventWaitHandle.h"

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

using namespace facebook::xplat::module;

namespace winrt::Microsoft::ReactNative {

template <typename T>
static bool HasNonJSEntry(T &&entries) {
  return std::any_of(entries.begin(), entries.end(), [](auto &&entry) { return !entry.UseJSDispatcher; });
}

ABICxxModule::ABICxxModule(
    std::string const &name,
    ReactModuleProvider const &moduleProvider,
    Mso::CntPtr<Mso::React::IReactContext> const &reactContext,
    IReactPropertyName const &dispatcherName) noexcept
    : m_name{name},
      m_moduleBuilder{make_self<implementation::ReactModuleBuilder>()},
      m_nativeModule{moduleProvider(m_moduleBuilder.as<IReactModuleBuilder>())} {
  m_jsDispatcher = reactContext->Properties().Get(ReactDispatcherHelper::JSDispatcherProperty()).as<IReactDispatcher>();
  if (dispatcherName && dispatcherName != ReactDispatcherHelper::JSDispatcherProperty()) {
    m_moduleDispatcher = reactContext->Properties().Get(dispatcherName).as<IReactDispatcher>();
  }

  RunInitializers(reactContext);
  SetupFinalizers(reactContext);
}

std::string ABICxxModule::getName() noexcept {
  return m_name;
}

std::map<std::string, folly::dynamic> ABICxxModule::getConstants() noexcept {
  VerifyElseCrashSz(m_jsDispatcher.HasThreadAccess(), "getConstants must be run from the JS dispatcher.");

  auto const &constantProviders = m_moduleBuilder->GetConstantProviders();
  auto getConstants = [constantProviders,
                       hasModuleDispatcher = static_cast<bool>(m_moduleDispatcher)](bool useJSDispatcher) {
    IJSValueWriter argWriter = winrt::make<DynamicWriter>();
    argWriter.WriteObjectBegin();
    for (auto const &constantProvider : constantProviders) {
      if (useJSDispatcher == (!hasModuleDispatcher || constantProvider.UseJSDispatcher)) {
        constantProvider.Delegate(argWriter);
      }
    }
    argWriter.WriteObjectEnd();
    return argWriter.as<DynamicWriter>()->TakeValue();
  };

  auto jsDispatcherConstants = getConstants(true);
  folly::dynamic moduleDispatcherConstants;
  if (m_moduleDispatcher && HasNonJSEntry(constantProviders)) {
    RunSync(m_moduleDispatcher, [&]() { moduleDispatcherConstants = getConstants(false); });
  }

  auto addDynamicConstants = [](auto &&constantMap, folly::dynamic &&constants) noexcept {
    if (constants.isObject()) {
      for (auto &item : constants.items()) {
        constantMap[item.first.asString()] = std::move(item.second);
      }
    }
  };

  std::map<std::string, folly::dynamic> constants;
  addDynamicConstants(constants, std::move(jsDispatcherConstants));
  addDynamicConstants(constants, std::move(moduleDispatcherConstants));
  return constants;
}

std::vector<CxxModule::Method> ABICxxModule::getMethods() noexcept {
  std::vector<CxxModule::Method> modules;
  modules.reserve(m_moduleBuilder->GetMethods().size() + m_moduleBuilder->GetSyncMethods().size());

  for (auto const &method : m_moduleBuilder->GetMethods()) {
    modules.push_back(CreateCxxMethod(method.first, method.second));
  }

  for (auto const &syncMethod : m_moduleBuilder->GetSyncMethods()) {
    modules.push_back(CreateCxxMethod(syncMethod.first, syncMethod.second));
  }

  return modules;
}

void ABICxxModule::RunInitializers(Mso::CntPtr<Mso::React::IReactContext> const &reactContext) const noexcept {
  VerifyElseCrashSz(m_jsDispatcher.HasThreadAccess(), "RunInitializers must be run from the JS dispatcher.");

  auto const &initializers = m_moduleBuilder->GetInitializers();
  if (initializers.empty()) {
    return;
  }

  auto runInitializers = [initializers,
                          winrtReactContext = winrt::make<implementation::ReactContext>(Mso::Copy(reactContext)),
                          hasModuleDispatcher = static_cast<bool>(m_moduleDispatcher)](
                             ReactInitializerType initializerType, bool useJSDispatcher) {
    for (auto const &initializer : initializers) {
      if (initializer.InitializerType == initializerType &&
          useJSDispatcher == (!hasModuleDispatcher || initializer.UseJSDispatcher)) {
        initializer.Delegate(winrtReactContext);
      }
    }
  };

  runInitializers(ReactInitializerType::Field, true);
  runInitializers(ReactInitializerType::Method, true);

  if (m_moduleDispatcher && HasNonJSEntry(initializers)) {
    m_moduleDispatcher.Post([runInitializers]() noexcept {
      runInitializers(ReactInitializerType::Field, false);
      runInitializers(ReactInitializerType::Method, false);
    });
  }
}

void ABICxxModule::SetupFinalizers(Mso::CntPtr<Mso::React::IReactContext> const &reactContext) const noexcept {
  auto const &finalizers = m_moduleBuilder->GetFinalizers();
  if (finalizers.empty()) {
    return;
  }

  auto runFinalizers = [finalizers,
                        hasModuleDispatcher = static_cast<bool>(m_moduleDispatcher)](bool useJSDispatcher) noexcept {
    for (auto const &finalizer : finalizers) {
      if (useJSDispatcher == (!hasModuleDispatcher || finalizer.UseJSDispatcher)) {
        finalizer.Delegate();
      }
    }
  };

  reactContext->Notifications().Subscribe(
      ReactDispatcherHelper::JSDispatcherShutdownNotification(),
      nullptr,
      [runFinalizers, jsDispatcher = m_jsDispatcher, moduleDispatcher = m_moduleDispatcher](
          auto && /*sender*/, IReactNotificationArgs const &args) {
        VerifyElseCrashSz(jsDispatcher.HasThreadAccess(), "Must run in JS dispatcher");
        if (moduleDispatcher) {
          RunSync(moduleDispatcher, [&runFinalizers]() { runFinalizers(false); });
        }
        runFinalizers(true);
        args.Subscription().Unsubscribe();
      });
}

ABICxxModule::CxxMethod ABICxxModule::CreateCxxMethod(
    std::string const &name,
    implementation::ReactModuleBuilder::Method const &method) const noexcept {
  using FuncType = decltype(std::declval<CxxMethod>().func);
  auto cxxMethodCallback =
      FuncType([method](folly::dynamic args, CxxModule::Callback resolve, CxxModule::Callback reject) noexcept {
        auto argReader = make<DynamicReader>(args);
        auto resultWriter = make<DynamicWriter>();
        auto resolveCallback = MakeMethodResultCallback(std::move(resolve));
        auto rejectCallback = MakeMethodResultCallback(std::move(reject));

        REACT_TERMINATE_GUARD(term);

        method.Delegate(argReader, resultWriter, resolveCallback, rejectCallback);
      });

  // By default the async methods are run in module dispatcher.
  // If we want to run them in JS dispatcher, then we must use the JS dispatcher explictly.
  if (m_moduleDispatcher && method.UseJSDispatcher) {
    cxxMethodCallback =
        FuncType([cxxMethodCallback = std::move(cxxMethodCallback), jsDispatcher = m_jsDispatcher](
                     folly::dynamic args, CxxModule::Callback resolve, CxxModule::Callback reject) mutable noexcept {
          jsDispatcher.Post([cxxMethodCallback = std::move(cxxMethodCallback),
                             args = std::move(args),
                             resolve = std::move(resolve),
                             reject = std::move(reject)]() mutable noexcept {
            cxxMethodCallback(std::move(args), std::move(resolve), std::move(reject));
          });
        });
  }

  CxxMethod cxxMethod{name, std::move(cxxMethodCallback)};
  switch (method.ReturnType) {
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

  return cxxMethod;
}

ABICxxModule::CxxMethod ABICxxModule::CreateCxxMethod(
    std::string const &name,
    implementation::ReactModuleBuilder::SyncMethod const &method) const noexcept {
  using SyncFuncType = decltype(std::declval<CxxMethod>().syncFunc);
  auto cxxMethodCallback = SyncFuncType([method](folly::dynamic args) noexcept {
    auto argReader = make<DynamicReader>(args);
    auto resultWriter = make<DynamicWriter>();
    method.Delegate(argReader, resultWriter);
    return get_self<DynamicWriter>(resultWriter)->TakeValue();
  });

  // By default the sync methods are run in JS dispatcher.
  // If we want to run them in a different dispatcher, then we must run it synchronously blocking the JS thread.
  if (m_moduleDispatcher && !method.UseJSDispatcher) {
    cxxMethodCallback = SyncFuncType([dispatcher = m_moduleDispatcher,
                                      cxxMethodCallback = std::move(cxxMethodCallback)](folly::dynamic args) noexcept {
      folly::dynamic result;
      RunSync(
          dispatcher, [cxxMethodCallback, &result, &args]() mutable { result = cxxMethodCallback(std::move(args)); });
      return result;
    });
  }

  return CxxMethod{name, std::move(cxxMethodCallback), CxxModule::SyncTag};
}

/*static*/ void ABICxxModule::RunSync(
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

/*static*/ MethodResultCallback ABICxxModule::MakeMethodResultCallback(CxxModule::Callback &&callback) noexcept {
  return [callback = std::move(callback)](const IJSValueWriter &outputWriter) noexcept {
    if (callback) {
      if (outputWriter) {
        folly::dynamic argArray = outputWriter.as<DynamicWriter>()->TakeValue();
        callback(std::vector<folly::dynamic>(argArray.begin(), argArray.end()));

      } else {
        callback(std::vector<folly::dynamic>{});
      }
    }
  };
}

} // namespace winrt::Microsoft::ReactNative

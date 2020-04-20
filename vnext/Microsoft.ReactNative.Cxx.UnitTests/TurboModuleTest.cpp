// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "ReactModuleBuilderMock.h"

#include <sstream>
#include "NativeModules.h"
#include "future/futureWait.h"

namespace ReactNativeTests {
REACT_MODULE(MyTurboModule)
struct MyTurboModule {
  // REACT_INIT(Initialize)
  // void Initialize(React::IReactContext const &context) noexcept {
  //  IsInitialized = true;
  //  TestCheck(context != nullptr);

  //  // Event and Function fields are initialized before REACT_INIT method call.
  //  TestCheck(this->OnIntEvent != nullptr);
  //  TestCheck(this->JSIntFunction != nullptr);
  //}

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
#if 0
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

  REACT_METHOD(NegateAsyncCallback)
  fire_and_forget NegateAsyncCallback(int x, std::function<void(int)> resolve) noexcept {
    co_await winrt::resume_background();
    resolve(-x);
  }

  REACT_METHOD(NegateDispatchQueueCallback)
  void NegateDispatchQueueCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve]() noexcept { resolve(-x); });
  }

  REACT_METHOD(NegateFutureCallback)
  void NegateFutureCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::PostFuture([x, resolve]() noexcept { resolve(-x); });
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

  REACT_METHOD(StaticNegateAsyncCallback)
  static fire_and_forget StaticNegateAsyncCallback(int x, std::function<void(int)> resolve) noexcept {
    co_await winrt::resume_background();
    resolve(-x);
  }

  REACT_METHOD(StaticNegateDispatchQueueCallback)
  static void StaticNegateDispatchQueueCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve]() noexcept { resolve(-x); });
  }

  REACT_METHOD(StaticNegateFutureCallback)
  static void StaticNegateFutureCallback(int x, std::function<void(int)> const &resolve) noexcept {
    Mso::PostFuture([x, resolve]() noexcept { resolve(-x); });
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
      std::function<void(std::string const &)> const &reject) noexcept {
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
      std::function<void(std::string const &)> const &reject) noexcept {
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(NegateAsyncCallbacks)
  fire_and_forget NegateAsyncCallbacks(
      int x,
      std::function<void(int)> resolve,
      std::function<void(std::string const &)> reject) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(NegateDispatchQueueCallbacks)
  void NegateDispatchQueueCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(NegateFutureCallbacks)
  void NegateFutureCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::PostFuture([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(ResolveSayHelloCallbacks)
  void ResolveSayHelloCallbacks(
      std::function<void(std::string const &)> const &resolve,
      std::function<void(std::string const &)> const & /*reject*/) noexcept {
    resolve("Hello_3");
  }

  REACT_METHOD(RejectSayHelloCallbacks)
  void RejectSayHelloCallbacks(
      std::function<void(std::string const &)> const & /*resolve*/,
      std::function<void(std::string const &)> const &reject) noexcept {
    reject("Goodbye");
  }

  REACT_METHOD(StaticDivideCallbacks)
  static void StaticDivideCallbacks(
      int x,
      int y,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
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
      std::function<void(std::string const &)> const &reject) noexcept {
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(StaticNegateAsyncCallbacks)
  static fire_and_forget StaticNegateAsyncCallbacks(
      int x,
      std::function<void(int)> resolve,
      std::function<void(std::string const &)> reject) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      resolve(-x);
    } else {
      reject("Already negative");
    }
  }

  REACT_METHOD(StaticNegateDispatchQueueCallbacks)
  static void StaticNegateDispatchQueueCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(StaticNegateFutureCallbacks)
  static void StaticNegateFutureCallbacks(
      int x,
      std::function<void(int)> const &resolve,
      std::function<void(std::string const &)> const &reject) noexcept {
    Mso::PostFuture([x, resolve, reject]() noexcept {
      if (x >= 0) {
        resolve(-x);
      } else {
        reject("Already negative");
      }
    });
  }

  REACT_METHOD(StaticResolveSayHelloCallbacks)
  static void StaticResolveSayHelloCallbacks(
      std::function<void(std::string const &)> const &resolve,
      std::function<void(std::string const &)> const & /*reject*/) noexcept {
    resolve("Hello_3");
  }

  REACT_METHOD(StaticRejectSayHelloCallbacks)
  static void StaticRejectSayHelloCallbacks(
      std::function<void(std::string const &)> const & /*resolve*/,
      std::function<void(std::string const &)> const &reject) noexcept {
    reject("Goodbye");
  }

  REACT_METHOD(DividePromise)
  void DividePromise(int x, int y, React::ReactPromise<int> const &result) noexcept {
    if (y != 0) {
      result.Resolve(x / y);
    } else {
      React::ReactError error{};
      error.Message = "Division by 0";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(NegatePromise)
  void NegatePromise(int x, React::ReactPromise<int> const &result) noexcept {
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(NegateAsyncPromise)
  fire_and_forget NegateAsyncPromise(int x, React::ReactPromise<int> result) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(NegateDispatchQueuePromise)
  void NegateDispatchQueuePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  REACT_METHOD(NegateFuturePromise)
  void NegateFuturePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::PostFuture([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  // Each macro has second optional parameter: JS name.
  REACT_METHOD(VoidPromise, L"voidPromise")
  void VoidPromise(int x, React::ReactPromise<void> const &result) noexcept {
    if (x % 2 == 0) {
      result.Resolve();
    } else {
      result.Reject("Odd unexpected");
    }
  }

  REACT_METHOD(ResolveSayHelloPromise)
  void ResolveSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    result.Resolve("Hello_4");
  }

  REACT_METHOD(RejectSayHelloPromise)
  void RejectSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    React::ReactError error{};
    error.Message = "Promise rejected";
    result.Reject(std::move(error));
  }

  REACT_METHOD(StaticDividePromise)
  static void StaticDividePromise(int x, int y, React::ReactPromise<int> const &result) noexcept {
    if (y != 0) {
      result.Resolve(x / y);
    } else {
      React::ReactError error{};
      error.Message = "Division by 0";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticNegatePromise)
  static void StaticNegatePromise(int x, React::ReactPromise<int> const &result) noexcept {
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticNegateAsyncPromise)
  static fire_and_forget StaticNegateAsyncPromise(int x, React::ReactPromise<int> result) noexcept {
    co_await winrt::resume_background();
    if (x >= 0) {
      result.Resolve(-x);
    } else {
      React::ReactError error{};
      error.Message = "Already negative";
      result.Reject(std::move(error));
    }
  }

  REACT_METHOD(StaticNegateDispatchQueuePromise)
  static void StaticNegateDispatchQueuePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::DispatchQueue::ConcurrentQueue().Post([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  REACT_METHOD(StaticNegateFuturePromise)
  static void StaticNegateFuturePromise(int x, React::ReactPromise<int> const &result) noexcept {
    Mso::PostFuture([x, result]() noexcept {
      if (x >= 0) {
        result.Resolve(-x);
      } else {
        React::ReactError error{};
        error.Message = "Already negative";
        result.Reject(std::move(error));
      }
    });
  }

  // Each macro has second optional parameter: JS name.
  REACT_METHOD(StaticVoidPromise, L"staticVoidPromise")
  void StaticVoidPromise(int x, React::ReactPromise<void> const &result) noexcept {
    if (x % 2 == 0) {
      result.Resolve();
    } else {
      result.Reject("Odd unexpected");
    }
  }

  REACT_METHOD(StaticResolveSayHelloPromise)
  static void StaticResolveSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    result.Resolve("Hello_4");
  }

  REACT_METHOD(StaticRejectSayHelloPromise)
  static void StaticRejectSayHelloPromise(React::ReactPromise<std::string> const &result) noexcept {
    React::ReactError error{};
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

  REACT_CONSTANT(Constant2, L"const2")
  const std::string Constant2{"MyConstant2"};

  REACT_CONSTANT(Constant3, L"const3")
  static constexpr Point Constant3{/*X =*/2, /*Y =*/3};

  REACT_CONSTANT(Constant4)
  static constexpr Point Constant4{/*X =*/3, /*Y =*/4};

  REACT_CONSTANT_PROVIDER(Constant5)
  void Constant5(React::ReactConstantProvider &provider) noexcept {
    provider.Add(L"const51", Point{/*X =*/12, /*Y =*/14});
    provider.Add(L"const52", "MyConstant52");
  }

  REACT_CONSTANT_PROVIDER(Constant6)
  static void Constant6(React::ReactConstantProvider &provider) noexcept {
    provider.Add(L"const61", Point{/*X =*/15, /*Y =*/17});
    provider.Add(L"const62", "MyConstant62");
  }

  // Allows to emit native module events
  REACT_EVENT(OnIntEvent)
  std::function<void(int)> OnIntEvent;

  // An event without arguments
  REACT_EVENT(OnNoArgEvent)
  std::function<void()> OnNoArgEvent;

  // An event with two arguments
  REACT_EVENT(OnTwoArgsEvent)
  std::function<void(Point const &, Point const &)> OnTwoArgsEvent;

  // Specify event name different from the field name.
  REACT_EVENT(OnPointEvent, L"onPointEvent")
  std::function<void(Point const &)> OnPointEvent;

  // By default we use the event emitter name from REACT_MODULE which is by default 'RCTDeviceEventEmitter'.
  // Here we specify event emitter name local for this event.
  REACT_EVENT(OnStringEvent, L"onStringEvent", L"MyEventEmitter")
  std::function<void(char const *)> OnStringEvent;

  // Use React::JSValue which is an immutable JSON-like data representation.
  REACT_EVENT(OnJSValueEvent)
  std::function<void(const React::JSValue &)> OnJSValueEvent;

  // Allows to call JS functions.
  REACT_FUNCTION(JSIntFunction)
  std::function<void(int)> JSIntFunction;

  // Specify JS function name different from the field name.
  REACT_FUNCTION(JSPointFunction, L"pointFunc")
  std::function<void(Point const &)> JSPointFunction;

  // Use two arguments. Specify JS function name different from the field name.
  REACT_FUNCTION(JSLineFunction, L"lineFunc")
  std::function<void(Point const &, Point const &)> JSLineFunction;

  // Use no arguments.
  REACT_FUNCTION(JSNoArgFunction)
  std::function<void()> JSNoArgFunction;

  // By default we use the module name from REACT_MODULE which is by default the struct name.
  // Here we specify module name local for this function.
  REACT_FUNCTION(JSStringFunction, L"stringFunc", L"MyModule")
  std::function<void(char const *)> JSStringFunction;

  // Use React::JSValue which is an immutable JSON-like data representation.
  REACT_FUNCTION(JSValueFunction)
  std::function<void(const React::JSValue &)> JSValueFunction;
#endif
 public: // Used to report some test messages
  bool IsInitialized{false};
  std::string Message;
  static std::string StaticMessage;
};

/*static*/ std::string MyTurboModule::StaticMessage;

// The TurboModule spec is going to be generated from the Flow spec file.
// It verifies that:
// - module methods names are unique;
// - method names are matching to the module spec method names;
// - method signatures match the spec method signatures.
struct MyTurboModuleSpec : winrt::Microsoft::ReactNative::TurboModuleSpec {
  static constexpr auto methods = std::tuple{
      Method<void(int, int, Callback<int>) noexcept>{0, L"Add"},
      Method<void(int, Callback<int>) noexcept>{1, L"Negate"},
      Method<void(Callback<std::string>) noexcept>{2, L"SayHello"},
      Method<void(int, int, Callback<int>) noexcept>{3, L"StaticAdd"},
      Method<void(int, Callback<int>) noexcept>{4, L"StaticNegate"},
      Method<void(Callback<std::string>) noexcept>{5, L"StaticSayHello"},
  };

  template <class TModule>
  static constexpr void ValidateModule() noexcept {
    constexpr auto methodCheckResults = CheckMethods<TModule, MyTurboModuleSpec>();

    REACT_SHOW_METHOD_SPEC_ERRORS(
        0,
        "Add",
        "    REACT_METHOD(Add) int Add(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Add) void Add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Add) winrt::fire_and_forget Add(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        1,
        "Negate",
        "    REACT_METHOD(Negate) int Negate(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Negate) void Negate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(Negate) winrt::fire_and_forget Negate(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        2,
        "SayHello",
        "    REACT_METHOD(SayHello) std::string SayHello() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello) void SayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(SayHello) winrt::fire_and_forget SayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        3,
        "StaticAdd",
        "    REACT_METHOD(StaticAdd) int StaticAdd(int, int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAdd) void StaticAdd(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticAdd) winrt::fire_and_forget StaticAdd(int, int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        4,
        "StaticNegate",
        "    REACT_METHOD(StaticNegate) int StaticNegate(int) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegate) void StaticNegate(int, ReactCallback<int>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticNegate) winrt::fire_and_forget StaticNegate(int, ReactCallback<int>) noexcept {/*implementation*/}\n");
    REACT_SHOW_METHOD_SPEC_ERRORS(
        5,
        "StaticSayHello",
        "    REACT_METHOD(StaticSayHello) std::string StaticSayHello() noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello) void StaticSayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n"
        "    REACT_METHOD(StaticSayHello) winrt::fire_and_forget StaticSayHello(ReactCallback<std::string>) noexcept {/*implementation*/}\n");
  }
};

TEST_CLASS (TurboModuleTest) {
  winrt::Microsoft::ReactNative::ReactModuleBuilderMock m_builderMock{};
  winrt::Microsoft::ReactNative::IReactModuleBuilder m_moduleBuilder;
  Windows::Foundation::IInspectable m_moduleObject{nullptr};
  MyTurboModule *m_module;

  TurboModuleTest() {
    m_moduleBuilder = winrt::make<winrt::Microsoft::ReactNative::ReactModuleBuilderImpl>(m_builderMock);
    auto provider = winrt::Microsoft::ReactNative::MakeTurboModuleProvider<MyTurboModule, MyTurboModuleSpec>();
    m_moduleObject = m_builderMock.CreateModule(provider, m_moduleBuilder);
    auto reactModule = m_moduleObject.as<winrt::Microsoft::ReactNative::IBoxedValue>();
    m_module = &winrt::Microsoft::ReactNative::BoxedValue<MyTurboModule>::GetImpl(reactModule);
  }

  TEST_METHOD(TestMethodCall_Add) {
    m_builderMock.Call1(L"Add", std::function<void(int)>([](int result) noexcept { TestCheckEqual(8, result); }), 3, 5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_Negate) {
    m_builderMock.Call1(L"Negate", std::function<void(int)>([](int result) noexcept { TestCheck(result == -3); }), 3);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_SayHello) {
    m_builderMock.Call1(L"SayHello", std::function<void(const std::string &)>([](const std::string &result) noexcept {
                          TestCheck(result == "Hello");
                        }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticAdd) {
    m_builderMock.Call1(
        L"StaticAdd", std::function<void(int)>([](int result) noexcept { TestCheck(result == 25); }), 20, 5);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticNegate) {
    m_builderMock.Call1(
        L"StaticNegate", std::function<void(int)>([](int result) noexcept { TestCheck(result == -7); }), 7);
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }

  TEST_METHOD(TestMethodCall_StaticSayHello) {
    m_builderMock.Call1(L"StaticSayHello", std::function<void(const std::string &)>([
                        ](const std::string &result) noexcept { TestCheck(result == "Hello"); }));
    TestCheck(m_builderMock.IsResolveCallbackCalled());
  }
};

} // namespace ReactNativeTests

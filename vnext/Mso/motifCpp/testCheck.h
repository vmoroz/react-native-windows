// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once
#ifndef MSO_MOTIFCPP_TESTCHECK_H
#define MSO_MOTIFCPP_TESTCHECK_H

//=============================================================================
// Test macros for better error reporting.
// They allow to report the line number and the failing expression.
// It allows to write more compact code because we do not need to add comment
// for every TestAssert.
//
// All macros have a form with suffix "L" that accept line number as a parameter.
// It is done to enable creation of custom check macros that internally
// use the macros we provide.
//=============================================================================

#include <csetjmp>
#include <csignal>
#include <string>
#include "motifCpp/assert_motifApi.h"

//=============================================================================
// Helper macros for stringifying expressions.
//=============================================================================
#ifndef MSO_TO_STR
#define MSO_INTERNAL_TO_STR(value) #value
#define MSO_TO_STR(value) MSO_INTERNAL_TO_STR(value)
#endif

//=============================================================================
// TestCheckFail fails the test unconditionally.
//=============================================================================
#define TestCheckFailAt(file, line, ...) \
  TestAssert::FailAt(                    \
      file, line, (TestAssert::FormatMsg("Line: %d", line) + TestAssert::FormatMsg("\n" __VA_ARGS__)).c_str())
#define TestCheckFail(...) TestCheckFailAt(__FILE__, __LINE__, __VA_ARGS__)

//=============================================================================
// TestCheck checks if provided expression evaluates to true.
// If check fails then it reports the line number and the failed expression.
//=============================================================================
#define TestCheckAt(file, line, expr, ...) \
  TestAssert::IsTrueAt(                    \
      file,                                \
      line,                                \
      expr,                                \
      #expr,                               \
      (TestAssert::FormatMsg("Line: %d", line) + TestAssert::FormatMsg(" [ " #expr " ]\n" __VA_ARGS__)).c_str())
#define TestCheck(expr, ...) TestCheckAt(__FILE__, __LINE__, expr, __VA_ARGS__)

//=============================================================================
// TestCheckEqual checks if two provided values are equal.
// If check fails then it reports the line number and the failed expression.
// In addition the TestAssert::AreEqual reports expected and actual values.
//=============================================================================
#define TestCheckEqualAt(file, line, expected, actual, ...)                      \
  TestAssert::AreEqualAt(                                                        \
      file,                                                                      \
      line,                                                                      \
      expected,                                                                  \
      actual,                                                                    \
      #expected,                                                                 \
      #actual,                                                                   \
      (TestAssert::FormatMsg("Line: %d", line) +                                 \
       TestAssert::FormatMsg(" [ " #expected " == " #actual " ]\n" __VA_ARGS__)) \
          .c_str())
#define TestCheckEqual(expected, actual, ...) TestCheckEqualAt(__FILE__, __LINE__, expected, actual, __VA_ARGS__)

//=============================================================================
// TestCheckIgnore ignores the provided expression.
// It can be used to avoid compilation errors related to unused variables.
// It evaluates the expression, but ignores its result.
//=============================================================================
#define TestCheckIgnore(expr) (void)expr

//=============================================================================
// TestCheckCrash expects that the provided expression causes a crash.
//=============================================================================
#define TestCheckCrashAt(file, line, expr, ...)                                                                     \
  TestAssert::ExpectCrashAt(                                                                                        \
      file,                                                                                                         \
      line,                                                                                                         \
      [&]() {                                                                                                       \
        OACR_POSSIBLE_THROW;                                                                                        \
        expr;                                                                                                       \
      },                                                                                                            \
      (TestAssert::FormatMsg("Line: %d", line) + TestAssert::FormatMsg(" Must crash: [ " #expr " ]\n" __VA_ARGS__)) \
          .c_str())
#define TestCheckCrash(expr, ...) TestCheckCrashAt(__FILE__, __LINE__, expr, __VA_ARGS__)

//=============================================================================
// TestCheckTerminate expects that the provided expression causes process termination
// with a call to std::terminate().
// It is useful to check when a noexcept function throws, or std::terminate is called directly.
// The implementation uses setjmp and longjmp which stops the termination sequence.
// In case if TestCheckTerminate succeeds we may end up with leaked memory because
// the call stack is not unwinded correctly.
// You should disable memory leak detection in tests that use TestCheckTerminate.
//=============================================================================
#define TestCheckTerminateAt(file, line, expr, ...)                           \
  TestAssert::ExpectTerminateAt(                                              \
      file,                                                                   \
      line,                                                                   \
      [&]() { expr; },                                                        \
      (TestAssert::FormatMsg("Line: %d", line) +                              \
       TestAssert::FormatMsg(" Must terminate: [ " #expr " ]\n" __VA_ARGS__)) \
          .c_str())
#define TestCheckTerminate(expr, ...) TestCheckTerminateAt(__FILE__, __LINE__, expr, __VA_ARGS__)

//=============================================================================
// TestCheckException expects that the provided expression throws an exception.
//=============================================================================
#define TestCheckExceptionAt(file, line, ex, expr, ...) \
  TestAssert::ExpectExceptionAt<ex>(                    \
      file, line, [&]() { expr; }, #expr, #ex, TestAssert::FormatMsg("" __VA_ARGS__).c_str())
#define TestCheckException(ex, expr, ...) TestCheckExceptionAt(__FILE__, __LINE__, ex, expr, __VA_ARGS__)

//=============================================================================
// TestCheckNoThrow expects that the provided expression does not throw an exception.
//=============================================================================
#define TestCheckNoThrowAt(file, line, expr, ...)                             \
  TestAssert::ExpectNoThrowAt(                                                \
      file,                                                                   \
      line,                                                                   \
      [&]() { expr; },                                                        \
      (TestAssert::FormatMsg("Line: %d", line) +                              \
       TestAssert::FormatMsg(" Must not throw: [ " #expr " ]\n" __VA_ARGS__)) \
          .c_str())
#define TestCheckNoThrow(expr, ...) TestCheckNoThrowAt(__FILE__, __LINE__, expr, __VA_ARGS__)

//=============================================================================
// TestCheckAssert checks for the code to produce assert with specified tag.
//=============================================================================
#define TestCheckShipAssert(tag, expr) Statement(Mso::ExpectShipAssert expectAssert(tag); expr;);

//=============================================================================
// A macro to disable memory leak detection in unit tests where we expect crash
// or terminate. These tests cannot do proper stack unwinding and thus are
// going to leak memory. In order to use this macro you need to turn on
// memory leak detection per test method instead of the default per-class.
// It is done by adding the following member to the test class:
// MemoryLeakDetectionHook::TrackPerTest m_trackLeakPerTest;
//=============================================================================
#define TEST_DISABLE_MEMORY_LEAK_DETECTION() \
  //StopTrackingMemoryAllocations(); \
	//auto restartTrackingMemoryAllocations = Mso::TCleanup::Make([&]() noexcept \
	//{ \
	//	StartTrackingMemoryAllocations(); \
	//});

//=============================================================================
// Helper functions to implement TestCheckTerminate.
//=============================================================================
namespace TestAssert {

struct TerminateHandlerRestorer {
  ~TerminateHandlerRestorer() noexcept {
    std::set_terminate(Handler);
  }

  std::terminate_handler Handler;
};

#pragma warning(push)
#pragma warning(disable : 4611) // interaction between '_setjmp' and C++ object destruction is non-portable
template <class Fn>
inline bool ExpectTerminateCore(const Fn &fn) {
  static jmp_buf buf;

  // Set a terminate handler and save the previous terminate handler to be restored in the end of function.
  TerminateHandlerRestorer terminateRestore = {std::set_terminate([]() { longjmp(buf, 1); })};

  // setjmp originally returns 0, and when longjmp is called it returns 1.
  if (!setjmp(buf)) {
    fn();
    return false; // must not be executed if fn() caused termination and the longjmp is executed.
  } else {
    return true; // executed if longjmp is executed in the terminate handler.
  }
}
#pragma warning(pop)

template <class Fn>
inline void ExpectTerminateAt(char const *file, int line, const Fn &fn, const char *message = "") {
  if (!ExpectTerminateCore(fn)) {
    FailAt(file, line, message == nullptr || message[0] == '\0' ? "Test function did not terminate!" : message);
  }
}

} // namespace TestAssert

#endif // MSO_MOTIFCPP_TESTCHECK_H

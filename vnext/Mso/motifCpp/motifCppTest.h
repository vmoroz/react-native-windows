// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once
#ifndef MSO_MOTIFCPP_MOTIFCPPTEST_H
#define MSO_MOTIFCPP_MOTIFCPPTEST_H

#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <functional>
#include <type_traits>
#include "comUtil/IUnknownShim.h"
#include "motifCpp/gTestAdapter.h"
#include "oacr/oacr.h"

typedef wchar_t WCHAR;

namespace TestAssert {

#define GTEST_ASSERT_AT_(file, line, expression, on_failure)    \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                 \
  if (const ::testing::AssertionResult gtest_ar = (expression)) \
    ;                                                           \
  else                                                          \
    on_failure(file, line, gtest_ar.failure_message())

#define GTEST_TEST_BOOLEAN_AT_(file, line, expression, text, actual, expected, fail)       \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                                            \
  if (const ::testing::AssertionResult gtest_ar_ = ::testing::AssertionResult(expression)) \
    ;                                                                                      \
  else                                                                                     \
    fail(file, line, ::testing::internal::GetBoolAssertionFailureMessage(gtest_ar_, text, #actual, #expected).c_str())

#define GTEST_FATAL_FAILURE_AT_(file, line, message) \
  return GTEST_MESSAGE_AT_(file, line, message, ::testing::TestPartResult::kFatalFailure)

inline void IsTrueAt(char const *file, int line, bool condition, char const *expr, _In_z_ const WCHAR *message = L"") {
  GTEST_TEST_BOOLEAN_AT_(file, line, condition, expr, false, true, GTEST_FATAL_FAILURE_AT_) << message;
}

template <typename T, typename TEnable = typename std::enable_if<!std::is_same<bool, T>::value>::type>
inline void IsTrueAt(char const *file, int line, bool condition, char const *expr, _In_z_ const WCHAR *message = L"") {
  return IsTrueAt(file, line, !!condition, expr, message);
}

template <typename ExpectedType, typename ActualType>
inline void AreEqualAt(
    char const *file,
    int line,
    _In_ const ExpectedType &expected,
    _In_ const ActualType &actual,
    char const *expectedExpr,
    char const *actualExpr,
    _In_z_ const WCHAR *message = L"") {
  GTEST_ASSERT_AT_(
      file,
      line,
      ::testing::internal::EqHelper<GTEST_IS_NULL_LITERAL_(expected)>::Compare(
          expectedExpr, actualExpr, expected, actual),
      GTEST_FATAL_FAILURE_AT_)
      << message;
}

template <
    typename ExpectedType,
    typename ActualType,
    typename TEnable = typename std::enable_if<
        std::is_same<ExpectedType, ActualType>::value && !std::is_same<ExpectedType, wchar_t>::value &&
        !std::is_same<ExpectedType, char>::value>::type>
void AreEqualAt(
    char const *file,
    int line,
    _In_ const ExpectedType *expected,
    _In_ const ActualType *actual,
    char const *expectedExpr,
    char const *actualExpr,
    _In_z_ const WCHAR *message = L"") {
  GTEST_ASSERT_AT_(
      file,
      line,
      ::testing::internal::EqHelper<GTEST_IS_NULL_LITERAL_(expected)>::Compare(
          expectedExpr, actualExpr, expected, actual),
      GTEST_FATAL_FAILURE_AT_)
      << message;
}

inline void AreEqualAt(
    char const *file,
    int line,
    _In_ const wchar_t *expected,
    _In_ const wchar_t *actual,
    char const *expectedExpr,
    char const *actualExpr,
    _In_z_ const WCHAR *message = L"") {
  GTEST_ASSERT_AT_(
      file,
      line,
      ::testing::internal::CmpHelperSTREQ(expectedExpr, actualExpr, expected, actual),
      GTEST_FATAL_FAILURE_AT_)
      << message;
}

inline void AreEqualAt(
    char const *file,
    int line,
    _In_ const char *expected,
    _In_ const char *actual,
    char const *expectedExpr,
    char const *actualExpr,
    _In_z_ const WCHAR *message = L"") {
  GTEST_ASSERT_AT_(
      file,
      line,
      ::testing::internal::CmpHelperSTREQ(expectedExpr, actualExpr, expected, actual),
      GTEST_FATAL_FAILURE_AT_)
      << message;
}

inline void AreEqualAt(
    char const *file,
    int line,
    _In_ wchar_t *expected,
    _In_ const wchar_t *actual,
    char const *expectedExpr,
    char const *actualExpr,
    _In_z_ const WCHAR *message = L"") {
  GTEST_ASSERT_AT_(
      file,
      line,
      ::testing::internal::CmpHelperSTREQ(expectedExpr, actualExpr, expected, actual),
      GTEST_FATAL_FAILURE_AT_)
      << message;
}

inline void AreEqualAt(
    char const *file,
    int line,
    _In_ char *expected,
    _In_ const char *actual,
    char const *expectedExpr,
    char const *actualExpr,
    _In_z_ const WCHAR *message = L"") {
  GTEST_ASSERT_AT_(
      file,
      line,
      ::testing::internal::CmpHelperSTREQ(expectedExpr, actualExpr, expected, actual),
      GTEST_FATAL_FAILURE_AT_)
      << message;
}

// Asserts that the specified condition is true, if it is false the unit test will fail
inline void IsTrue(bool condition, _In_z_ const WCHAR *message = L"") {
  ASSERT_TRUE(condition) << message;
}

// Verify that a condition is true:
template <typename T, typename TEnable = typename std::enable_if<!std::is_same<bool, T>::value>::type>
inline void IsTrue(const T &condition, _In_z_ const WCHAR *message = L"") {
  return IsTrue(!!condition, message);
}

// Asserts that the specified condition is false, if it is true the unit test will fail
inline void IsFalse(bool condition, const WCHAR *message = L"") {
  ASSERT_FALSE(condition) << message;
}

template <typename T, typename TEnable = typename std::enable_if<!std::is_same<bool, T>::value>::type>
inline void IsFalse(const T &condition, _In_z_ const WCHAR *message = L"") {
  return IsFalse(!!condition, message);
}

template <typename ExpectedType, typename ActualType>
inline void
AreEqual(_In_ const ExpectedType &expected, _In_ const ActualType &actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_EQ(expected, actual) << wstrMessage.c_str();
}

template <
    typename ExpectedType,
    typename ActualType,
    typename TEnable = typename std::enable_if<
        std::is_same<ExpectedType, ActualType>::value && !std::is_same<ExpectedType, wchar_t>::value &&
        !std::is_same<ExpectedType, char>::value>::type>
void AreEqual(_In_ const ExpectedType *expected, _In_ const ActualType *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_EQ(expected, actual) << wstrMessage.c_str();
}

inline void AreEqual(_In_ const wchar_t *expected, _In_ const wchar_t *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STREQ(expected, actual) << wstrMessage.c_str();
}

inline void AreEqual(_In_ const char *expected, _In_ const char *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STREQ(expected, actual) << wstrMessage.c_str();
}

inline void AreEqual(_In_ wchar_t *expected, _In_ const wchar_t *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STREQ(expected, actual) << wstrMessage.c_str();
}

inline void AreEqual(_In_ char *expected, _In_ const char *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STREQ(expected, actual) << wstrMessage.c_str();
}

template <typename ExpectedType, typename ActualType>
inline void
AreNotEqual(_In_ const ExpectedType &expected, _In_ const ActualType &actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_NE(expected, actual) << wstrMessage.c_str();
}

template <
    typename ExpectedType,
    typename ActualType,
    typename TEnable = typename std::enable_if<
        std::is_same<ExpectedType, ActualType>::value && !std::is_same<ExpectedType, wchar_t>::value &&
        !std::is_same<ExpectedType, char>::value>::type>
void AreNotEqual(_In_ const ExpectedType *expected, _In_ const ActualType *actual, _In_z_ const WCHAR *message = L"") {
  AreNotEqual(*expected, *actual, message);
}

inline void AreNotEqual(_In_ const wchar_t *expected, _In_ const wchar_t *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STRNE(expected, actual) << wstrMessage.c_str();
}

inline void AreNotEqual(_In_ const char *expected, _In_ const char *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STRNE(expected, actual) << wstrMessage.c_str();
}

inline void AreNotEqual(_In_ wchar_t *expected, _In_ const wchar_t *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STRNE(expected, actual) << wstrMessage.c_str();
}

inline void AreNotEqual(_In_ char *expected, _In_ const char *actual, _In_z_ const WCHAR *message = L"") {
  std::wstring wstrMessage(message);
  ASSERT_STRNE(expected, actual) << wstrMessage.c_str();
}

inline void Comment(_In_ const WCHAR *message) {
  std::wcerr << L"[ INFO     ] " << message << std::endl;
}

inline void CommentEx(_In_z_ _Printf_format_string_ const WCHAR *format, va_list args) noexcept {
  fwprintf(stderr, L"[ INFO     ] ");
  vfwprintf(stderr, format, args);
  fwprintf(stderr, L"\n");
}

inline void CommentEx(_In_z_ _Printf_format_string_ const WCHAR *format, ...) noexcept {
  va_list args;
  va_start(args, format);
  fwprintf(stderr, L"[ INFO     ] ");
  vfwprintf(stderr, format, args);
  va_end(args);
  fwprintf(stderr, L"\n");
}

inline void Pass(_In_z_ const WCHAR *message = L"") noexcept {
  ASSERT_TRUE(true) << message;
}

inline void Fail(_In_z_ const WCHAR *message = L"") {
  ASSERT_TRUE(false) << message;
}

template <typename ValueType>
inline void IsNull(const ValueType &ptr, _In_z_ const WCHAR *message = L"") {
  AreEqual(ptr, nullptr, message);
}

template <typename ValueType>
inline void IsNotNull(const ValueType &ptr, _In_z_ const WCHAR *message = L"") {
  AreNotEqual(ptr, nullptr, message);
}

template <typename ExceptionType>
inline void ExpectException(const std::function<void()> &statement, const WCHAR *message = L"") {
  EXPECT_THROW(statement(), ExceptionType) << message;
}

template <typename ExceptionType>
inline void ExpectException(
    const std::function<void()> &statement,
    const std::function<void()> &onException,
    const WCHAR *message = L"") {
  EXPECT_THROW(statement(), ExceptionType) << message;
}

inline void ExpectNoThrow(const std::function<void()> &statement, const WCHAR *message = L"") {
  EXPECT_NO_THROW(statement()) << message;
}

inline void HrSucceeded(HRESULT hr, _In_z_ const WCHAR *message = L"") {
  ASSERT_TRUE(SUCCEEDED(hr)) << message;
}

inline void HrFailed(HRESULT hr, _In_z_ const WCHAR *message = L"") {
  ASSERT_FALSE(SUCCEEDED(hr)) << message;
}

// Code used for all the C++ exceptions
static const DWORD EXCEPTION_CPLUSPLUS = static_cast<DWORD>(0xE06D7363);

inline DWORD FilterCrashExceptions(DWORD exceptionCode) noexcept {
  if ((exceptionCode == EXCEPTION_BREAKPOINT) // allow exceptions to get to the debugger
      || (exceptionCode == EXCEPTION_SINGLE_STEP) // allow exceptions to get to the debugger
      || (exceptionCode == EXCEPTION_GUARD_PAGE) // allow to crash on memory page access violation
      || (exceptionCode == EXCEPTION_STACK_OVERFLOW)) // allow to crash on stack overflow
  {
    return EXCEPTION_CONTINUE_SEARCH;
  }
  if (exceptionCode == EXCEPTION_CPLUSPLUS) // log C++ exception and pass it through
  {
    TestAssert::Fail(L"Test function did not crash, but exception is thrown!");
    return EXCEPTION_CONTINUE_SEARCH;
  }
  return EXCEPTION_EXECUTE_HANDLER;
}

template <typename Fn>
inline bool ExpectCrashCore(const Fn &fn, const WCHAR * /*message*/) {
  __try {
    fn();
  } __except (FilterCrashExceptions(GetExceptionCode())) {
    // Pass(message == nullptr || message[0] == L'\0' ? L"Crash occurred as expected." : message);
    return true;
  }
  // Fail(message == nullptr || message[0] == L'\0' ? L"Test function did not crash!" : message);
  return false;
}

template <typename Fn>
inline void ExpectVEC(const Fn &fn, const WCHAR *message = L"") {
  if (!ExpectCrashCore(fn, message)) {
    Fail(message);
  }
}

} // namespace TestAssert

#endif // MSO_MOTIFCPP_MOTIFCPPTEST_H

// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#define NAPI_EXPERIMENTAL
#include "NapiTests.h"
#include <limits>

// Check condition and crash process if it fails.
#define CHECK_ELSE_CRASH(condition, message)               \
  do {                                                     \
    if (!(condition)) {                                    \
      assert(false && "Failed: " #condition && (message)); \
      *((int *)0) = 1;                                     \
    }                                                      \
  } while (false)

#define EXPECT_NAPI_OK(expr)                         \
  do {                                               \
    napi_status temp_status_ = (expr);               \
    if (temp_status_ != napi_status::napi_ok) {      \
      AssertNapiException(env, temp_status_, #expr); \
    }                                                \
  } while (false)

namespace napi {

void AssertNapiException(napi_env env, napi_status errorCode, const char *exprStr) {
  napi_value jsError{};
  CHECK_ELSE_CRASH(napi_get_and_clear_last_exception(env, &jsError) == napi_ok, "Cannot retrieve JS exception.");
  FAIL() << exprStr << "\n error code: " << errorCode;
}

napi_value NapiTestBase::Eval(const char *code) {
  napi_value result{}, global{}, func{}, undefined{}, codeStr{};
  EXPECT_NAPI_OK(napi_get_global(env, &global));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "eval", &func));
  EXPECT_NAPI_OK(napi_get_undefined(env, &undefined));
  EXPECT_NAPI_OK(napi_create_string_utf8(env, code, NAPI_AUTO_LENGTH, &codeStr));
  EXPECT_NAPI_OK(napi_call_function(env, undefined, func, 1, &codeStr, &result));
  return result;
}

napi_value NapiTestBase::Function(const std::string &code) {
  return Eval(("(" + code + ")").c_str());
}

napi_value NapiTestBase::CallFunction(std::initializer_list<napi_value> args, const std::string &code) {
  napi_value result{}, undefined{}, booleanResult{};
  napi_value func = Function(code);
  EXPECT_NAPI_OK(napi_get_undefined(env, &undefined));
  EXPECT_NAPI_OK(napi_call_function(env, undefined, func, args.size(), args.begin(), &result));
  return result;
}

bool NapiTestBase::CallBoolFunction(std::initializer_list<napi_value> args, const std::string &code) {
  bool result{};
  napi_value booleanResult = CallFunction(args, code);
  EXPECT_NAPI_OK(napi_get_value_bool(env, booleanResult, &result));
  return result;
}

bool NapiTestBase::CheckEqual(napi_value value, const std::string &jsValue) {
  return CallBoolFunction({value}, "function(value) { return value == " + jsValue + "; }");
}

bool NapiTestBase::CheckStrictEqual(napi_value value, const std::string &jsValue) {
  return CallBoolFunction({value}, "function(value) { return value === " + jsValue + "; }");
}

} // namespace napi

using namespace napi;

struct NapiTest : NapiTestBase {};

TEST_P(NapiTest, RunScriptTest) {
  napi_value script{}, scriptResult{}, global{}, xValue{};
  int intValue{};
  EXPECT_NAPI_OK(napi_create_string_utf8(env, "1", NAPI_AUTO_LENGTH, &script));
  EXPECT_NAPI_OK(napi_run_script(env, script, &scriptResult));
  EXPECT_NAPI_OK(napi_get_value_int32(env, scriptResult, &intValue));
  EXPECT_EQ(intValue, 1);

  EXPECT_NAPI_OK(napi_create_string_utf8(env, "x = 1", NAPI_AUTO_LENGTH, &script));
  EXPECT_NAPI_OK(napi_run_script(env, script, &scriptResult));
  EXPECT_NAPI_OK(napi_get_global(env, &global));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "x", &xValue));
  EXPECT_NAPI_OK(napi_get_value_int32(env, xValue, &intValue));
  EXPECT_EQ(intValue, 1);
}

TEST_P(NapiTest, StringTest) {
  auto TestLatin1 = [&](napi_value value) {
    char buffer[128];
    size_t bufferSize = 128;
    size_t copied{};
    napi_value result{};

    EXPECT_NAPI_OK(napi_get_value_string_latin1(env, value, buffer, bufferSize, &copied));
    EXPECT_NAPI_OK(napi_create_string_latin1(env, buffer, copied, &result));

    return result;
  };

  auto TestUtf8 = [&](napi_value value) {
    char buffer[128];
    size_t bufferSize = 128;
    size_t copied{};
    napi_value result{};

    EXPECT_NAPI_OK(napi_get_value_string_utf8(env, value, buffer, bufferSize, &copied));
    EXPECT_NAPI_OK(napi_create_string_utf8(env, buffer, copied, &result));

    return result;
  };

  auto TestUtf16 = [&](napi_value value) {
    char16_t buffer[128];
    size_t bufferSize = 128;
    size_t copied{};
    napi_value result{};

    EXPECT_NAPI_OK(napi_get_value_string_utf16(env, value, buffer, bufferSize, &copied));
    EXPECT_NAPI_OK(napi_create_string_utf16(env, buffer, copied, &result));

    return result;
  };

  auto TestLatin1Insufficient = [&](napi_value value) {
    char buffer[4];
    size_t bufferSize = 4;
    size_t copied{};
    napi_value result{};

    EXPECT_NAPI_OK(napi_get_value_string_latin1(env, value, buffer, bufferSize, &copied));
    EXPECT_NAPI_OK(napi_create_string_latin1(env, buffer, copied, &result));

    return result;
  };

  auto TestUtf8Insufficient = [&](napi_value value) {
    char buffer[4];
    size_t bufferSize = 4;
    size_t copied{};
    napi_value result{};

    EXPECT_NAPI_OK(napi_get_value_string_utf8(env, value, buffer, bufferSize, &copied));
    EXPECT_NAPI_OK(napi_create_string_utf8(env, buffer, copied, &result));

    return result;
  };

  auto TestUtf16Insufficient = [&](napi_value value) {
    char16_t buffer[4];
    size_t bufferSize = 4;
    size_t copied{};
    napi_value result{};

    EXPECT_NAPI_OK(napi_get_value_string_utf16(env, value, buffer, bufferSize, &copied));
    EXPECT_NAPI_OK(napi_create_string_utf16(env, buffer, copied, &result));
    return result;
  };

  auto Utf16Length = [&](napi_value value) {
    size_t length{};
    napi_value result{};
    EXPECT_NAPI_OK(napi_get_value_string_utf16(env, value, nullptr, 0, &length));
    EXPECT_NAPI_OK(napi_create_uint32(env, (uint32_t)length, &result));
    return result;
  };

  auto Utf8Length = [&](napi_value value) {
    size_t length{};
    napi_value result{};
    EXPECT_NAPI_OK(napi_get_value_string_utf8(env, value, nullptr, 0, &length));
    EXPECT_NAPI_OK(napi_create_uint32(env, (uint32_t)length, &result));
    return result;
  };

  Eval(R"(
    empty = '';
    str1 = 'hello world';
    str2 = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789';
    str3 = '?!@#$%^&*()_+-=[]{}/.,<>\'"\\';
    str4 = '¡¢£¤¥¦§¨©ª«¬­®¯°±²³´µ¶·¸¹º»¼½¾¿';
    str5 = 'ÀÁÂÃÄÅÆÇÈÉÊËÌÍÎÏÐÑÒÓÔÕÖ×ØÙÚÛÜÝÞßàáâãäåæçèéêëìíîïðñòóôõö÷øùúûüýþ';
    str6 = '\u{2003}\u{2101}\u{2001}\u{202}\u{2011}';
  )");

  napi_value global{}, empty{}, str1{}, str2{}, str3{}, str4{}, str5{}, str6{};
  EXPECT_NAPI_OK(napi_get_global(env, &global));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "empty", &empty));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "str1", &str1));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "str2", &str2));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "str3", &str3));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "str4", &str4));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "str5", &str5));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "str6", &str6));

  EXPECT_TRUE(CheckStrictEqual(TestLatin1(empty), "empty"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8(empty), "empty"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16(empty), "empty"));
  EXPECT_TRUE(CheckStrictEqual(Utf16Length(empty), "0"));
  EXPECT_TRUE(CheckStrictEqual(Utf8Length(empty), "0"));

  EXPECT_TRUE(CheckStrictEqual(TestLatin1(str1), "str1"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8(str1), "str1"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16(str1), "str1"));
  EXPECT_TRUE(CheckStrictEqual(TestLatin1Insufficient(str1), "str1.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8Insufficient(str1), "str1.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16Insufficient(str1), "str1.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(Utf16Length(str1), "11"));
  EXPECT_TRUE(CheckStrictEqual(Utf8Length(str1), "11"));

  EXPECT_TRUE(CheckStrictEqual(TestLatin1(str2), "str2"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8(str2), "str2"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16(str2), "str2"));
  EXPECT_TRUE(CheckStrictEqual(TestLatin1Insufficient(str2), "str2.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8Insufficient(str2), "str2.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16Insufficient(str2), "str2.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(Utf16Length(str2), "62"));
  EXPECT_TRUE(CheckStrictEqual(Utf8Length(str2), "62"));

  EXPECT_TRUE(CheckStrictEqual(TestLatin1(str3), "str3"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8(str3), "str3"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16(str3), "str3"));
  EXPECT_TRUE(CheckStrictEqual(TestLatin1Insufficient(str3), "str3.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8Insufficient(str3), "str3.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16Insufficient(str3), "str3.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(Utf16Length(str3), "27"));
  EXPECT_TRUE(CheckStrictEqual(Utf8Length(str3), "27"));

  EXPECT_TRUE(CheckStrictEqual(TestLatin1(str4), "str4"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8(str4), "str4"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16(str4), "str4"));
  EXPECT_TRUE(CheckStrictEqual(TestLatin1Insufficient(str4), "str4.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8Insufficient(str4), "str4.slice(0, 1)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16Insufficient(str4), "str4.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(Utf16Length(str4), "31"));
  EXPECT_TRUE(CheckStrictEqual(Utf8Length(str4), "62"));

  EXPECT_TRUE(CheckStrictEqual(TestLatin1(str5), "str5"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8(str5), "str5"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16(str5), "str5"));
  EXPECT_TRUE(CheckStrictEqual(TestLatin1Insufficient(str5), "str5.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8Insufficient(str5), "str5.slice(0, 1)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16Insufficient(str5), "str5.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(Utf16Length(str5), "63"));
  EXPECT_TRUE(CheckStrictEqual(Utf8Length(str5), "126"));

  EXPECT_TRUE(CheckStrictEqual(TestUtf8(str6), "str6"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16(str6), "str6"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf8Insufficient(str6), "str6.slice(0, 1)"));
  EXPECT_TRUE(CheckStrictEqual(TestUtf16Insufficient(str6), "str6.slice(0, 3)"));
  EXPECT_TRUE(CheckStrictEqual(Utf16Length(str6), "5"));
  EXPECT_TRUE(CheckStrictEqual(Utf8Length(str6), "14"));
}

TEST_P(NapiTest, ArrayTest) {
  Eval(R"(
    array = [
      1,
      9,
      48,
      13493,
      9459324,
      { name: 'hello' },
      [
        'world',
        'node',
        'abi'
      ]
    ];
  )");

  napi_value undefined{}, global{}, array{}, element{}, newArray{}, arrayCtor{}, array2{}, valueFive{};
  napi_valuetype elementType{};
  bool isArray{}, hasElement{}, isDeleted{};
  uint32_t arrayLength{};

  EXPECT_NAPI_OK(napi_get_undefined(env, &undefined));
  EXPECT_NAPI_OK(napi_get_global(env, &global));
  EXPECT_NAPI_OK(napi_get_named_property(env, global, "array", &array));

  EXPECT_NAPI_OK(napi_is_array(env, array, &isArray));
  EXPECT_TRUE(isArray);

  EXPECT_NAPI_OK(napi_get_array_length(env, array, &arrayLength));
  EXPECT_EQ(arrayLength, 7u);

  EXPECT_NAPI_OK(napi_get_element(env, array, arrayLength, &element));
  EXPECT_NAPI_OK(napi_typeof(env, element, &elementType));
  EXPECT_EQ(elementType, napi_valuetype::napi_undefined);

  for (uint32_t i = 0; i < arrayLength; ++i) {
    EXPECT_NAPI_OK(napi_get_element(env, array, i, &element));
    EXPECT_NAPI_OK(napi_typeof(env, element, &elementType));
    EXPECT_NE(elementType, napi_valuetype::napi_undefined);
    EXPECT_TRUE(CheckStrictEqual(element, "array[" + std::to_string(i) + "]"));
  }

  // Clone the array.
  EXPECT_NAPI_OK(napi_create_array(env, &newArray));
  for (uint32_t i = 0; i < arrayLength; ++i) {
    EXPECT_NAPI_OK(napi_get_element(env, array, i, &element));
    EXPECT_NAPI_OK(napi_set_element(env, newArray, i, element));
  }

  // See if all elements of the new array are the same as the old one.
  EXPECT_TRUE(CallBoolFunction({newArray}, R"JS(
    function(newArray) {
      if (array.length !== newArray.length) {
        return false;
      }
      for (let i = 0; i < array.length; ++i) {
        if (array[i] !== newArray[i]) {
          return false;
        }
      }
      return true;
    })JS"));

  EXPECT_NAPI_OK(napi_has_element(env, array, 0, &hasElement));
  EXPECT_TRUE(hasElement);
  EXPECT_NAPI_OK(napi_has_element(env, array, arrayLength, &hasElement));
  EXPECT_FALSE(hasElement);

  EXPECT_NAPI_OK(napi_create_array_with_length(env, 0, &newArray));
  EXPECT_TRUE(CallBoolFunction({newArray}, "function(newArray) { return newArray instanceof Array; }"));
  EXPECT_NAPI_OK(napi_create_array_with_length(env, 1, &newArray));
  EXPECT_TRUE(CallBoolFunction({newArray}, "function(newArray) { return newArray instanceof Array; }"));
  // Check max allowed length for an array 2^32 -1
  EXPECT_NAPI_OK(napi_create_array_with_length(env, 4294967295, &newArray));
  EXPECT_TRUE(CallBoolFunction({newArray}, "function(newArray) { return newArray instanceof Array; }"));

  // Verify that array elements can be deleted.
  array2 = Eval("array2 = ['a', 'b', 'c', 'd']");
  EXPECT_TRUE(CallBoolFunction({array2}, "function(array2) { return array2.length == 4; }"));
  EXPECT_TRUE(CallBoolFunction({array2}, "function(array2) { return 2 in array2; }"));

  EXPECT_NAPI_OK(napi_delete_element(env, array2, 2, nullptr));

  EXPECT_TRUE(CallBoolFunction({array2}, "function(array2) { return array2.length == 4; }"));
  EXPECT_TRUE(CallBoolFunction({array2}, "function(array2) { return !(2 in array2); }"));

  EXPECT_NAPI_OK(napi_delete_element(env, array2, 1, &isDeleted));
  EXPECT_TRUE(isDeleted);
  EXPECT_NAPI_OK(napi_delete_element(env, array2, 1, &isDeleted));
  EXPECT_TRUE(isDeleted); // deletion succeeded as long as the element is undefined.

  EXPECT_TRUE(CallFunction({array2}, "function(array2) { Object.freeze(array2); }"));

  EXPECT_NAPI_OK(napi_delete_element(env, array2, 0, &isDeleted));
  EXPECT_FALSE(isDeleted);
  EXPECT_NAPI_OK(napi_delete_element(env, array2, 1, &isDeleted));
  EXPECT_TRUE(isDeleted); // deletion succeeded as long as the element is undefined.

  // Check when (index > int32) max(int32) + 2 = 2,147,483,650
  EXPECT_NAPI_OK(napi_create_int32(env, 5, &valueFive));
  EXPECT_NAPI_OK(napi_set_element(env, array, 2'147'483'650u, valueFive));
  EXPECT_TRUE(CheckStrictEqual(valueFive, "array[2147483650]"));

  EXPECT_NAPI_OK(napi_has_element(env, array, 2'147'483'650u, &hasElement));
  EXPECT_TRUE(hasElement);

  EXPECT_NAPI_OK(napi_get_element(env, array, 2'147'483'650u, &element));
  EXPECT_TRUE(CheckStrictEqual(element, "5"));

  EXPECT_NAPI_OK(napi_delete_element(env, array, 2'147'483'650u, &isDeleted));
  EXPECT_TRUE(isDeleted);
  EXPECT_TRUE(CheckStrictEqual(undefined, "array[2147483650]"));
}

INSTANTIATE_TEST_CASE_P(NapiEnv, NapiTest, ::testing::ValuesIn(napiEnvGenerators()));

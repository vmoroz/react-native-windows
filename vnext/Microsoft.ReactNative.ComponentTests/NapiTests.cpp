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

#define EXPECT_NAPI_NOT_OK(expr, msg)           \
  do {                                          \
    napi_status temp_status_ = (expr);          \
    if (temp_status_ == napi_status::napi_ok) { \
      FAIL() << msg << " " << #expr;            \
    } else {                                    \
      ClearNapiException(env);                  \
    }                                           \
  } while (false)

#define EXPECT_CALL_TRUE(args, jsExpr)                              \
  do {                                                              \
    std::string argsStr = #args;                                    \
    std::replace(argsStr.begin(), argsStr.end(), '{', '(');         \
    std::replace(argsStr.begin(), argsStr.end(), '}', ')');         \
    EXPECT_TRUE(CallBoolFunction(args, argsStr + " => " + jsExpr)); \
  } while (false)

#define EXPECT_STRICT_EQ(left, right) EXPECT_TRUE(CheckStrictEqual(left, right))

#define EXPECT_DEEP_STRICT_EQ(left, right) EXPECT_TRUE(CheckDeepStrictEqual(left, right))

namespace napi {

void AssertNapiException(napi_env env, napi_status errorCode, const char *exprStr) {
  napi_value error{}, errorMessage{};
  size_t messageSize{};
  const napi_extended_error_info *extendedErrorInfo{};
  napi_get_last_error_info(env, &extendedErrorInfo);
  CHECK_ELSE_CRASH(napi_get_and_clear_last_exception(env, &error) == napi_ok, "Cannot retrieve JS exception.");
  napi_get_named_property(env, error, "message", &errorMessage);
  napi_get_value_string_utf8(env, errorMessage, nullptr, 0, &messageSize);
  std::string messageStr(messageSize, '\0');
  napi_get_value_string_utf8(env, errorMessage, messageStr.data(), messageSize + 1, nullptr);
  FAIL() << exprStr << "\n message: " << messageStr << "\n error code: " << errorCode
         << "\n code message: " << extendedErrorInfo->error_message;
}

void ClearNapiException(napi_env env) {
  napi_value error{};
  CHECK_ELSE_CRASH(napi_get_and_clear_last_exception(env, &error) == napi_ok, "Cannot retrieve JS exception.");
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

bool NapiTestBase::CheckStrictEqual(const std::string &left, const std::string &right) {
  return CallBoolFunction({}, "function() { return " + left + " === " + right + "; }");
}

static const std::string deepEqualFunc = R"JS(function(left, right) {
    function check(left, right) {
      if (left === right) {
        return true;
      }
      if (typeof left !== typeof right) {
        return false;
      }
      if (Array.isArray(left)) {
        return Array.isArray(right) && checkArray(left, right);
      }
      if (typeof left === 'number') {
        return isNaN(left) && isNaN(right);
      }
      if (typeof left === 'object') {
        return checkObject(left, right);
      }
      return false;
    }

    function checkArray(left, right) {
      if (left.length !== right.length) {
        return false;
      }
      for (let i = 0; i < left.length; ++i) {
        if (!check(left[i], right[i])) {
          return false;
        }
      }
      return true;
    }

    function checkObject(left, right) {
      const leftNames = Object.getOwnPropertyNames(left);
      const rightNames = Object.getOwnPropertyNames(right);
      if (leftNames.length !== rightNames.length) {
        return false;
      }
      for (let i = 0; i < leftNames.length; ++i) {
        if (!check(left[leftNames[i]], right[leftNames[i]])) {
          return false;
        }
      }
      const leftSymbols = Object.getOwnPropertySymbols(left);
      const rightSymbols = Object.getOwnPropertySymbols(right);
      if (leftSymbols.length !== rightSymbols.length) {
        return false;
      }
      for (let i = 0; i < leftSymbols.length; ++i) {
        if (!check(left[leftSymbols[i]], right[leftSymbols[i]])) {
          return false;
        }
      }
      return check(Object.getPrototypeOf(left), Object.getPrototypeOf(right));
    }

    return check(left, right);
  })JS";

bool NapiTestBase::CheckDeepStrictEqual(napi_value value, const std::string &jsValue) {
  return CallBoolFunction({value}, "function(value) { return " + deepEqualFunc + "(value, " + jsValue + "); }");
}

bool NapiTestBase::CheckDeepStrictEqual(const std::string &left, const std::string &right) {
  return CallBoolFunction({}, "function() { return " + deepEqualFunc + "(" + left + ", " + right + "); }");
}

napi_value NapiTestBase::CreateInt32(int32_t value) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_create_int32(env, value, &result));
  return result;
};

napi_value NapiTestBase::CreateStringUtf8(const char *value) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &result));
  return result;
};

napi_value NapiTestBase::GetProperty(napi_value object, napi_value key) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_get_property(env, object, key, &result));
  return result;
};

napi_value NapiTestBase::GetProperty(napi_value object, const char *utf8Name) {
  return GetProperty(object, CreateStringUtf8(utf8Name));
}

napi_value NapiTestBase::GetNamedProperty(napi_value object, const char *utf8Name) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_get_named_property(env, object, utf8Name, &result));
  return result;
};

napi_value NapiTestBase::GetPropertyNames(napi_value object) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_get_property_names(env, object, &result));
  return result;
};

napi_value NapiTestBase::GetPropertySymbols(napi_value object) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_get_all_property_names(
      env, object, napi_key_include_prototypes, napi_key_skip_strings, napi_key_numbers_to_strings, &result));
  return result;
};

void NapiTestBase::SetProperty(napi_value object, napi_value key, napi_value value) {
  EXPECT_NAPI_OK(napi_set_property(env, object, key, value));
};

void NapiTestBase::SetProperty(napi_value object, const char *utf8Name, napi_value value) {
  SetProperty(object, CreateStringUtf8(utf8Name), value);
};

void NapiTestBase::SetNamedProperty(napi_value object, const char *utf8Name, napi_value value) {
  EXPECT_NAPI_OK(napi_set_named_property(env, object, utf8Name, value));
};

bool NapiTestBase::HasProperty(napi_value object, napi_value key) {
  bool result{};
  EXPECT_NAPI_OK(napi_has_property(env, object, key, &result));
  return result;
};

bool NapiTestBase::HasProperty(napi_value object, const char *utf8Name) {
  return HasProperty(object, CreateStringUtf8(utf8Name));
};

bool NapiTestBase::HasNamedProperty(napi_value object, const char *utf8Name) {
  bool result{};
  EXPECT_NAPI_OK(napi_has_named_property(env, object, utf8Name, &result));
  return result;
};

bool NapiTestBase::HasOwnProperty(napi_value object, napi_value key) {
  bool result{};
  EXPECT_NAPI_OK(napi_has_own_property(env, object, key, &result));
  return result;
};

bool NapiTestBase::HasOwnProperty(napi_value object, const char *utf8Name) {
  return HasOwnProperty(object, CreateStringUtf8(utf8Name));
};

bool NapiTestBase::DeleteProperty(napi_value object, napi_value key) {
  bool result{};
  EXPECT_NAPI_OK(napi_delete_property(env, object, key, &result));
  return result;
};

napi_value NapiTestBase::CreateObject() {
  napi_value result{};
  EXPECT_NAPI_OK(napi_create_object(env, &result));
  return result;
};

uint32_t NapiTestBase::GetArrayLength(napi_value value) {
  uint32_t result{};
  EXPECT_NAPI_OK(napi_get_array_length(env, value, &result));
  return result;
}

napi_value NapiTestBase::GetElement(napi_value value, uint32_t index) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_get_element(env, value, index, &result));
  return result;
}

double NapiTestBase::GetValueDouble(napi_value value) {
  double result{};
  EXPECT_NAPI_OK(napi_get_value_double(env, value, &result));
  return result;
}

napi_value NapiTestBase::CreateDouble(double value) {
  napi_value result{};
  EXPECT_NAPI_OK(napi_create_double(env, value, &result));
  return result;
}

} // namespace napi

// void add_returned_status(
//    napi_env env,
//    const char *key,
//    napi_value object,
//    char *expected_message,
//    napi_status expected_status,
//    napi_status actual_status) {
//  char napi_message_string[100] = "";
//  napi_value prop_value;
//
//  if (actual_status != expected_status) {
//    snprintf(napi_message_string, sizeof(napi_message_string), "Invalid status [%d]", actual_status);
//  }
//
//  NAPI_CALL_RETURN_VOID(
//      env,
//      napi_create_string_utf8(
//          env,
//          (actual_status == expected_status ? expected_message : napi_message_string),
//          NAPI_AUTO_LENGTH,
//          &prop_value));
//  NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, object, key, prop_value));
//}
//
// void add_last_status(napi_env env, const char *key, napi_value return_value) {
//  napi_value prop_value;
//  const napi_extended_error_info *p_last_error;
//  NAPI_CALL_RETURN_VOID(env, napi_get_last_error_info(env, &p_last_error));
//
//  NAPI_CALL_RETURN_VOID(
//      env,
//      napi_create_string_utf8(
//          env,
//          (p_last_error->error_message == NULL ? "napi_ok" : p_last_error->error_message),
//          NAPI_AUTO_LENGTH,
//          &prop_value));
//  NAPI_CALL_RETURN_VOID(env, napi_set_named_property(env, return_value, key, prop_value));
//}

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

TEST_P(NapiTest, SymbolTest) {
  auto New = [&](const char *value = nullptr) {
    napi_value description{}, symbol{};
    if (value) {
      EXPECT_NAPI_OK(napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &description));
    }
    EXPECT_NAPI_OK(napi_create_symbol(env, description, &symbol));
    return symbol;
  };

  const napi_value sym = New("test");
  EXPECT_TRUE(CallBoolFunction({sym}, "function(sym) { return sym.toString() === 'Symbol(test)'; }"));

  const napi_value fooSym = New("foo");
  const napi_value otherSym = New("bar");
  CallFunction({fooSym, otherSym}, R"(
    function(fooSym, otherSym) {
      myObj = {};
      myObj.foo = 'bar';
      myObj[fooSym] = 'baz';
      myObj[otherSym] = 'bing';
    })");
  EXPECT_TRUE(CallBoolFunction({}, "function() { return myObj.foo === 'bar'; }"));
  EXPECT_TRUE(CallBoolFunction({fooSym}, "function(fooSym) { return myObj[fooSym] === 'baz'; }"));
  EXPECT_TRUE(CallBoolFunction({otherSym}, "function(otherSym) { return myObj[otherSym] === 'bing'; }"));
  EXPECT_TRUE(CallBoolFunction({otherSym}, "function(otherSym) { return myObj[otherSym] === 'bing'; }"));

  const napi_value sym1 = New();
  const napi_value sym2 = New();
  EXPECT_TRUE(CallBoolFunction({sym1, sym2}, "function(sym1, sym2) { return sym1 !== sym2; }"));
  const napi_value fooSym1 = New("foo");
  const napi_value fooSym2 = New("foo");
  EXPECT_TRUE(CallBoolFunction({fooSym1, fooSym2}, "function(sym1, sym2) { return sym1 !== sym2; }"));
  const napi_value barSym = New("bar");
  EXPECT_TRUE(CallBoolFunction({fooSym1, barSym}, "function(sym1, sym2) { return sym1 !== sym2; }"));
}

TEST_P(NapiTest, ObjectTest) {
  int test_value = 3;

  auto New = [&]() {
    napi_value result = CreateObject();
    SetNamedProperty(result, "test_number", CreateInt32(987654321));
    SetNamedProperty(result, "test_string", CreateStringUtf8("test string"));
    return result;
  };

  auto Inflate = [&](napi_value obj) {
    napi_value propertyNames = GetPropertyNames(obj);
    uint32_t length = GetArrayLength(propertyNames);
    for (uint32_t i = 0; i < length; i++) {
      napi_value propertyName = GetElement(propertyNames, i);
      napi_value value = GetProperty(obj, propertyName);
      SetProperty(obj, propertyName, CreateDouble(GetValueDouble(value) + 1));
    }

    return obj;
  };

  auto Wrap = [&](napi_value obj) {
    EXPECT_NAPI_OK(napi_wrap(env, obj, &test_value, nullptr, nullptr, nullptr));
  };

  auto Unwrap = [&](napi_value obj) {
    void *data{};
    EXPECT_NAPI_OK(napi_unwrap(env, obj, &data));

    bool is_expected = (data != nullptr && *(int *)data == 3);
    return is_expected;
  };

#if 0
  auto TestSetProperty=[&]() {
    napi_status status;
    napi_value object, key, value;

    EXPECT_NAPI_OK(napi_create_object(env, &object));
    EXPECT_NAPI_OK(napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &key));
    EXPECT_NAPI_OK(napi_create_object(env, &value));

    status = napi_set_property(nullptr, object, key, value);

    add_returned_status(env, "envIsNull", object, "Invalid argument", napi_invalid_arg, status);

    napi_set_property(env, NULL, key, value);

    add_last_status(env, "objectIsNull", object);

    napi_set_property(env, object, NULL, value);

    add_last_status(env, "keyIsNull", object);

    napi_set_property(env, object, key, NULL);

    add_last_status(env, "valueIsNull", object);

    return object;
  }

  static napi_value TestHasProperty(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value object, key;
    bool result;

    NAPI_CALL(env, napi_create_object(env, &object));

    NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &key));

    status = napi_has_property(NULL, object, key, &result);

    add_returned_status(env, "envIsNull", object, "Invalid argument", napi_invalid_arg, status);

    napi_has_property(env, NULL, key, &result);

    add_last_status(env, "objectIsNull", object);

    napi_has_property(env, object, NULL, &result);

    add_last_status(env, "keyIsNull", object);

    napi_has_property(env, object, key, NULL);

    add_last_status(env, "resultIsNull", object);

    return object;
  }

  static napi_value TestGetProperty(napi_env env, napi_callback_info info) {
    napi_status status;
    napi_value object, key, result;

    NAPI_CALL(env, napi_create_object(env, &object));

    NAPI_CALL(env, napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &key));

    NAPI_CALL(env, napi_create_object(env, &result));

    status = napi_get_property(NULL, object, key, &result);

    add_returned_status(env, "envIsNull", object, "Invalid argument", napi_invalid_arg, status);

    napi_get_property(env, NULL, key, &result);

    add_last_status(env, "objectIsNull", object);

    napi_get_property(env, object, NULL, &result);

    add_last_status(env, "keyIsNull", object);

    napi_get_property(env, object, key, NULL);

    add_last_status(env, "resultIsNull", object);

    return object;
  }

  static napi_value TestFreeze(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    napi_value object = args[0];
    NAPI_CALL(env, napi_object_freeze(env, object));

    return object;
  }

  static napi_value TestSeal(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, NULL, NULL));

    napi_value object = args[0];
    NAPI_CALL(env, napi_object_seal(env, object));

    return object;
  }

  // We create two type tags. They are basically 128-bit UUIDs.
  static const napi_type_tag type_tags[2] = {
      {0xdaf987b3cc62481a, 0xb745b0497f299531}, {0xbb7936c374084d9b, 0xa9548d0762eeedb9}};

  static napi_value TypeTaggedInstance(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    uint32_t type_index;
    napi_value instance, which_type;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, &which_type, NULL, NULL));
    NAPI_CALL(env, napi_get_value_uint32(env, which_type, &type_index));
    NAPI_CALL(env, napi_create_object(env, &instance));
    NAPI_CALL(env, napi_type_tag_object(env, instance, &type_tags[type_index]));

    return instance;
  }

  auto CheckTypeTag = [&](napi_env env, napi_callback_info info) {
    size_t argc = 2;
    bool result;
    napi_value argv[2], js_result;
    uint32_t type_index;

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
    NAPI_CALL(env, napi_get_value_uint32(env, argv[0], &type_index));
    NAPI_CALL(env, napi_check_object_type_tag(env, argv[1], &type_tags[type_index], &result));
    NAPI_CALL(env, napi_get_boolean(env, result, &js_result));

    return js_result;
  }
#endif
  {
    napi_value object = Eval(R"(object = {
      hello : 'world',
      array : [ 1, 94, 'str', 12.321, {test : 'obj in arr'} ],
      newObject : {test : 'obj in obj'}
    })");

    EXPECT_TRUE(CheckStrictEqual(GetProperty(object, "hello"), "'world'"));
    EXPECT_TRUE(CheckStrictEqual(GetNamedProperty(object, "hello"), "'world'"));
    EXPECT_TRUE(CheckDeepStrictEqual(GetProperty(object, "array"), "[ 1, 94, 'str', 12.321, {test : 'obj in arr'} ]"));
    EXPECT_TRUE(CheckDeepStrictEqual(GetProperty(object, "newObject"), "{test : 'obj in obj'}"));

    EXPECT_TRUE(HasProperty(object, "hello"));
    EXPECT_TRUE(HasNamedProperty(object, "hello"));
    EXPECT_TRUE(HasProperty(object, "array"));
    EXPECT_TRUE(HasProperty(object, "newObject"));

    napi_value newObject = New();
    EXPECT_TRUE(HasProperty(newObject, "test_number"));
    EXPECT_CALL_TRUE({newObject}, "newObject.test_number === 987654321");
    EXPECT_CALL_TRUE({newObject}, "newObject.test_string === 'test string'");
  }

  {
    // Verify that napi_get_property() walks the prototype chain.
    napi_value obj = Eval(R"(
      function MyObject() {
        this.foo = 42;
        this.bar = 43;
      }

      MyObject.prototype.bar = 44;
      MyObject.prototype.baz = 45;

      obj = new MyObject();
      )");

    EXPECT_STRICT_EQ(GetProperty(obj, "foo"), "42");
    EXPECT_STRICT_EQ(GetProperty(obj, "bar"), "43");
    EXPECT_STRICT_EQ(GetProperty(obj, "baz"), "45");
    EXPECT_STRICT_EQ(GetProperty(obj, "toString"), "Object.prototype.toString");
  }

  {
    // Verify that napi_has_own_property() fails if property is not a name.
    napi_value notNames = Eval("[ true, false, null, undefined, {}, [], 0, 1, () => {} ]");
    uint32_t notNamesLength = GetArrayLength(notNames);
    for (uint32_t i = 0; i < notNamesLength; ++i) {
      EXPECT_FALSE(HasOwnProperty(CreateObject(), GetElement(notNames, i)));
    }
  }

  {
    // Verify that napi_has_own_property() does not walk the prototype chain.
    napi_value symbol1 = Eval("symbol1 = Symbol()");
    napi_value symbol2 = Eval("symbol2 = Symbol()");

    napi_value obj = Eval(R"(
      function MyObject() {
        this.foo = 42;
        this.bar = 43;
        this[symbol1] = 44;
      }

      MyObject.prototype.bar = 45;
      MyObject.prototype.baz = 46;
      MyObject.prototype[symbol2] = 47;

      obj = new MyObject();
      )");

    EXPECT_TRUE(HasOwnProperty(obj, "foo"));
    EXPECT_TRUE(HasOwnProperty(obj, "bar"));
    EXPECT_TRUE(HasOwnProperty(obj, symbol1));
    EXPECT_FALSE(HasOwnProperty(obj, "baz"));
    EXPECT_FALSE(HasOwnProperty(obj, "toString"));
    EXPECT_FALSE(HasOwnProperty(obj, symbol2));
  }

  {
    // test_object.Inflate increases all properties by 1
    napi_value cube = Eval(R"(cube = {
      x : 10,
      y : 10,
      z : 10
    })");

    EXPECT_DEEP_STRICT_EQ(cube, "{x : 10, y : 10, z : 10}");
    EXPECT_DEEP_STRICT_EQ(Inflate(cube), "{x : 11, y : 11, z : 11}");
    EXPECT_DEEP_STRICT_EQ(Inflate(cube), "{x : 12, y : 12, z : 12}");
    EXPECT_DEEP_STRICT_EQ(Inflate(cube), "{x : 13, y : 13, z : 13}");
    Eval("cube.t = 13");
    EXPECT_DEEP_STRICT_EQ(Inflate(cube), "{x : 14, y : 14, z : 14, t : 14}");

    napi_value sym1 = Eval("sym1 = Symbol('1')");
    napi_value sym2 = Eval("sym2 = Symbol('2')");
    napi_value sym3 = Eval("sym3 = Symbol('3')");
    napi_value sym4 = Eval("sym4 = Symbol('4')");
    napi_value object2 = Eval("object2 = {[sym1] : '@@iterator', [sym2] : sym3}");

    EXPECT_TRUE(HasProperty(object2, sym1));
    EXPECT_TRUE(HasProperty(object2, sym2));
    EXPECT_STRICT_EQ(GetProperty(object2, sym1), "'@@iterator'");
    EXPECT_STRICT_EQ(GetProperty(object2, sym2), "sym3");
    SetProperty(object2, "string", CreateStringUtf8("value"));
    SetNamedProperty(object2, "named_string", CreateStringUtf8("value"));
    SetProperty(object2, sym4, CreateInt32(123));
    EXPECT_TRUE(HasProperty(object2, "string"));
    EXPECT_TRUE(HasProperty(object2, "named_string"));
    EXPECT_TRUE(HasProperty(object2, sym4));
    EXPECT_STRICT_EQ(GetProperty(object2, "string"), "'value'");
    EXPECT_STRICT_EQ(GetProperty(object2, sym4), "123");
  }

  {
    // Wrap a pointer in a JS object, then verify the pointer can be unwrapped.
    napi_value wrapper = CreateObject();
    Wrap(wrapper);
    EXPECT_TRUE(Unwrap(wrapper));
  }
}

INSTANTIATE_TEST_CASE_P(NapiEnv, NapiTest, ::testing::ValuesIn(napiEnvGenerators()));

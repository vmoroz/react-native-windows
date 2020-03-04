// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "JSValue.h"
#include "JsonJSValueReader.h"

namespace winrt::Microsoft::ReactNative {

TEST_CLASS (JSValueTest) {
  TEST_METHOD(TestReadObject) {
    const wchar_t *json =
        LR"JSON({
        "NullValue": null,
        "ObjValue": {},
        "ArrayValue": [],
        "StringValue": "Hello",
        "BoolValue": true,
        "IntValue": 42,
        "DoubleValue": 4.5
      })JSON";

    IJSValueReader reader = make<JsonJSValueReader>(json);

    JSValue jsValue = JSValue::ReadFrom(reader);
    TestCheck(jsValue.Type() == JSValueType::Object);
    TestCheck(jsValue["NullValue"].IsNull());
    TestCheck(jsValue["ObjValue"].GetIfObject()->empty());
    TestCheck(jsValue["ArrayValue"].GetIfArray()->empty());
    TestCheck(jsValue["StringValue"] == "Hello");
    TestCheck(jsValue["BoolValue"] == true);
    TestCheck(jsValue["IntValue"] == 42);
    TestCheck(jsValue["DoubleValue"] == 4.5);
  } // namespace winrt::Microsoft::ReactNative

  TEST_METHOD(TestReadNestedObject) {
    const wchar_t *json =
        LR"JSON({
        "NestedObj": {
          "NullValue": null,
          "ObjValue": {},
          "ArrayValue": [],
          "StringValue": "Hello",
          "BoolValue": true,
          "IntValue": 42,
          "DoubleValue": 4.5
        }
      })JSON";

    IJSValueReader reader = make<JsonJSValueReader>(json);

    JSValue jsValue = JSValue::ReadFrom(reader);
    TestCheck(jsValue.Type() == JSValueType::Object);
    auto const *nestedObj = jsValue["NestedObj"].GetIfObject();
    TestCheck(nestedObj->at("NullValue").IsNull());
    TestCheck(nestedObj->at("ObjValue").GetIfObject()->empty());
    TestCheck(nestedObj->at("ArrayValue").GetIfArray()->empty());
    TestCheck(nestedObj->at("StringValue") == "Hello");
    TestCheck(nestedObj->at("BoolValue") == true);
    TestCheck(nestedObj->at("IntValue") == 42);
    TestCheck(nestedObj->at("DoubleValue") == 4.5);
  }

  TEST_METHOD(TestReadArray) {
    const wchar_t *json = LR"JSON([null, {}, [], "Hello", true, 42, 4.5])JSON";
    IJSValueReader reader = make<JsonJSValueReader>(json);

    JSValue jsValue = JSValue::ReadFrom(reader);
    TestCheck(jsValue.Type() == JSValueType::Array);
    TestCheck(jsValue[0].IsNull());
    TestCheck(jsValue[1].GetIfObject()->empty());
    TestCheck(jsValue[2].GetIfArray()->empty());
    TestCheck(jsValue[3] == "Hello");
    TestCheck(jsValue[4] == true);
    TestCheck(jsValue[5] == 42);
    TestCheck(jsValue[6] == 4.5);
  }

  TEST_METHOD(TestReadNestedArray) {
    const wchar_t *json = LR"JSON([[null, {}, [], "Hello", true, 42, 4.5]])JSON";
    IJSValueReader reader = make<JsonJSValueReader>(json);

    JSValue jsValue = JSValue::ReadFrom(reader);
    TestCheck(jsValue[0].GetIfArray());
    auto const &nestedArr = *jsValue[0].GetIfArray();

    TestCheck(nestedArr[0].Type() == JSValueType::Null);
    TestCheck(nestedArr[1].Type() == JSValueType::Object);
    TestCheck(nestedArr[2].Type() == JSValueType::Array);
    TestCheck(nestedArr[3].Type() == JSValueType::String);
    TestCheck(nestedArr[4].Type() == JSValueType::Boolean);
    TestCheck(nestedArr[5].Type() == JSValueType::Int64);
    TestCheck(nestedArr[6].Type() == JSValueType::Double);

    TestCheck(nestedArr[0].IsNull());
    TestCheck(nestedArr[1].GetIfObject()->empty());
    TestCheck(nestedArr[2].GetIfArray()->empty());
    TestCheck(nestedArr[3] == "Hello");
    TestCheck(nestedArr[4] == true);
    TestCheck(nestedArr[5] == 42);
    TestCheck(nestedArr[6] == 4.5);
  }

  void CheckConversion(
      JSValue const &value, char const *strVal, bool boolVal, int64_t intVal, double doubleVal) noexcept {
    TestCheckEqual(strVal, value.AsString());
    TestCheckEqual(boolVal, value.AsBoolean());
    TestCheckEqual(intVal, value.AsInt64());
    if (std::isnan(doubleVal)) {
      TestCheck(std::isnan(value.AsDouble()));
    } else {
      TestCheckEqual(doubleVal, value.AsDouble());
    }
  }

  TEST_METHOD(TestDefaultAsConverters) {
    CheckConversion(false, "false", false, 0, 0);
    CheckConversion(true, "true", true, 1, 1);
    CheckConversion(0, "0", false, 0, 0);
    CheckConversion(1, "1", true, 1, 1);
    CheckConversion("0", "0", true, 0, 0);
    CheckConversion("000", "000", true, 0, 0);
    CheckConversion("1", "1", true, 1, 1);
    CheckConversion(NAN, "NaN", false, 0, NAN);
    CheckConversion(INFINITY, "Infinity", true, 0, INFINITY);
    CheckConversion(-INFINITY, "-Infinity", true, 0, -INFINITY);
    CheckConversion("", "", false, 0, 0);
    CheckConversion("20", "20", true, 20, 20);
    CheckConversion("twenty", "twenty", true, 0, NAN);
    CheckConversion(JSValueArray{}, "", true, 0, 0);
    CheckConversion(JSValueArray{20}, "20", true, 20, 20);
    CheckConversion(JSValueArray{10, 20}, "10,20", true, 0, NAN);
    CheckConversion(JSValueArray{"twenty"}, "twenty", true, 0, NAN);
    CheckConversion(JSValueArray{"ten", "twenty"}, "ten,twenty", true, 0, NAN);
    CheckConversion(JSValueArray{NAN}, "NaN", true, 0, NAN);
    CheckConversion(JSValueObject{}, "[object Object]", true, 0, NAN);
    CheckConversion(nullptr, "null", false, 0, 0);
  }
};

} // namespace winrt::Microsoft::ReactNative

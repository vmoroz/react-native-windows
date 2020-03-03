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
};

} // namespace winrt::Microsoft::ReactNative

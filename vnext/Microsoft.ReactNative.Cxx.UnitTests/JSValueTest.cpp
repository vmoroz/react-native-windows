// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "JSValue.h"
#include "JsonJSValueReader.h"

#undef max
#undef min

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

    TestCheck(jsValue["NullValue"].Type() == JSValueType::Null);
    TestCheck(jsValue["ObjValue"].Type() == JSValueType::Object);
    TestCheck(jsValue["ArrayValue"].Type() == JSValueType::Array);
    TestCheck(jsValue["StringValue"].Type() == JSValueType::String);
    TestCheck(jsValue["BoolValue"].Type() == JSValueType::Boolean);
    TestCheck(jsValue["IntValue"].Type() == JSValueType::Int64);
    TestCheck(jsValue["DoubleValue"].Type() == JSValueType::Double);

    TestCheck(jsValue["NullValue"].IsNull());
    TestCheck(jsValue["ObjValue"].TryGetObject()->empty());
    TestCheck(jsValue["ArrayValue"].TryGetArray()->empty());
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
    TestCheck(jsValue["NestedObj"].Type() == JSValueType::Object);
    auto const &nestedObj = *jsValue["NestedObj"].TryGetObject();

    TestCheck(nestedObj["NullValue"].Type() == JSValueType::Null);
    TestCheck(nestedObj["ObjValue"].Type() == JSValueType::Object);
    TestCheck(nestedObj["ArrayValue"].Type() == JSValueType::Array);
    TestCheck(nestedObj["StringValue"].Type() == JSValueType::String);
    TestCheck(nestedObj["BoolValue"].Type() == JSValueType::Boolean);
    TestCheck(nestedObj["IntValue"].Type() == JSValueType::Int64);
    TestCheck(nestedObj["DoubleValue"].Type() == JSValueType::Double);

    TestCheck(nestedObj["NullValue"].IsNull());
    TestCheck(nestedObj["ObjValue"].TryGetObject()->empty());
    TestCheck(nestedObj["ArrayValue"].TryGetArray()->empty());
    TestCheck(nestedObj["StringValue"] == "Hello");
    TestCheck(nestedObj["BoolValue"] == true);
    TestCheck(nestedObj["IntValue"] == 42);
    TestCheck(nestedObj["DoubleValue"] == 4.5);
  }

  TEST_METHOD(TestReadArray) {
    const wchar_t *json = LR"JSON([null, {}, [], "Hello", true, 42, 4.5])JSON";
    IJSValueReader reader = make<JsonJSValueReader>(json);

    JSValue jsValue = JSValue::ReadFrom(reader);
    TestCheck(jsValue.Type() == JSValueType::Array);

    TestCheck(jsValue[0].Type() == JSValueType::Null);
    TestCheck(jsValue[1].Type() == JSValueType::Object);
    TestCheck(jsValue[2].Type() == JSValueType::Array);
    TestCheck(jsValue[3].Type() == JSValueType::String);
    TestCheck(jsValue[4].Type() == JSValueType::Boolean);
    TestCheck(jsValue[5].Type() == JSValueType::Int64);
    TestCheck(jsValue[6].Type() == JSValueType::Double);

    TestCheck(jsValue[0].IsNull());
    TestCheck(jsValue[1].TryGetObject()->empty());
    TestCheck(jsValue[2].TryGetArray()->empty());
    TestCheck(jsValue[3] == "Hello");
    TestCheck(jsValue[4] == true);
    TestCheck(jsValue[5] == 42);
    TestCheck(jsValue[6] == 4.5);
  }

  TEST_METHOD(TestReadNestedArray) {
    const wchar_t *json = LR"JSON([[null, {}, [], "Hello", true, 42, 4.5]])JSON";
    IJSValueReader reader = make<JsonJSValueReader>(json);

    JSValue jsValue = JSValue::ReadFrom(reader);
    TestCheck(jsValue[0].TryGetArray());
    auto const &nestedArr = *jsValue[0].TryGetArray();

    TestCheck(nestedArr[0].Type() == JSValueType::Null);
    TestCheck(nestedArr[1].Type() == JSValueType::Object);
    TestCheck(nestedArr[2].Type() == JSValueType::Array);
    TestCheck(nestedArr[3].Type() == JSValueType::String);
    TestCheck(nestedArr[4].Type() == JSValueType::Boolean);
    TestCheck(nestedArr[5].Type() == JSValueType::Int64);
    TestCheck(nestedArr[6].Type() == JSValueType::Double);

    TestCheck(nestedArr[0].IsNull());
    TestCheck(nestedArr[1].TryGetObject()->empty());
    TestCheck(nestedArr[2].TryGetArray()->empty());
    TestCheck(nestedArr[3] == "Hello");
    TestCheck(nestedArr[4] == true);
    TestCheck(nestedArr[5] == 42);
    TestCheck(nestedArr[6] == 4.5);
  }

  TEST_METHOD(TestJSValueObject1) {
    JSValue value = JSValueObject{{"prop1", 1}, {"prop2", "Two"}};
    TestCheck(value.Type() == JSValueType::Object);
    TestCheck(value.PropertyCount() == 2);
    TestCheck(value["prop1"] == 1);
    TestCheck(value["prop2"] == "Two");
    TestCheck(value.AsObject().size() == 2);
    TestCheck(value.AsObject()["prop1"] == 1);
    TestCheck(value.AsObject()["prop2"] == "Two");
  }

  TEST_METHOD(TestJSValueObject2) {
    JSValueObject obj;
    obj["prop1"] = 1;
    obj["prop2"] = "Two";
    JSValue value = std::move(obj);
    TestCheck(value.Type() == JSValueType::Object);
    TestCheck(value.PropertyCount() == 2);
    TestCheck(value["prop1"] == 1);
    TestCheck(value["prop2"] == "Two");
    TestCheck(value.AsObject().size() == 2);
    TestCheck(value.AsObject()["prop1"] == 1);
    TestCheck(value.AsObject()["prop2"] == "Two");
  }

  TEST_METHOD(TestJSValueObjectInsertion) {
    // See if our JSValueObject operator[] works correctly.
    // We use a random order of keys to insert.
    JSValueObject obj;
    obj["prop03"] = 3;
    obj["prop02"] = 2;
    obj["prop08"] = 8;
    obj["prop06"] = 6;
    obj["prop10"] = 10;
    obj["prop09"] = 9;
    obj["prop04"] = 4;
    obj["prop01"] = 1;
    obj["prop05"] = 5;
    obj["prop07"] = 7;
    // Modify
    obj["prop02"] = 22;
    JSValue value = std::move(obj);
    TestCheck(value.Type() == JSValueType::Object);
    TestCheck(value.PropertyCount() == 10);
    TestCheck(value["prop01"] == 1);
    TestCheck(value["prop02"] == 22);
    TestCheck(value["prop03"] == 3);
    TestCheck(value["prop04"] == 4);
    TestCheck(value["prop05"] == 5);
    TestCheck(value["prop06"] == 6);
    TestCheck(value["prop07"] == 7);
    TestCheck(value["prop08"] == 8);
    TestCheck(value["prop09"] == 9);
    TestCheck(value["prop10"] == 10);
  }

  TEST_METHOD(TestJSValueArray1) {
    JSValue value = JSValueArray{1, "Two", 3.3, true, nullptr};
    TestCheck(value.Type() == JSValueType::Array);
    TestCheck(value.ItemCount() == 5);
    TestCheck(value[0] == 1);
    TestCheck(value[1] == "Two");
    TestCheck(value[2] == 3.3);
    TestCheck(value[3] == true);
    TestCheck(value[4] == nullptr);
  }

  TEST_METHOD(TestJSValueArray2) {
    JSValueArray arr(5);
    arr[0] = 1;
    arr[1] = "Two";
    arr[2] = 3.3;
    arr[3] = true;
    arr[4] = nullptr;
    JSValue value = std::move(arr);
    TestCheck(value.Type() == JSValueType::Array);
    TestCheck(value.ItemCount() == 5);
    TestCheck(value[0] == 1);
    TestCheck(value[1] == "Two");
    TestCheck(value[2] == 3.3);
    TestCheck(value[3] == true);
    TestCheck(value[4] == nullptr);
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
    CheckConversion(INFINITY, "Infinity", true, std::numeric_limits<int64_t>::max(), INFINITY);
    CheckConversion(-INFINITY, "-Infinity", true, std::numeric_limits<int64_t>::min(), -INFINITY);
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

  TEST_METHOD(TestEqualsWithConversion) {
    TestCheck(JSValue{nullptr}.JSEquals(nullptr) == true);
    TestCheck(JSValue{nullptr}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{nullptr}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{nullptr}.JSEquals("") == false);
    TestCheck(JSValue{nullptr}.JSEquals(false) == false);
    TestCheck(JSValue{nullptr}.JSEquals(0) == false);
    TestCheck(JSValue{nullptr}.JSEquals(0.0) == false);

    TestCheck(JSValue{JSValueObject{}}.JSEquals(nullptr) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueObject{}) == true);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals("") == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals("0") == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals("1") == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals("true") == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals("false") == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals("Hello") == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals("[object Object]") == true);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(false) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(true) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(0) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(5) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(1) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(0.0) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(0.5) == false);
    TestCheck(JSValue{JSValueObject{}}.JSEquals(1.0) == false);

    TestCheck(
        JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}})
            .JSEquals(JSValueObject{{"prop2", 0}, {"prop1", "2"}}) == true);
    TestCheck(
        JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}}).JSEquals(JSValueObject{{"prop2", 0}}) ==
        false);
    TestCheck(
        JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}})
            .JSEquals(JSValueObject{{"prop1", 2}, {"prop25", false}}) == false);
    TestCheck(
        JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}})
            .JSEquals(JSValueObject{{"prop1", 2}, {"prop2", true}}) == false);

    TestCheck(JSValue{JSValueArray{}}.JSEquals(nullptr) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{}) == true);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals("") == true);
    TestCheck(JSValue{JSValueArray{}}.JSEquals("0") == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals("1") == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals("true") == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals("false") == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals("Hello") == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(false) == true);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(true) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(0) == true);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(5) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(1) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(0.0) == true);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(0.5) == false);
    TestCheck(JSValue{JSValueArray{}}.JSEquals(1.0) == false);

    TestCheck(JSValue{JSValueArray{1}}.JSEquals(nullptr) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{1}) == true);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{"1"}) == true);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{true}) == true);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals("") == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals("0") == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals("1") == true);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals("true") == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals("false") == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals("Hello") == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(false) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(true) == true);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(0) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(5) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(1) == true);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(0.0) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(0.5) == false);
    TestCheck(JSValue{JSValueArray{1}}.JSEquals(1.0) == true);

    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(nullptr) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{"Hello"}) == true);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals("") == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals("0") == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals("1") == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals("true") == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals("false") == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals("Hello") == true);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(false) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(true) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(0) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(5) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(1) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(0.0) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(0.5) == false);
    TestCheck(JSValue{JSValueArray{"Hello"}}.JSEquals(1.0) == false);

    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(nullptr) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{0, 1}) == true);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{false, true}) == true);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{"0", "1"}) == true);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(JSValueArray{"0", true}) == true);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals("") == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals("0") == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals("1") == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals("0,1") == true);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals("true") == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals("false") == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals("Hello") == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(false) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(true) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(0) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(5) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(1) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(0.0) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(0.5) == false);
    TestCheck(JSValue(JSValueArray{0, 1}).JSEquals(1.0) == false);

    TestCheck(JSValue{""}.JSEquals(nullptr) == false);
    TestCheck(JSValue{""}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{""}.JSEquals(JSValueArray{}) == true);
    TestCheck(JSValue{""}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{""}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{""}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{""}.JSEquals(JSValueArray{""}) == true);
    TestCheck(JSValue{""}.JSEquals("") == true);
    TestCheck(JSValue{""}.JSEquals("1") == false);
    TestCheck(JSValue{""}.JSEquals("Hello") == false);
    TestCheck(JSValue{""}.JSEquals(false) == true);
    TestCheck(JSValue{""}.JSEquals(true) == false);
    TestCheck(JSValue{""}.JSEquals(0) == true);
    TestCheck(JSValue{""}.JSEquals(5) == false);
    TestCheck(JSValue{""}.JSEquals(1) == false);
    TestCheck(JSValue{""}.JSEquals(0.0) == true);
    TestCheck(JSValue{""}.JSEquals(0.5) == false);
    TestCheck(JSValue{""}.JSEquals(1.0) == false);

    TestCheck(JSValue{"Hello"}.JSEquals(nullptr) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(JSValueArray{"Hello"}) == true);
    TestCheck(JSValue{"Hello"}.JSEquals("") == false);
    TestCheck(JSValue{"Hello"}.JSEquals("1") == false);
    TestCheck(JSValue{"Hello"}.JSEquals("Hello") == true);
    TestCheck(JSValue{"Hello"}.JSEquals(false) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(true) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(0) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(5) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(1) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(0.0) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(0.5) == false);
    TestCheck(JSValue{"Hello"}.JSEquals(1.0) == false);

    TestCheck(JSValue{"0"}.JSEquals(nullptr) == false);
    TestCheck(JSValue{"0"}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{"0"}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{"0"}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{"0"}.JSEquals(JSValueArray{"0"}) == true);
    TestCheck(JSValue{"0"}.JSEquals(JSValueArray{0}) == true);
    TestCheck(JSValue{"0"}.JSEquals("") == false);
    TestCheck(JSValue{"0"}.JSEquals("0") == true);
    TestCheck(JSValue{"0"}.JSEquals("Hello") == false);
    TestCheck(JSValue{"0"}.JSEquals(false) == true);
    TestCheck(JSValue{"0"}.JSEquals(true) == false);
    TestCheck(JSValue{"0"}.JSEquals(0) == true);
    TestCheck(JSValue{"0"}.JSEquals(5) == false);
    TestCheck(JSValue{"0"}.JSEquals(1) == false);
    TestCheck(JSValue{"0"}.JSEquals(0.0) == true);
    TestCheck(JSValue{"0"}.JSEquals(0.5) == false);
    TestCheck(JSValue{"0"}.JSEquals(1.0) == false);

    TestCheck(JSValue{"1"}.JSEquals(nullptr) == false);
    TestCheck(JSValue{"1"}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{"1"}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{"1"}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{"1"}.JSEquals(JSValueArray{"1"}) == true);
    TestCheck(JSValue{"1"}.JSEquals(JSValueArray{1}) == true);
    TestCheck(JSValue{"1"}.JSEquals("") == false);
    TestCheck(JSValue{"1"}.JSEquals("1") == true);
    TestCheck(JSValue{"1"}.JSEquals("Hello") == false);
    TestCheck(JSValue{"1"}.JSEquals(false) == false);
    TestCheck(JSValue{"1"}.JSEquals(true) == true);
    TestCheck(JSValue{"1"}.JSEquals(0) == false);
    TestCheck(JSValue{"1"}.JSEquals(5) == false);
    TestCheck(JSValue{"1"}.JSEquals(1) == true);
    TestCheck(JSValue{"1"}.JSEquals(0.0) == false);
    TestCheck(JSValue{"1"}.JSEquals(0.5) == false);
    TestCheck(JSValue{"1"}.JSEquals(1.0) == true);

    TestCheck(JSValue{"true"}.JSEquals(nullptr) == false);
    TestCheck(JSValue{"true"}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{"true"}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{"true"}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{"true"}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{"true"}.JSEquals(JSValueArray{true}) == true);
    TestCheck(JSValue{"true"}.JSEquals(JSValueArray{"true"}) == true);
    TestCheck(JSValue{"true"}.JSEquals("") == false);
    TestCheck(JSValue{"true"}.JSEquals("1") == false);
    TestCheck(JSValue{"true"}.JSEquals("Hello") == false);
    TestCheck(JSValue{"true"}.JSEquals("true") == true);
    TestCheck(JSValue{"true"}.JSEquals(false) == false);
    TestCheck(JSValue{"true"}.JSEquals(true) == false);
    TestCheck(JSValue{"true"}.JSEquals(0) == false);
    TestCheck(JSValue{"true"}.JSEquals(5) == false);
    TestCheck(JSValue{"true"}.JSEquals(1) == false);
    TestCheck(JSValue{"true"}.JSEquals(0.0) == false);
    TestCheck(JSValue{"true"}.JSEquals(0.5) == false);
    TestCheck(JSValue{"true"}.JSEquals(1.0) == false);

    TestCheck(JSValue{"[object Object]"}.JSEquals(JSValueObject{}) == true);

    TestCheck(JSValue{true}.JSEquals(nullptr) == false);
    TestCheck(JSValue{true}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{1}) == true);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{"1"}) == true);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{true}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{true}.JSEquals("") == false);
    TestCheck(JSValue{true}.JSEquals("0") == false);
    TestCheck(JSValue{true}.JSEquals("1") == true);
    TestCheck(JSValue{true}.JSEquals("true") == false);
    TestCheck(JSValue{true}.JSEquals("false") == false);
    TestCheck(JSValue{true}.JSEquals("Hello") == false);
    TestCheck(JSValue{true}.JSEquals(false) == false);
    TestCheck(JSValue{true}.JSEquals(true) == true);
    TestCheck(JSValue{true}.JSEquals(0) == false);
    TestCheck(JSValue{true}.JSEquals(5) == false);
    TestCheck(JSValue{true}.JSEquals(1) == true);
    TestCheck(JSValue{true}.JSEquals(0.0) == false);
    TestCheck(JSValue{true}.JSEquals(0.5) == false);
    TestCheck(JSValue{true}.JSEquals(1.0) == true);

    TestCheck(JSValue{false}.JSEquals(nullptr) == false);
    TestCheck(JSValue{false}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{}) == true);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{0}) == true);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{"0"}) == true);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{false}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{false}.JSEquals("") == true);
    TestCheck(JSValue{false}.JSEquals("0") == true);
    TestCheck(JSValue{false}.JSEquals("1") == false);
    TestCheck(JSValue{false}.JSEquals("true") == false);
    TestCheck(JSValue{false}.JSEquals("false") == false);
    TestCheck(JSValue{false}.JSEquals("Hello") == false);
    TestCheck(JSValue{false}.JSEquals(false) == true);
    TestCheck(JSValue{false}.JSEquals(true) == false);
    TestCheck(JSValue{false}.JSEquals(0) == true);
    TestCheck(JSValue{false}.JSEquals(5) == false);
    TestCheck(JSValue{false}.JSEquals(1) == false);
    TestCheck(JSValue{false}.JSEquals(0.0) == true);
    TestCheck(JSValue{false}.JSEquals(0.5) == false);
    TestCheck(JSValue{false}.JSEquals(1.0) == false);

    TestCheck(JSValue{0}.JSEquals(nullptr) == false);
    TestCheck(JSValue{0}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{}) == true);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{0}) == true);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{"0"}) == true);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{0}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{0}.JSEquals("") == true);
    TestCheck(JSValue{0}.JSEquals("0") == true);
    TestCheck(JSValue{0}.JSEquals("1") == false);
    TestCheck(JSValue{0}.JSEquals("true") == false);
    TestCheck(JSValue{0}.JSEquals("false") == false);
    TestCheck(JSValue{0}.JSEquals("Hello") == false);
    TestCheck(JSValue{0}.JSEquals(false) == true);
    TestCheck(JSValue{0}.JSEquals(true) == false);
    TestCheck(JSValue{0}.JSEquals(0) == true);
    TestCheck(JSValue{0}.JSEquals(5) == false);
    TestCheck(JSValue{0}.JSEquals(1) == false);
    TestCheck(JSValue{0}.JSEquals(0.0) == true);
    TestCheck(JSValue{0}.JSEquals(0.5) == false);
    TestCheck(JSValue{0}.JSEquals(1.0) == false);

    TestCheck(JSValue{1}.JSEquals(nullptr) == false);
    TestCheck(JSValue{1}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{1}) == true);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{"1"}) == true);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{1}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{1}.JSEquals("") == false);
    TestCheck(JSValue{1}.JSEquals("0") == false);
    TestCheck(JSValue{1}.JSEquals("1") == true);
    TestCheck(JSValue{1}.JSEquals("true") == false);
    TestCheck(JSValue{1}.JSEquals("false") == false);
    TestCheck(JSValue{1}.JSEquals("Hello") == false);
    TestCheck(JSValue{1}.JSEquals(false) == false);
    TestCheck(JSValue{1}.JSEquals(true) == true);
    TestCheck(JSValue{1}.JSEquals(0) == false);
    TestCheck(JSValue{1}.JSEquals(5) == false);
    TestCheck(JSValue{1}.JSEquals(1) == true);
    TestCheck(JSValue{1}.JSEquals(0.0) == false);
    TestCheck(JSValue{1}.JSEquals(0.5) == false);
    TestCheck(JSValue{1}.JSEquals(1.0) == true);

    TestCheck(JSValue{5}.JSEquals(nullptr) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{5}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{5}.JSEquals("") == false);
    TestCheck(JSValue{5}.JSEquals("0") == false);
    TestCheck(JSValue{5}.JSEquals("1") == false);
    TestCheck(JSValue{5}.JSEquals("true") == false);
    TestCheck(JSValue{5}.JSEquals("false") == false);
    TestCheck(JSValue{5}.JSEquals("Hello") == false);
    TestCheck(JSValue{5}.JSEquals(false) == false);
    TestCheck(JSValue{5}.JSEquals(true) == false);
    TestCheck(JSValue{5}.JSEquals(0) == false);
    TestCheck(JSValue{5}.JSEquals(5) == true);
    TestCheck(JSValue{5}.JSEquals(1) == false);
    TestCheck(JSValue{5}.JSEquals(0.0) == false);
    TestCheck(JSValue{5}.JSEquals(0.5) == false);
    TestCheck(JSValue{5}.JSEquals(1.0) == false);

    TestCheck(JSValue{0.0}.JSEquals(nullptr) == false);
    TestCheck(JSValue{0.0}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{}) == true);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{0}) == true);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{"0"}) == true);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{0.0}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{0.0}.JSEquals("") == true);
    TestCheck(JSValue{0.0}.JSEquals("0") == true);
    TestCheck(JSValue{0.0}.JSEquals("1") == false);
    TestCheck(JSValue{0.0}.JSEquals("true") == false);
    TestCheck(JSValue{0.0}.JSEquals("false") == false);
    TestCheck(JSValue{0.0}.JSEquals("Hello") == false);
    TestCheck(JSValue{0.0}.JSEquals(false) == true);
    TestCheck(JSValue{0.0}.JSEquals(true) == false);
    TestCheck(JSValue{0.0}.JSEquals(0) == true);
    TestCheck(JSValue{0.0}.JSEquals(5) == false);
    TestCheck(JSValue{0.0}.JSEquals(1) == false);
    TestCheck(JSValue{0.0}.JSEquals(0.0) == true);
    TestCheck(JSValue{0.0}.JSEquals(0.5) == false);
    TestCheck(JSValue{0.0}.JSEquals(1.0) == false);

    TestCheck(JSValue{1.0}.JSEquals(nullptr) == false);
    TestCheck(JSValue{1.0}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{1}) == true);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{"1"}) == true);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{1.0}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{1.0}.JSEquals("") == false);
    TestCheck(JSValue{1.0}.JSEquals("0") == false);
    TestCheck(JSValue{1.0}.JSEquals("1") == true);
    TestCheck(JSValue{1.0}.JSEquals("true") == false);
    TestCheck(JSValue{1.0}.JSEquals("false") == false);
    TestCheck(JSValue{1.0}.JSEquals("Hello") == false);
    TestCheck(JSValue{1.0}.JSEquals(false) == false);
    TestCheck(JSValue{1.0}.JSEquals(true) == true);
    TestCheck(JSValue{1.0}.JSEquals(0) == false);
    TestCheck(JSValue{1.0}.JSEquals(5) == false);
    TestCheck(JSValue{1.0}.JSEquals(1) == true);
    TestCheck(JSValue{1.0}.JSEquals(0.0) == false);
    TestCheck(JSValue{1.0}.JSEquals(0.5) == false);
    TestCheck(JSValue{1.0}.JSEquals(1.0) == true);

    TestCheck(JSValue{0.5}.JSEquals(nullptr) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueObject{}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{"Hello"}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{0}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{"0"}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{1}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{"1"}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{true}) == false);
    TestCheck(JSValue{0.5}.JSEquals(JSValueArray{"true"}) == false);
    TestCheck(JSValue{0.5}.JSEquals("") == false);
    TestCheck(JSValue{0.5}.JSEquals("0") == false);
    TestCheck(JSValue{0.5}.JSEquals("1") == false);
    TestCheck(JSValue{0.5}.JSEquals("true") == false);
    TestCheck(JSValue{0.5}.JSEquals("false") == false);
    TestCheck(JSValue{0.5}.JSEquals("Hello") == false);
    TestCheck(JSValue{0.5}.JSEquals(false) == false);
    TestCheck(JSValue{0.5}.JSEquals(true) == false);
    TestCheck(JSValue{0.5}.JSEquals(0) == false);
    TestCheck(JSValue{0.5}.JSEquals(5) == false);
    TestCheck(JSValue{0.5}.JSEquals(1) == false);
    TestCheck(JSValue{0.5}.JSEquals(0.0) == false);
    TestCheck(JSValue{0.5}.JSEquals(0.5) == true);
    TestCheck(JSValue{0.5}.JSEquals(1.0) == false);
  }
};

} // namespace winrt::Microsoft::ReactNative

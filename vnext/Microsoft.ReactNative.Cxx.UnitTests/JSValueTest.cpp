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
        "ObjValue": {"prop1": 2},
        "ArrayValue": [1, 2],
        "StringValue": "Hello",
        "BoolValue": true,
        "IntValue": 42,
        "DoubleValue": 4.5
      })JSON";
    IJSValueReader reader = make<JsonJSValueReader>(json);
    JSValue jsValue = JSValue::ReadFrom(reader);

    TestCheckEqual(JSValueType::Object, jsValue.Type());
    TestCheckEqual(JSValueType::Null, jsValue["NullValue"].Type());
    TestCheckEqual(JSValueType::Object, jsValue["ObjValue"].Type());
    TestCheckEqual(JSValueType::Array, jsValue["ArrayValue"].Type());
    TestCheckEqual(JSValueType::String, jsValue["StringValue"].Type());
    TestCheckEqual(JSValueType::Boolean, jsValue["BoolValue"].Type());
    TestCheckEqual(JSValueType::Int64, jsValue["IntValue"].Type());
    TestCheckEqual(JSValueType::Double, jsValue["DoubleValue"].Type());

    JSValueObject const *objValue = nullptr;
    JSValueArray const *arrayValue = nullptr;
    std::string const *stringValue = nullptr;
    bool const *boolValue = nullptr;
    int64_t const *intValue = nullptr;
    double const *doubleValue = nullptr;

    TestCheck(jsValue["NullValue"].IsNull());
    TestCheck(objValue = jsValue["ObjValue"].TryGetObject());
    TestCheck(arrayValue = jsValue["ArrayValue"].TryGetArray());
    TestCheck(stringValue = jsValue["StringValue"].TryGetString());
    TestCheck(boolValue = jsValue["BoolValue"].TryGetBoolean());
    TestCheck(intValue = jsValue["IntValue"].TryGetInt64());
    TestCheck(doubleValue = jsValue["DoubleValue"].TryGetDouble());

    TestCheckEqual(1, objValue->size());
    TestCheckEqual(1, jsValue["ObjValue"].PropertyCount());
    TestCheckEqual(2, arrayValue->size());
    TestCheckEqual(2, jsValue["ArrayValue"].ItemCount());
    TestCheckEqual("Hello", *stringValue);
    TestCheckEqual(true, *boolValue);
    TestCheckEqual(42, *intValue);
    TestCheckEqual(4.5, *doubleValue);
  }

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

    TestCheckEqual(JSValueType::Object, jsValue.Type());
    TestCheckEqual(JSValueType::Object, jsValue["NestedObj"].Type());
    auto const &nestedObj = *jsValue["NestedObj"].TryGetObject();

    TestCheckEqual(JSValueType::Null, nestedObj["NullValue"].Type());
    TestCheckEqual(JSValueType::Object, nestedObj["ObjValue"].Type());
    TestCheckEqual(JSValueType::Array, nestedObj["ArrayValue"].Type());
    TestCheckEqual(JSValueType::String, nestedObj["StringValue"].Type());
    TestCheckEqual(JSValueType::Boolean, nestedObj["BoolValue"].Type());
    TestCheckEqual(JSValueType::Int64, nestedObj["IntValue"].Type());
    TestCheckEqual(JSValueType::Double, nestedObj["DoubleValue"].Type());

    TestCheck(nestedObj["NullValue"].IsNull());
    TestCheck(nestedObj["ObjValue"].TryGetObject());
    TestCheck(nestedObj["ArrayValue"].TryGetArray());
    TestCheckEqual("Hello", nestedObj["StringValue"]);
    TestCheckEqual(true, nestedObj["BoolValue"]);
    TestCheckEqual(42, nestedObj["IntValue"]);
    TestCheckEqual(4.5, nestedObj["DoubleValue"]);
  }

  TEST_METHOD(TestReadArray) {
    const wchar_t *json = LR"JSON([null, {}, [], "Hello", true, 42, 4.5])JSON";
    IJSValueReader reader = make<JsonJSValueReader>(json);
    JSValue jsValue = JSValue::ReadFrom(reader);

    TestCheckEqual(JSValueType::Array, jsValue.Type());
    TestCheckEqual(JSValueType::Null, jsValue[0].Type());
    TestCheckEqual(JSValueType::Object, jsValue[1].Type());
    TestCheckEqual(JSValueType::Array, jsValue[2].Type());
    TestCheckEqual(JSValueType::String, jsValue[3].Type());
    TestCheckEqual(JSValueType::Boolean, jsValue[4].Type());
    TestCheckEqual(JSValueType::Int64, jsValue[5].Type());
    TestCheckEqual(JSValueType::Double, jsValue[6].Type());

    TestCheck(jsValue[0].IsNull());
    TestCheck(jsValue[1].TryGetObject());
    TestCheck(jsValue[2].TryGetArray());
    TestCheckEqual("Hello", jsValue[3]);
    TestCheckEqual(true, jsValue[4]);
    TestCheckEqual(42, jsValue[5]);
    TestCheckEqual(4.5, jsValue[6]);
  }
#if 0
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

  void CheckJSConversionImpl(
      int line,
      JSValue const &jsValue,
      char const *stringValue,
      bool boolValue,
      double numberValue,
      char const *message) noexcept {
    // We must have "%s" parameter for messages because the first format parameter must be a constant.
    TestCheckEqualAt(__FILE__, line, stringValue, jsValue.AsJSString(), "%s", message);
    TestCheckEqualAt(__FILE__, line, boolValue, jsValue.AsJSBoolean(), "%s", message);
    if (std::isnan(numberValue)) {
      TestCheckAt(__FILE__, line, std::isnan(jsValue.AsJSNumber()), "%s", message);
    } else {
      TestCheckEqualAt(__FILE__, line, numberValue, jsValue.AsJSNumber(), "%s", message);
    }
  }

#define CheckJSConversion(jsValue, stringValue, boolValue, numberValue) \
  CheckJSConversionImpl(                                                \
      __LINE__,                                                         \
      jsValue,                                                          \
      stringValue,                                                      \
      boolValue,                                                        \
      numberValue,                                                      \
      "Conversion: [ " #jsValue " ==> " #stringValue ", " #boolValue ", " #numberValue " ] ");

  TEST_METHOD(TestDefaultAsConverters) {
    CheckJSConversion(false, "false", false, 0);
     CheckJSConversion(true, "true", true, 1);
     CheckJSConversion(0, "0", false, 0);
     CheckJSConversion(1, "1", true, 1);
     CheckJSConversion("0", "0", true, 0);
     CheckJSConversion("000", "000", true, 0);
     CheckJSConversion("1", "1", true, 1);
     CheckJSConversion(NAN, "NaN", false, NAN);
     CheckJSConversion(INFINITY, "Infinity", true, INFINITY);
     CheckJSConversion(-INFINITY, "-Infinity", true, -INFINITY);
     CheckJSConversion("", "", false, 0);
     CheckJSConversion("20", "20", true, 20);
     CheckJSConversion("twenty", "twenty", true, NAN);
     CheckJSConversion(JSValueArray{}, "", true, 0);
     CheckJSConversion(JSValueArray{20}, "20", true, 20);
     CheckJSConversion((JSValueArray{10, 20}), "10,20", true, NAN);
     CheckJSConversion(JSValueArray{"twenty"}, "twenty", true, NAN);
     CheckJSConversion((JSValueArray{"ten", "twenty"}), "ten,twenty", true, NAN);
     CheckJSConversion(JSValueArray{NAN}, "NaN", true, NAN);
     CheckJSConversion(JSValueObject{}, "[object Object]", true, NAN);
     CheckJSConversion(nullptr, "null", false, 0);
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
        JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}}).JSEquals(JSValueObject{{"prop2", 0}, {"prop1", "2"}}) ==
        true);
    TestCheck(JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}}).JSEquals(JSValueObject{{"prop2", 0}}) == false);
    TestCheck(
        JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}})
            .JSEquals(JSValueObject{{"prop1", 2}, {"prop25", false}}) == false);
    TestCheck(
        JSValue(JSValueObject{{"prop1", 2}, {"prop2", false}}).JSEquals(JSValueObject{{"prop1", 2}, {"prop2", true}}) ==
        false);

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
#endif
};

} // namespace winrt::Microsoft::ReactNative

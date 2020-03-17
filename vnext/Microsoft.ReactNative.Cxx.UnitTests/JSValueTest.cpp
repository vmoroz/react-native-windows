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

  TEST_METHOD(TestReadNestedArray) {
    const wchar_t *json = LR"JSON([[null, {}, [], "Hello", true, 42, 4.5]])JSON";
    IJSValueReader reader = make<JsonJSValueReader>(json);
    JSValue jsValue = JSValue::ReadFrom(reader);

    TestCheckEqual(JSValueType::Array, jsValue.Type());
    TestCheckEqual(JSValueType::Array, jsValue[0].Type());
    auto const &nestedArr = *jsValue[0].TryGetArray();

    TestCheckEqual(JSValueType::Null, nestedArr[0].Type());
    TestCheckEqual(JSValueType::Object, nestedArr[1].Type());
    TestCheckEqual(JSValueType::Array, nestedArr[2].Type());
    TestCheckEqual(JSValueType::String, nestedArr[3].Type());
    TestCheckEqual(JSValueType::Boolean, nestedArr[4].Type());
    TestCheckEqual(JSValueType::Int64, nestedArr[5].Type());
    TestCheckEqual(JSValueType::Double, nestedArr[6].Type());

    TestCheck(nestedArr[0].IsNull());
    TestCheck(nestedArr[1].TryGetObject());
    TestCheck(nestedArr[2].TryGetArray());
    TestCheckEqual("Hello", nestedArr[3]);
    TestCheckEqual(true, nestedArr[4]);
    TestCheckEqual(42, nestedArr[5]);
    TestCheckEqual(4.5, nestedArr[6]);
  }

  TEST_METHOD(TestJSSimpleLiterals) {
    JSValue jsValue01 = nullptr;
    JSValue jsValue02 = "Hello";
    JSValue jsValue03 = "";
    JSValue jsValue04 = true;
    JSValue jsValue05 = false;
    JSValue jsValue06 = 42;
    JSValue jsValue07 = 0;
    JSValue jsValue08 = 4.5;
    JSValue jsValue09 = 0.0;
    JSValue jsValue10 = NAN;
    JSValue jsValue11 = INFINITY;
    JSValue jsValue12 = -INFINITY;

    TestCheckEqual(JSValueType::Null, jsValue01.Type());
    TestCheckEqual(JSValueType::String, jsValue02.Type());
    TestCheckEqual(JSValueType::String, jsValue03.Type());
    TestCheckEqual(JSValueType::Boolean, jsValue04.Type());
    TestCheckEqual(JSValueType::Boolean, jsValue05.Type());
    TestCheckEqual(JSValueType::Int64, jsValue06.Type());
    TestCheckEqual(JSValueType::Int64, jsValue07.Type());
    TestCheckEqual(JSValueType::Double, jsValue08.Type());
    TestCheckEqual(JSValueType::Double, jsValue09.Type());
    TestCheckEqual(JSValueType::Double, jsValue10.Type());
    TestCheckEqual(JSValueType::Double, jsValue11.Type());
    TestCheckEqual(JSValueType::Double, jsValue12.Type());

    TestCheck(jsValue01.IsNull());
    TestCheckEqual("Hello", *jsValue02.TryGetString());
    TestCheckEqual("", *jsValue03.TryGetString());
    TestCheckEqual(true, *jsValue04.TryGetBoolean());
    TestCheckEqual(false, *jsValue05.TryGetBoolean());
    TestCheckEqual(42, *jsValue06.TryGetInt64());
    TestCheckEqual(0, *jsValue07.TryGetInt64());
    TestCheckEqual(4.5, *jsValue08.TryGetDouble());
    TestCheckEqual(0.0, *jsValue09.TryGetDouble());
    TestCheck(std::isnan(*jsValue10.TryGetDouble()));
    TestCheckEqual(INFINITY, *jsValue11.TryGetDouble());
    TestCheckEqual(-INFINITY, *jsValue12.TryGetDouble());
  }

  TEST_METHOD(TestObjectLiteral) {
    JSValue jsValue = JSValueObject{{"NullValue1", nullptr},
                                    {"NullValue2", JSValue::Null.Copy()},
                                    {"ObjValue", JSValueObject{{"prop1", 2}}},
                                    {"ObjValueEmpty", JSValue::EmptyObject.Copy()},
                                    {"ArrayValue", JSValueArray{1, 2}},
                                    {"ArrayValueEmpty", JSValue::EmptyArray.Copy()},
                                    {"StringValue1", "Hello"},
                                    {"StringValue2", JSValue::EmptyString.Copy()},
                                    {"BoolValue", true},
                                    {"IntValue", 42},
                                    {"DoubleValue", 4.5}};
    TestCheckEqual(JSValueType::Object, jsValue.Type());
    TestCheckEqual(JSValueType::Null, jsValue["NullValue1"].Type());
    TestCheckEqual(JSValueType::Null, jsValue["NullValue2"].Type());
    TestCheckEqual(JSValueType::Object, jsValue["ObjValue"].Type());
    TestCheckEqual(JSValueType::Object, jsValue["ObjValueEmpty"].Type());
    TestCheckEqual(JSValueType::Array, jsValue["ArrayValue"].Type());
    TestCheckEqual(JSValueType::Array, jsValue["ArrayValueEmpty"].Type());
    TestCheckEqual(JSValueType::String, jsValue["StringValue1"].Type());
    TestCheckEqual(JSValueType::String, jsValue["StringValue2"].Type());
    TestCheckEqual(JSValueType::Boolean, jsValue["BoolValue"].Type());
    TestCheckEqual(JSValueType::Int64, jsValue["IntValue"].Type());
    TestCheckEqual(JSValueType::Double, jsValue["DoubleValue"].Type());

    TestCheck(jsValue["NullValue1"].IsNull());
    TestCheck(jsValue["NullValue2"].IsNull());
    TestCheckEqual(1, jsValue["ObjValue"].PropertyCount());
    TestCheckEqual(2, jsValue["ObjValue"]["prop1"]);
    TestCheckEqual(0, jsValue["ObjValueEmpty"].PropertyCount());
    TestCheckEqual(2, jsValue["ArrayValue"].ItemCount());
    TestCheckEqual(1, jsValue["ArrayValue"][0]);
    TestCheckEqual(2, jsValue["ArrayValue"][1]);
    TestCheckEqual(0, jsValue["ArrayValueEmpty"].ItemCount());
    TestCheckEqual("Hello", jsValue["StringValue1"]);
    TestCheckEqual("", jsValue["StringValue2"]);
    TestCheckEqual(true, jsValue["BoolValue"]);
    TestCheckEqual(42, jsValue["IntValue"]);
    TestCheck(24 != jsValue["IntValue"]);
    TestCheckEqual(4.5, jsValue["DoubleValue"]);
  }

  TEST_METHOD(TestArrayLiteral) {
    JSValue jsValue = JSValueArray{nullptr,
                                   JSValueObject{{"prop1", 2}},
                                   JSValueObject{},
                                   JSValueArray{1, 2},
                                   JSValueArray{},
                                   "Hello",
                                   true,
                                   42,
                                   4.5};

    TestCheckEqual(JSValueType::Array, jsValue.Type());
    TestCheckEqual(JSValueType::Null, jsValue[0].Type());
    TestCheckEqual(JSValueType::Object, jsValue[1].Type());
    TestCheckEqual(JSValueType::Object, jsValue[2].Type());
    TestCheckEqual(JSValueType::Array, jsValue[3].Type());
    TestCheckEqual(JSValueType::Array, jsValue[4].Type());
    TestCheckEqual(JSValueType::String, jsValue[5].Type());
    TestCheckEqual(JSValueType::Boolean, jsValue[6].Type());
    TestCheckEqual(JSValueType::Int64, jsValue[7].Type());
    TestCheckEqual(JSValueType::Double, jsValue[8].Type());

    TestCheck(jsValue["NullValue"].IsNull());
    TestCheckEqual(1, jsValue[1].PropertyCount());
    TestCheckEqual(2, jsValue[1]["prop1"]);
    TestCheckEqual(0, jsValue[2].PropertyCount());
    TestCheckEqual(2, jsValue[3].ItemCount());
    TestCheckEqual(1, jsValue[3][0]);
    TestCheckEqual(2, jsValue[3][1]);
    TestCheckEqual(0, jsValue[4].ItemCount());
    TestCheckEqual("Hello", jsValue[5]);
    TestCheckEqual(true, jsValue[6]);
    TestCheckEqual(42, jsValue[7]);
    TestCheck(24 != jsValue[7]);
    TestCheckEqual(4.5, jsValue[8]);
  }

  TEST_METHOD(TestJSValueConstructor) {
    JSValue value01;
    JSValue value02{JSValueObject{{"prop1", 3}}};
    JSValue value03{JSValueObject{}};
    JSValue value04{JSValueArray{1, 2}};
    JSValue value05{JSValueArray{}};
    JSValue value06{"Hello"};
    JSValue value07{true};
    JSValue value08{false};
    JSValue value09{0};
    JSValue value10{42};
    JSValue value11{4.2};

    TestCheckEqual(JSValueType::Null, value01.Type());
    TestCheckEqual(JSValueType::Object, value02.Type());
    TestCheckEqual(JSValueType::Object, value03.Type());
    TestCheckEqual(JSValueType::Array, value04.Type());
    TestCheckEqual(JSValueType::Array, value05.Type());
    TestCheckEqual(JSValueType::String, value06.Type());
    TestCheckEqual(JSValueType::Boolean, value07.Type());
    TestCheckEqual(JSValueType::Boolean, value08.Type());
    TestCheckEqual(JSValueType::Int64, value09.Type());
    TestCheckEqual(JSValueType::Int64, value10.Type());
    TestCheckEqual(JSValueType::Double, value11.Type());

    TestCheck(value01.IsNull());
    TestCheckEqual(1, value02.TryGetObject()->size());
    TestCheckEqual(0, value03.TryGetObject()->size());
    TestCheckEqual(2, value04.TryGetArray()->size());
    TestCheckEqual(0, value05.TryGetArray()->size());
    TestCheckEqual("Hello", *value06.TryGetString());
    TestCheckEqual(true, *value07.TryGetBoolean());
    TestCheckEqual(false, *value08.TryGetBoolean());
    TestCheckEqual(0, *value09.TryGetInt64());
    TestCheckEqual(42, *value10.TryGetInt64());
    TestCheckEqual(4.2, *value11.TryGetDouble());
  }

  TEST_METHOD(TestJSValueImplicitCast) {
    // We do not assign values directly here sbecause it would be an explicit constructor call.
    JSValue value01, value02, value03, value04, value05, value06, value07, value08, value09, value10, value11, value12,
        value13, value14, value15, value16, value17, value18, value19, value20, value21;

    std::vector<std::pair<std::string, JSValue>> objInput1;
    objInput1.emplace_back("prop1", nullptr);
    objInput1.emplace_back("prop2", 42);
    objInput1.emplace_back("prop3", "Hello");

    std::map<std::string, JSValue, std::less<>> objInput2;
    objInput2["prop1"] = nullptr;
    objInput2["prop2"] = 42;
    objInput2["prop3"] = "Hello";

    std::vector<JSValue> arrInput1;
    arrInput1.emplace_back(nullptr);
    arrInput1.emplace_back(42);
    arrInput1.emplace_back("Hello");

    value01 = JSValueObject(std::move_iterator(objInput1.begin()), std::move_iterator(objInput1.end()));
    value02 = JSValueObject(std::move(objInput2));
    value03 = JSValueObject{{"prop1", 3}};
    value04 = JSValueArray(3); // Array of 3 JSValue::Null
    value05 = JSValueArray{3}; // Array with one JSValue(3) item
    value06 = JSValueArray(2, 42); // Array with two JSValue(42) items
    value07 = JSValueArray(std::move(arrInput1));
    value08 = JSValueArray{nullptr, 42, "Hello"};
    value09 = "Hello";
    value10 = true;
    value11 = false;
    value12 = (int8_t)42;
    value13 = (int16_t)42;
    value14 = (int32_t)42;
    value15 = (int64_t)42;
    value16 = (uint8_t)42;
    value17 = (uint16_t)42;
    value18 = (uint32_t)42;
    value19 = (uint64_t)42;
    value20 = (float)4.2;
    value21 = 4.2;

    TestCheckEqual(JSValueType::Object, value01.Type());
    TestCheckEqual(JSValueType::Object, value02.Type());
    TestCheckEqual(JSValueType::Object, value03.Type());
    TestCheckEqual(JSValueType::Array, value04.Type());
    TestCheckEqual(JSValueType::Array, value05.Type());
    TestCheckEqual(JSValueType::Array, value06.Type());
    TestCheckEqual(JSValueType::Array, value07.Type());
    TestCheckEqual(JSValueType::Array, value08.Type());
    TestCheckEqual(JSValueType::String, value09.Type());
    TestCheckEqual(JSValueType::Boolean, value10.Type());
    TestCheckEqual(JSValueType::Boolean, value11.Type());
    TestCheckEqual(JSValueType::Int64, value12.Type());
    TestCheckEqual(JSValueType::Int64, value13.Type());
    TestCheckEqual(JSValueType::Int64, value14.Type());
    TestCheckEqual(JSValueType::Int64, value15.Type());
    TestCheckEqual(JSValueType::Int64, value16.Type());
    TestCheckEqual(JSValueType::Int64, value17.Type());
    TestCheckEqual(JSValueType::Int64, value18.Type());
    TestCheckEqual(JSValueType::Int64, value19.Type());
    TestCheckEqual(JSValueType::Double, value20.Type());
    TestCheckEqual(JSValueType::Double, value21.Type());

    TestCheckEqual(3, value01.PropertyCount());
    TestCheckEqual(nullptr, value01["prop1"]);
    TestCheckEqual(42, value01["prop2"]);
    TestCheckEqual("Hello", value01["prop3"]);

    TestCheckEqual(3, value02.PropertyCount());
    TestCheckEqual(nullptr, value02["prop1"]);
    TestCheckEqual(42, value02["prop2"]);
    TestCheckEqual("Hello", value02["prop3"]);

    TestCheckEqual(1, value03.PropertyCount());
    TestCheckEqual(3, value03["prop1"]);

    TestCheckEqual(3, value04.ItemCount());
    TestCheckEqual(nullptr, value04[0]);
    TestCheckEqual(nullptr, value04[1]);
    TestCheckEqual(nullptr, value04[2]);

    TestCheckEqual(1, value05.ItemCount());
    TestCheckEqual(3, value05[0]);

    TestCheckEqual(2, value06.ItemCount());
    TestCheckEqual(42, value06[0]);
    TestCheckEqual(42, value06[1]);

    TestCheckEqual(3, value07.ItemCount());
    TestCheckEqual(nullptr, value07[0]);
    TestCheckEqual(42, value07[1]);
    TestCheckEqual("Hello", value07[2]);

    TestCheckEqual(3, value08.ItemCount());
    TestCheckEqual(nullptr, value08[0]);
    TestCheckEqual(42, value08[1]);
    TestCheckEqual("Hello", value08[2]);

    TestCheckEqual("Hello", value09);
    TestCheckEqual(true, value10);
    TestCheckEqual(false, value11);
    TestCheckEqual(42, value12);
    TestCheckEqual(42, value13);
    TestCheckEqual(42, value14);
    TestCheckEqual(42, value15);
    TestCheckEqual(42, value16);
    TestCheckEqual(42, value17);
    TestCheckEqual(42, value18);
    TestCheckEqual(42, value19);
    TestCheckEqual((float)4.2, value20);
    TestCheckEqual(4.2, value21);
  }

#if 0
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

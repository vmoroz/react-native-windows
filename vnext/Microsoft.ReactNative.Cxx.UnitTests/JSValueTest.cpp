// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "JSValue.h"
#include "JsonJSValueReader.h"
#include "catch.hpp"

namespace winrt::Microsoft::ReactNative::Bridge {

TEST_CASE("TestReadObject", "JSValueTest") {
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
  REQUIRE(jsValue.Type() == JSValueType::Object);
  REQUIRE(jsValue.Object().at("NullValue").IsNull());
  REQUIRE(jsValue.Object().at("ObjValue").Object().empty());
  REQUIRE(jsValue.Object().at("ArrayValue").Array().empty());
  REQUIRE(jsValue.Object().at("StringValue").String() == "Hello");
  REQUIRE(jsValue.Object().at("BoolValue").Boolean() == true);
  REQUIRE(jsValue.Object().at("IntValue").Int64() == 42);
  REQUIRE(jsValue.Object().at("DoubleValue").Double() == 4.5);
}

TEST_CASE("TestReadNestedObject", "JSValueTest") {
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
  REQUIRE(jsValue.Type() == JSValueType::Object);
  const auto &nestedObj = jsValue.Object().at("NestedObj").Object();
  REQUIRE(nestedObj.at("NullValue").IsNull());
  REQUIRE(nestedObj.at("ObjValue").Object().empty());
  REQUIRE(nestedObj.at("ArrayValue").Array().empty());
  REQUIRE(nestedObj.at("StringValue").String() == "Hello");
  REQUIRE(nestedObj.at("BoolValue").Boolean() == true);
  REQUIRE(nestedObj.at("IntValue").Int64() == 42);
  REQUIRE(nestedObj.at("DoubleValue").Double() == 4.5);
}

//[TestMethod] public void TestReadArray() {
//  JArray jarr = JArray.Parse(
//      @"[null, {}, [], "
//       "Hello"
//       ", true, 42, 4.5]");
//  IJSValueReader reader = new JTokenJSValueReader(jarr);
//
//  JSValue jsValue = JSValue.ReadFrom(reader);
//  Assert.AreEqual(JSValueType.Array, jsValue.Type);
//  Assert.IsTrue(jsValue.Array[0].IsNull);
//  Assert.IsNotNull(jsValue.Array[1].Object);
//  Assert.IsNotNull(jsValue.Array[2].Array);
//  Assert.AreEqual("Hello", jsValue.Array[3].String);
//  Assert.AreEqual(true, jsValue.Array[4].Boolean);
//  Assert.AreEqual(42, jsValue.Array[5].Int64);
//  Assert.AreEqual(4.5, jsValue.Array[6].Double);
//}
//
//[TestMethod] public void TestReadNestedArray() {
//  JArray jarr = JArray.Parse(
//      @"[[null, {}, [], "
//       "Hello"
//       ", true, 42, 4.5]]");
//  IJSValueReader reader = new JTokenJSValueReader(jarr);
//
//  JSValue jsValue = JSValue.ReadFrom(reader);
//  Assert.AreEqual(JSValueType.Array, jsValue.Type);
//  var nestedArr = jsValue.Array[0].Array;
//  Assert.IsTrue(nestedArr[0].IsNull);
//  Assert.IsNotNull(nestedArr[1].Object);
//  Assert.IsNotNull(nestedArr[2].Array);
//  Assert.AreEqual("Hello", nestedArr[3].String);
//  Assert.AreEqual(true, nestedArr[4].Boolean);
//  Assert.AreEqual(42, nestedArr[5].Int64);
//  Assert.AreEqual(4.5, nestedArr[6].Double);
//}
//}
//}

} // namespace winrt::Microsoft::ReactNative::Bridge

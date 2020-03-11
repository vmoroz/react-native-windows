// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;

namespace Microsoft.ReactNative.Managed.UnitTests
{
  [TestClass]
  public class JSValueTest
  {
    [TestMethod]
    public void TestReadObject()
    {
      JObject jobj = JObject.Parse(@"{
              NullValue: null,
              ObjValue: {prop1: 2},
              ArrayValue: [1, 2],
              StringValue: ""Hello"",
              BoolValue: true,
              IntValue: 42,
              DoubleValue: 4.5
            }");
      IJSValueReader reader = new JTokenJSValueReader(jobj);

      JSValue jsValue = JSValue.ReadFrom(reader);

      Assert.AreEqual(JSValueType.Object, jsValue.Type);
      Assert.AreEqual(JSValueType.Null, jsValue["NullValue"].Type);
      Assert.AreEqual(JSValueType.Object, jsValue["ObjValue"].Type);
      Assert.AreEqual(JSValueType.Array, jsValue["ArrayValue"].Type);
      Assert.AreEqual(JSValueType.String, jsValue["StringValue"].Type);
      Assert.AreEqual(JSValueType.Boolean, jsValue["BoolValue"].Type);
      Assert.AreEqual(JSValueType.Int64, jsValue["IntValue"].Type);
      Assert.AreEqual(JSValueType.Double, jsValue["DoubleValue"].Type);

      Assert.IsTrue(jsValue["NullValue"].IsNull);
      Assert.IsTrue(jsValue["ObjValue"].TryGetObject(out var objValue));
      Assert.AreEqual(1, objValue.Count);
      Assert.IsTrue(jsValue["ArrayValue"].TryGetArray(out var arrayValue));
      Assert.AreEqual(2, arrayValue.Count);
      Assert.IsTrue(jsValue["StringValue"].TryGetString(out var stringValue));
      Assert.AreEqual("Hello", stringValue);
      Assert.IsTrue(jsValue["BoolValue"].TryGetBoolean(out var boolValue));
      Assert.AreEqual(true, boolValue);
      Assert.IsTrue(jsValue["IntValue"].TryGetInt64(out var intValue));
      Assert.AreEqual(42, intValue);
      Assert.IsTrue(jsValue["DoubleValue"].TryGetDouble(out var doubleValue));
      Assert.AreEqual(4.5, doubleValue);

      //Assert.AreEqual(1, jsValue["ObjValue"].PropertyCount);
      //Assert.AreEqual(1, jsValue["ObjValue"].AsObject.Count);

      //Assert.AreEqual(2, jsValue["ArrayValue"].ItemCount);
      //Assert.AreEqual(2, jsValue["ArrayValue"].AsArray.Count);
      //Assert.AreEqual("Hello", jsValue["StringValue"].AsJSString);
      //Assert.AreEqual(true, jsValue["BoolValue"].AsJSBoolean);
      //Assert.AreEqual(42, jsValue["IntValue"].AsJSNumber);
      //Assert.AreEqual(4.5, jsValue["DoubleValue"].AsJSNumber);
    }

    [TestMethod]
    public void TestReadNestedObject()
    {
      JObject jobj = JObject.Parse(@"{
              NestedObj: {
                NullValue: null,
                ObjValue: {},
                ArrayValue: [],
                StringValue: ""Hello"",
                BoolValue: true,
                IntValue: 42,
                DoubleValue: 4.5
              }
            }");
      IJSValueReader reader = new JTokenJSValueReader(jobj);

      JSValue jsValue = JSValue.ReadFrom(reader);
      Assert.AreEqual(JSValueType.Object, jsValue.Type);
      var nestedObj = jsValue.Object["NestedObj"].Object;
      Assert.IsTrue(nestedObj["NullValue"].IsNull);
      Assert.IsNotNull(nestedObj["ObjValue"].Object);
      Assert.IsNotNull(nestedObj["ArrayValue"].Array);
      Assert.AreEqual("Hello", nestedObj["StringValue"].String);
      Assert.AreEqual(true, nestedObj["BoolValue"].Boolean);
      Assert.AreEqual(42, nestedObj["IntValue"].Int64);
      Assert.AreEqual(4.5, nestedObj["DoubleValue"].Double);
    }

    [TestMethod]
    public void TestReadArray()
    {
      JArray jarr = JArray.Parse(@"[null, {}, [], ""Hello"", true, 42, 4.5]");
      IJSValueReader reader = new JTokenJSValueReader(jarr);

      JSValue jsValue = JSValue.ReadFrom(reader);
      Assert.AreEqual(JSValueType.Array, jsValue.Type);
      Assert.IsTrue(jsValue.Array[0].IsNull);
      Assert.IsNotNull(jsValue.Array[1].Object);
      Assert.IsNotNull(jsValue.Array[2].Array);
      Assert.AreEqual("Hello", jsValue.Array[3].String);
      Assert.AreEqual(true, jsValue.Array[4].Boolean);
      Assert.AreEqual(42, jsValue.Array[5].Int64);
      Assert.AreEqual(4.5, jsValue.Array[6].Double);
    }

    [TestMethod]
    public void TestReadNestedArray()
    {
      JArray jarr = JArray.Parse(@"[[null, {}, [], ""Hello"", true, 42, 4.5]]");
      IJSValueReader reader = new JTokenJSValueReader(jarr);

      JSValue jsValue = JSValue.ReadFrom(reader);
      Assert.AreEqual(JSValueType.Array, jsValue.Type);
      var nestedArr = jsValue.Array[0].Array;
      Assert.IsTrue(nestedArr[0].IsNull);
      Assert.IsNotNull(nestedArr[1].Object);
      Assert.IsNotNull(nestedArr[2].Array);
      Assert.AreEqual("Hello", nestedArr[3].String);
      Assert.AreEqual(true, nestedArr[4].Boolean);
      Assert.AreEqual(42, nestedArr[5].Int64);
      Assert.AreEqual(4.5, nestedArr[6].Double);
    }

    [TestMethod]
    public void TestObjectLiteral()
    {
      JSValue jsValue = new JSValueObject
      {
        ["NullValue"] = JSValue.Null,
        ["ObjValue"] = new JSValueObject { ["prop1"] = 2 },
        ["ObjValueEmpty"] = JSValue.EmptyObject,
        ["ArrayValue"] = new JSValueArray { 1, 2 },
        ["ArrayValueEmpty"] = JSValue.EmptyArray,
        ["StringValue"] = "Hello",
        ["BoolValue"] = true,
        ["IntValue"] = 42,
        ["DoubleValue"] = 4.5
      };

      Assert.AreEqual(JSValueType.Object, jsValue.Type);
      Assert.AreEqual(JSValueType.Null, jsValue["NullValue"].Type);
      Assert.AreEqual(JSValueType.Object, jsValue["ObjValue"].Type);
      Assert.AreEqual(JSValueType.Object, jsValue["ObjValueEmpty"].Type);
      Assert.AreEqual(JSValueType.Array, jsValue["ArrayValue"].Type);
      Assert.AreEqual(JSValueType.Array, jsValue["ArrayValueEmpty"].Type);
      Assert.AreEqual(JSValueType.String, jsValue["StringValue"].Type);
      Assert.AreEqual(JSValueType.Boolean, jsValue["BoolValue"].Type);
      Assert.AreEqual(JSValueType.Int64, jsValue["IntValue"].Type);
      Assert.AreEqual(JSValueType.Double, jsValue["DoubleValue"].Type);

      Assert.IsTrue(jsValue["NullValue"].IsNull);
      Assert.AreEqual(1, jsValue["ObjValue"].PropertyCount);
      Assert.AreEqual(2, jsValue["ObjValue"]["prop1"]);
      Assert.AreEqual(0, jsValue["ObjValueEmpty"].PropertyCount);
      Assert.AreEqual(2, jsValue["ArrayValue"].ItemCount);
      Assert.AreEqual(1, jsValue["ArrayValue"][0]);
      Assert.AreEqual(2, jsValue["ArrayValue"][1]);
      Assert.AreEqual(0, jsValue["ArrayValueEmpty"].ItemCount);
      Assert.AreEqual("Hello", jsValue["StringValue"]);
      Assert.AreEqual(true, jsValue["BoolValue"]);
      Assert.AreEqual(42, jsValue["IntValue"]);
      Assert.AreNotEqual(24, jsValue["IntValue"]);
      Assert.AreEqual(4.5, jsValue["DoubleValue"]);
    }

    [TestMethod]
    public void TestArrayLiteral()
    {
      JSValue jsValue = new JSValueArray
      {
        JSValue.Null,
        new JSValueObject { ["prop1"] = 2 },
        JSValue.EmptyObject,
        new JSValueArray { 1, 2 },
        JSValue.EmptyArray,
        "Hello",
        true,
        42,
        4.5
      };

      Assert.AreEqual(JSValueType.Array, jsValue.Type);
      Assert.AreEqual(JSValueType.Null, jsValue[0].Type);
      Assert.AreEqual(JSValueType.Object, jsValue[1].Type);
      Assert.AreEqual(JSValueType.Object, jsValue[2].Type);
      Assert.AreEqual(JSValueType.Array, jsValue[3].Type);
      Assert.AreEqual(JSValueType.Array, jsValue[4].Type);
      Assert.AreEqual(JSValueType.String, jsValue[5].Type);
      Assert.AreEqual(JSValueType.Boolean, jsValue[6].Type);
      Assert.AreEqual(JSValueType.Int64, jsValue[7].Type);
      Assert.AreEqual(JSValueType.Double, jsValue[8].Type);

      Assert.IsTrue(jsValue["NullValue"].IsNull);
      Assert.AreEqual(1, jsValue[1].PropertyCount);
      Assert.AreEqual(2, jsValue[1]["prop1"]);
      Assert.AreEqual(0, jsValue[2].PropertyCount);
      Assert.AreEqual(2, jsValue[3].ItemCount);
      Assert.AreEqual(1, jsValue[3][0]);
      Assert.AreEqual(2, jsValue[3][1]);
      Assert.AreEqual(0, jsValue[4].ItemCount);
      Assert.AreEqual("Hello", jsValue[5]);
      Assert.AreEqual(true, jsValue[6]);
      Assert.AreEqual(42, jsValue[7]);
      Assert.AreNotEqual(24, jsValue[7]);
      Assert.AreEqual(4.5, jsValue[8]);
    }

    [TestMethod]
    public void TestJSValueConstructor()
    {
      var value01 = new JSValue();
      var value02 = new JSValue(new JSValueObject { ["prop1"] = 3 });
      var value03 = new JSValue(new JSValueObject { });
      var value04 = new JSValue(new JSValueArray { 1, 2 });
      var value05 = new JSValue(new JSValueArray { });
      var value06 = new JSValue("Hello");
      var value07 = new JSValue(true);
      var value08 = new JSValue(false);
      var value09 = new JSValue(0);
      var value10 = new JSValue(42);
      var value11 = new JSValue(4.2);

      Assert.AreEqual(JSValueType.Null, value01.Type, "tag_a1001");
      Assert.AreEqual(JSValueType.Object, value02.Type, "tag_a1002");
      Assert.AreEqual(JSValueType.Object, value03.Type, "tag_a1003");
      Assert.AreEqual(JSValueType.Array, value04.Type, "tag_a1004");
      Assert.AreEqual(JSValueType.Array, value05.Type, "tag_a1005");
      Assert.AreEqual(JSValueType.String, value06.Type, "tag_a1006");
      Assert.AreEqual(JSValueType.Boolean, value07.Type, "tag_a1007");
      Assert.AreEqual(JSValueType.Boolean, value08.Type, "tag_a1008");
      Assert.AreEqual(JSValueType.Int64, value09.Type, "tag_a1009");
      Assert.AreEqual(JSValueType.Int64, value10.Type, "tag_a1010");
      Assert.AreEqual(JSValueType.Double, value11.Type, "tag_a1011");

      Assert.IsTrue(value01.IsNull, "tag_a2001");
      Assert.IsTrue(value02.TryGetObject(out var objValue02) && objValue02.Count == 1, "tag_a2002");
      Assert.IsTrue(value03.TryGetObject(out var objValue03) && objValue03.Count == 0, "tag_a2003");
      Assert.IsTrue(value04.TryGetArray(out var arrValue04) && arrValue04.Count == 2, "tag_a2004");
      Assert.IsTrue(value05.TryGetArray(out var arrValue05) && arrValue05.Count == 0, "tag_a2005");
      Assert.IsTrue(value06.TryGetString(out var strValue06) && strValue06 == "Hello", "tag_a2006");
      Assert.IsTrue(value07.TryGetBoolean(out var boolValue07) && boolValue07 == true, "tag_a2007");
      Assert.IsTrue(value08.TryGetBoolean(out var boolValue08) && boolValue08 == false, "tag_a2008");
      Assert.IsTrue(value09.TryGetInt64(out var intValue09) && intValue09 == 0, "tag_a2009");
      Assert.IsTrue(value10.TryGetInt64(out var intValue10) && intValue10 == 42, "tag_a2010");
      Assert.IsTrue(value11.TryGetDouble(out var doubleValue11) && doubleValue11 == 4.2, "tag_a2011");
    }

    [TestMethod]
    public void TestJSValueImplicitCast()
    {
      JSValue value01 = new ReadOnlyDictionary<string, JSValue>(new Dictionary<string, JSValue> { ["prop1"] = 3 });
      JSValue value02 = new Dictionary<string, JSValue> { ["prop1"] = 3 };
      JSValue value03 = new JSValueObject { ["prop1"] = 3 };
      JSValue value04 = new ReadOnlyCollection<JSValue>(new List<JSValue> { 1, 2 });
      JSValue value05 = new List<JSValue> { 1, 2 };
      JSValue value06 = new JSValueArray { 1, 2 };
      JSValue value07 = "Hello";
      JSValue value08 = true;
      JSValue value09 = false;
      JSValue value10 = (sbyte)42;
      JSValue value11 = (short)42;
      JSValue value12 = 42;
      JSValue value13 = (long)42;
      JSValue value14 = (byte)42;
      JSValue value15 = (ushort)42;
      JSValue value16 = (uint)42;
      JSValue value17 = (ulong)42;
      JSValue value18 = (float)4.2;
      JSValue value19 = 4.2;

      Assert.AreEqual(JSValueType.Object, value01.Type, "tag_b1001");
      Assert.AreEqual(JSValueType.Object, value02.Type, "tag_b1002");
      Assert.AreEqual(JSValueType.Object, value03.Type, "tag_b1003");
      Assert.AreEqual(JSValueType.Array, value04.Type, "tag_b1004");
      Assert.AreEqual(JSValueType.Array, value05.Type, "tag_b1005");
      Assert.AreEqual(JSValueType.Array, value06.Type, "tag_b1006");
      Assert.AreEqual(JSValueType.String, value07.Type, "tag_b1007");
      Assert.AreEqual(JSValueType.Boolean, value08.Type, "tag_b1008");
      Assert.AreEqual(JSValueType.Boolean, value09.Type, "tag_b1009");
      Assert.AreEqual(JSValueType.Int64, value10.Type, "tag_b1010");
      Assert.AreEqual(JSValueType.Int64, value11.Type, "tag_b1011");
      Assert.AreEqual(JSValueType.Int64, value12.Type, "tag_b1012");
      Assert.AreEqual(JSValueType.Int64, value13.Type, "tag_b1013");
      Assert.AreEqual(JSValueType.Int64, value14.Type, "tag_b1014");
      Assert.AreEqual(JSValueType.Int64, value15.Type, "tag_b1015");
      Assert.AreEqual(JSValueType.Int64, value16.Type, "tag_b1016");
      Assert.AreEqual(JSValueType.Int64, value17.Type, "tag_b1017");
      Assert.AreEqual(JSValueType.Double, value18.Type, "tag_b1018");
      Assert.AreEqual(JSValueType.Double, value19.Type, "tag_b1019");

      Assert.IsTrue(value01.TryGetObject(out var objValue01) && objValue01.Count == 1, "tag_b2001");
      Assert.IsTrue(value02.TryGetObject(out var objValue02) && objValue02.Count == 1, "tag_b2002");
      Assert.IsTrue(value03.TryGetObject(out var objValue03) && objValue03.Count == 1, "tag_b2003");
      Assert.IsTrue(value04.TryGetArray(out var arrValue04) && arrValue04.Count == 2, "tag_b2004");
      Assert.IsTrue(value05.TryGetArray(out var arrValue05) && arrValue05.Count == 2, "tag_b2005");
      Assert.IsTrue(value06.TryGetArray(out var arrValue06) && arrValue06.Count == 2, "tag_b2006");
      Assert.IsTrue(value07.TryGetString(out var strValue07) && strValue07 == "Hello", "tag_b2007");
      Assert.IsTrue(value08.TryGetBoolean(out var boolValue08) && boolValue08 == true, "tag_b2008");
      Assert.IsTrue(value09.TryGetBoolean(out var boolValue09) && boolValue09 == false, "tag_b2009");
      Assert.IsTrue(value10.TryGetInt64(out var intValue10) && intValue10 == 42, "tag_b2010");
      Assert.IsTrue(value11.TryGetInt64(out var intValue11) && intValue11 == 42, "tag_b2011");
      Assert.IsTrue(value12.TryGetInt64(out var intValue12) && intValue12 == 42, "tag_b2012");
      Assert.IsTrue(value13.TryGetInt64(out var intValue13) && intValue13 == 42, "tag_b2013");
      Assert.IsTrue(value14.TryGetInt64(out var intValue14) && intValue14 == 42, "tag_b2014");
      Assert.IsTrue(value15.TryGetInt64(out var intValue15) && intValue15 == 42, "tag_b2015");
      Assert.IsTrue(value16.TryGetInt64(out var intValue16) && intValue16 == 42, "tag_b2016");
      Assert.IsTrue(value17.TryGetInt64(out var intValue17) && intValue17 == 42, "tag_b2017");
      Assert.IsTrue(value18.TryGetDouble(out var doubleValue18) && doubleValue18 == (float)4.2, "tag_b2018");
      Assert.IsTrue(value19.TryGetDouble(out var doubleValue19) && doubleValue19 == 4.2, "tag_b2019");
    }

    private void CheckConversion(JSValue value, string stringValue, bool boolValue, double doubleValue)
    {
      Assert.AreEqual(stringValue, value.AsJSString());
      Assert.AreEqual(boolValue, value.AsJSBoolean());
      if (double.IsNaN(doubleValue))
      {
        Assert.IsTrue(double.IsNaN(value.AsJSNumber()));
      }
      else
      {
        Assert.AreEqual(doubleValue, value.AsJSNumber());
      }
    }

    [TestMethod]
    public void TestJSConverters()
    {
      CheckConversion(false, "false", false, 0);
      CheckConversion(true, "true", true, 1);
      CheckConversion(0, "0", false, 0);
      CheckConversion(1, "1", true, 1);
      CheckConversion("0", "0", true, 0);
      CheckConversion("000", "000", true, 0);
      CheckConversion("1", "1", true, 1);
      CheckConversion(double.NaN, "NaN", false, double.NaN);
      CheckConversion(double.PositiveInfinity, "Infinity", true, double.PositiveInfinity);
      CheckConversion(double.NegativeInfinity, "-Infinity", true, double.NegativeInfinity);
      CheckConversion("", "", false, 0);
      CheckConversion("20", "20", true, 20);
      CheckConversion("twenty", "twenty", true, double.NaN);
      CheckConversion(new JSValueArray { }, "", true, 0);
      CheckConversion(new JSValueArray { 20 }, "20", true, 20);
      CheckConversion(new JSValueArray { 10, 20 }, "10,20", true, double.NaN);
      CheckConversion(new JSValueArray { "twenty" }, "twenty", true, double.NaN);
      CheckConversion(new JSValueArray { "ten", "twenty" }, "ten,twenty", true, double.NaN);
      CheckConversion(new JSValueArray { double.NaN }, "NaN", true, double.NaN);
      CheckConversion(new JSValueObject { }, "[object Object]", true, double.NaN);
      CheckConversion(JSValue.Null, "null", false, 0);
    }

    [TestMethod]
    public void TestJSEquals()
    {
      Assert.IsTrue(new JSValue().JSEquals(JSValue.Null), "Assert 1001");
      Assert.IsFalse(new JSValue().JSEquals(new JSValueObject { }), "Assert 1002");
      Assert.IsFalse(new JSValue().JSEquals(new JSValueArray { }), "Assert 1003");
      Assert.IsFalse(new JSValue().JSEquals(""), "Assert 1004");
      Assert.IsFalse(new JSValue().JSEquals(false), "Assert 1005");
      Assert.IsFalse(new JSValue().JSEquals(0), "Assert 1006");
      Assert.IsFalse(new JSValue().JSEquals(0.0), "Assert 1007");

      Assert.IsFalse(new JSValue(new JSValueObject()).JSEquals(JSValue.Null), "Assert 2001");
      Assert.IsTrue(new JSValue(new JSValueObject { }).JSEquals(new JSValueObject { }), "Assert 2002");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { }), "Assert 2003");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "Hello" }), "Assert 2004");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { 0 }), "Assert 2005");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "0" }), "Assert 2006");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { 1 }), "Assert 2007");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "1" }), "Assert 2008");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { true }), "Assert 2009");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "true" }), "Assert 2010");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(""), "Assert 2011");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("0"), "Assert 2012");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("1"), "Assert 2013");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("true"), "Assert 2014");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("false"), "Assert 2015");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("Hello"), "Assert 2016");
      Assert.IsTrue(new JSValue(new JSValueObject { }).JSEquals("[object Object]"), "Assert 2017");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(false), "Assert 2018");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(true), "Assert 2019");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(0), "Assert 2020");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(5), "Assert 2021");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(1), "Assert 2022");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(0.0), "Assert 2023");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(0.5), "Assert 2024");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(1.0), "Assert 2025");

      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop2", 0 }, { "prop1", "2" } }), "Assert 3001");
      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop2", 0 } }), "Assert 3002");
      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop1", 2 }, { "prop25", false } }), "Assert 3003");
      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop1", 2 }, { "prop2", true } }), "Assert 3004");

      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(JSValue.Null), "Assert 4001");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueObject { }), "Assert 4002");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { }), "Assert 4003");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "Hello" }), "Assert 4004");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { 0 }), "Assert 4005");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "0" }), "Assert 4006");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { 1 }), "Assert 4007");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "1" }), "Assert 4008");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { true }), "Assert 4009");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "true" }), "Assert 4010");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(""), "Assert 4011");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("0"), "Assert 4012");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("1"), "Assert 4013");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("true"), "Assert 4014");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("false"), "Assert 4015");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("Hello"), "Assert 4016");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(false), "Assert 4017");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(true), "Assert 4018");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(0), "Assert 4019");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(5), "Assert 4020");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(1), "Assert 4021");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(0.0), "Assert 4022");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(0.5), "Assert 4023");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(1.0), "Assert 4024");

      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(JSValue.Null), "Assert 5001");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueObject { }), "Assert 5002");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { }), "Assert 5003");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "Hello" }), "Assert 5004");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { 0 }), "Assert 5005");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "0" }), "Assert 5006");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { 1 }), "Assert 5007");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "1" }), "Assert 5008");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { true }), "Assert 5009");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "true" }), "Assert 5010");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(""), "Assert 5011");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("0"), "Assert 5012");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals("1"), "Assert 5013");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("true"), "Assert 5014");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("false"), "Assert 5015");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("Hello"), "Assert 5016");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(false), "Assert 5017");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(true), "Assert 5018");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(0), "Assert 5019");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(5), "Assert 5020");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(1), "Assert 5021");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(0.0), "Assert 5022");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(0.5), "Assert 5023");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(1.0), "Assert 5024");

      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(JSValue.Null), "Assert 6001");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueObject { }), "Assert 6002");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { }), "Assert 6003");
      Assert.IsTrue(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "Hello" }), "Assert 6004");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { 0 }), "Assert 6005");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "0" }), "Assert 6006");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { 1 }), "Assert 6007");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "1" }), "Assert 6008");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { true }), "Assert 6009");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "true" }), "Assert 6010");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(""), "Assert 6011");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("0"), "Assert 6012");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("1"), "Assert 6013");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("true"), "Assert 6014");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("false"), "Assert 6015");
      Assert.IsTrue(new JSValue(new JSValueArray { "Hello" }).JSEquals("Hello"), "Assert 6016");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(false), "Assert 6017");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(true), "Assert 6018");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(0), "Assert 6019");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(5), "Assert 6020");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(1), "Assert 6021");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(0.0), "Assert 6022");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(0.5), "Assert 6023");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(1.0), "Assert 6024");

      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(JSValue.Null), "Assert 7001");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueObject { }), "Assert 7002");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { }), "Assert 7003");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "Hello" }), "Assert 7004");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { 0 }), "Assert 7005");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "0" }), "Assert 7006");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { 1 }), "Assert 7007");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "1" }), "Assert 7008");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { true }), "Assert 7009");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "true" }), "Assert 7010");
      Assert.IsTrue(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { 0, 1 }), "Assert 7011");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { false, true }), "Assert 7012");
      Assert.IsTrue(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "0", "1" }), "Assert 7013");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "0", true }), "Assert 7014");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(""), "Assert 7015");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("0"), "Assert 7016");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("1"), "Assert 7017");
      Assert.IsTrue(new JSValue(new JSValueArray { 0, 1 }).JSEquals("0,1"), "Assert 7018");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("true"), "Assert 7019");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("false"), "Assert 7020");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("Hello"), "Assert 7021");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(false), "Assert 7022");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(true), "Assert 7023");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(0), "Assert 7024");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(5), "Assert 7025");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(1), "Assert 7026");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(0.0), "Assert 7027");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(0.5), "Assert 7028");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(1.0), "Assert 7029");

      Assert.IsFalse(new JSValue("").JSEquals(JSValue.Null), "Assert 8001");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueObject { }), "Assert 8002");
      Assert.IsTrue(new JSValue("").JSEquals(new JSValueArray { }), "Assert 8003");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueArray { 0 }), "Assert 8004");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueArray { 1 }), "Assert 8005");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueArray { true }), "Assert 8006");
      Assert.IsTrue(new JSValue("").JSEquals(new JSValueArray { "" }), "Assert 8007");
      Assert.IsTrue(new JSValue("").JSEquals(""), "Assert 8008");
      Assert.IsFalse(new JSValue("").JSEquals("1"), "Assert 8009");
      Assert.IsFalse(new JSValue("").JSEquals("Hello"), "Assert 8010");
      Assert.IsTrue(new JSValue("").JSEquals(false), "Assert 8011");
      Assert.IsFalse(new JSValue("").JSEquals(true), "Assert 8012");
      Assert.IsTrue(new JSValue("").JSEquals(0), "Assert 8013");
      Assert.IsFalse(new JSValue("").JSEquals(5), "Assert 8014");
      Assert.IsFalse(new JSValue("").JSEquals(1), "Assert 8015");
      Assert.IsTrue(new JSValue("").JSEquals(0.0), "Assert 8016");
      Assert.IsFalse(new JSValue("").JSEquals(0.5), "Assert 8017");
      Assert.IsFalse(new JSValue("").JSEquals(1.0), "Assert 8018");

      Assert.IsFalse(new JSValue("Hello").JSEquals(JSValue.Null), "Assert 9001");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueObject { }), "Assert 9002");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { }), "Assert 9003");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { 0 }), "Assert 9004");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { 1 }), "Assert 9005");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { true }), "Assert 9006");
      Assert.IsTrue(new JSValue("Hello").JSEquals(new JSValueArray { "Hello" }), "Assert 9007");
      Assert.IsFalse(new JSValue("Hello").JSEquals(""), "Assert 9008");
      Assert.IsFalse(new JSValue("Hello").JSEquals("1"), "Assert 9009");
      Assert.IsTrue(new JSValue("Hello").JSEquals("Hello"), "Assert 9010");
      Assert.IsFalse(new JSValue("Hello").JSEquals(false), "Assert 9011");
      Assert.IsFalse(new JSValue("Hello").JSEquals(true), "Assert 9012");
      Assert.IsFalse(new JSValue("Hello").JSEquals(0), "Assert 9013");
      Assert.IsFalse(new JSValue("Hello").JSEquals(5), "Assert 9014");
      Assert.IsFalse(new JSValue("Hello").JSEquals(1), "Assert 9015");
      Assert.IsFalse(new JSValue("Hello").JSEquals(0.0), "Assert 9016");
      Assert.IsFalse(new JSValue("Hello").JSEquals(0.5), "Assert 9017");
      Assert.IsFalse(new JSValue("Hello").JSEquals(1.0), "Assert 9018");

      Assert.IsFalse(new JSValue("0").JSEquals(JSValue.Null), "Assert 10001");
      Assert.IsFalse(new JSValue("0").JSEquals(new JSValueObject { }), "Assert 10002");
      Assert.IsFalse(new JSValue("0").JSEquals(new JSValueArray { }), "Assert 10003");
      Assert.IsFalse(new JSValue("0").JSEquals(new JSValueArray { "Hello" }), "Assert 10004");
      Assert.IsTrue(new JSValue("0").JSEquals(new JSValueArray { "0" }), "Assert 10005");
      Assert.IsTrue(new JSValue("0").JSEquals(new JSValueArray { 0 }), "Assert 10006");
      Assert.IsFalse(new JSValue("0").JSEquals(""), "Assert 10007");
      Assert.IsTrue(new JSValue("0").JSEquals("0"), "Assert 10008");
      Assert.IsFalse(new JSValue("0").JSEquals("Hello"), "Assert 10009");
      Assert.IsTrue(new JSValue("0").JSEquals(false), "Assert 10010");
      Assert.IsFalse(new JSValue("0").JSEquals(true), "Assert 10011");
      Assert.IsTrue(new JSValue("0").JSEquals(0), "Assert 10012");
      Assert.IsFalse(new JSValue("0").JSEquals(5), "Assert 10013");
      Assert.IsFalse(new JSValue("0").JSEquals(1), "Assert 10014");
      Assert.IsTrue(new JSValue("0").JSEquals(0.0), "Assert 10015");
      Assert.IsFalse(new JSValue("0").JSEquals(0.5), "Assert 10016");
      Assert.IsFalse(new JSValue("0").JSEquals(1.0), "Assert 10017");

      Assert.IsFalse(new JSValue("1").JSEquals(JSValue.Null), "Assert 11001");
      Assert.IsFalse(new JSValue("1").JSEquals(new JSValueObject { }), "Assert 11002");
      Assert.IsFalse(new JSValue("1").JSEquals(new JSValueArray { }), "Assert 11003");
      Assert.IsFalse(new JSValue("1").JSEquals(new JSValueArray { "Hello" }), "Assert 11004");
      Assert.IsTrue(new JSValue("1").JSEquals(new JSValueArray { "1" }), "Assert 11005");
      Assert.IsTrue(new JSValue("1").JSEquals(new JSValueArray { 1 }), "Assert 11006");
      Assert.IsFalse(new JSValue("1").JSEquals(""), "Assert 11007");
      Assert.IsTrue(new JSValue("1").JSEquals("1"), "Assert 11008");
      Assert.IsFalse(new JSValue("1").JSEquals("Hello"), "Assert 11009");
      Assert.IsFalse(new JSValue("1").JSEquals(false), "Assert 11010");
      Assert.IsTrue(new JSValue("1").JSEquals(true), "Assert 11011");
      Assert.IsFalse(new JSValue("1").JSEquals(0), "Assert 11012");
      Assert.IsFalse(new JSValue("1").JSEquals(5), "Assert 11013");
      Assert.IsTrue(new JSValue("1").JSEquals(1), "Assert 11014");
      Assert.IsFalse(new JSValue("1").JSEquals(0.0), "Assert 11015");
      Assert.IsFalse(new JSValue("1").JSEquals(0.5), "Assert 11016");
      Assert.IsTrue(new JSValue("1").JSEquals(1.0), "Assert 11017");

      Assert.IsFalse(new JSValue("true").JSEquals(JSValue.Null), "Assert 12001");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueObject { }), "Assert 12002");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueArray { }), "Assert 12003");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueArray { 0 }), "Assert 12004");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueArray { 1 }), "Assert 12005");
      Assert.IsTrue(new JSValue("true").JSEquals(new JSValueArray { true }), "Assert 12006");
      Assert.IsTrue(new JSValue("true").JSEquals(new JSValueArray { "true" }), "Assert 12007");
      Assert.IsFalse(new JSValue("true").JSEquals(""), "Assert 12008");
      Assert.IsFalse(new JSValue("true").JSEquals("1"), "Assert 12009");
      Assert.IsFalse(new JSValue("true").JSEquals("Hello"), "Assert 12010");
      Assert.IsTrue(new JSValue("true").JSEquals("true"), "Assert 12011");
      Assert.IsFalse(new JSValue("true").JSEquals(false), "Assert 12012");
      Assert.IsFalse(new JSValue("true").JSEquals(true), "Assert 12013");
      Assert.IsFalse(new JSValue("true").JSEquals(0), "Assert 12014");
      Assert.IsFalse(new JSValue("true").JSEquals(5), "Assert 12015");
      Assert.IsFalse(new JSValue("true").JSEquals(1), "Assert 12016");
      Assert.IsFalse(new JSValue("true").JSEquals(0.0), "Assert 12017");
      Assert.IsFalse(new JSValue("true").JSEquals(0.5), "Assert 12018");
      Assert.IsFalse(new JSValue("true").JSEquals(1.0), "Assert 12019");

      Assert.IsTrue(new JSValue("[object Object]").JSEquals(new JSValueObject { }), "Assert 13001");

      Assert.IsFalse(new JSValue(true).JSEquals(JSValue.Null), "Assert 14001");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueObject { }), "Assert 14002");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { }), "Assert 14003");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { "Hello" }), "Assert 14004");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { 0 }), "Assert 14005");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { "0" }), "Assert 14006");
      Assert.IsTrue(new JSValue(true).JSEquals(new JSValueArray { 1 }), "Assert 14007");
      Assert.IsTrue(new JSValue(true).JSEquals(new JSValueArray { "1" }), "Assert 14008");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { true }), "Assert 14009");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { "true" }), "Assert 14010");
      Assert.IsFalse(new JSValue(true).JSEquals(""), "Assert 14011");
      Assert.IsFalse(new JSValue(true).JSEquals("0"), "Assert 14012");
      Assert.IsTrue(new JSValue(true).JSEquals("1"), "Assert 14013");
      Assert.IsFalse(new JSValue(true).JSEquals("true"), "Assert 14014");
      Assert.IsFalse(new JSValue(true).JSEquals("false"), "Assert 14015");
      Assert.IsFalse(new JSValue(true).JSEquals("Hello"), "Assert 14016");
      Assert.IsFalse(new JSValue(true).JSEquals(false), "Assert 14017");
      Assert.IsTrue(new JSValue(true).JSEquals(true), "Assert 14018");
      Assert.IsFalse(new JSValue(true).JSEquals(0), "Assert 14019");
      Assert.IsFalse(new JSValue(true).JSEquals(5), "Assert 14020");
      Assert.IsTrue(new JSValue(true).JSEquals(1), "Assert 14021");
      Assert.IsFalse(new JSValue(true).JSEquals(0.0), "Assert 14022");
      Assert.IsFalse(new JSValue(true).JSEquals(0.5), "Assert 14023");
      Assert.IsTrue(new JSValue(true).JSEquals(1.0), "Assert 14024");

      Assert.IsFalse(new JSValue(false).JSEquals(JSValue.Null), "Assert 15001");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueObject { }), "Assert 15002");
      Assert.IsTrue(new JSValue(false).JSEquals(new JSValueArray { }), "Assert 15003");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { "Hello" }), "Assert 15004");
      Assert.IsTrue(new JSValue(false).JSEquals(new JSValueArray { 0 }), "Assert 15005");
      Assert.IsTrue(new JSValue(false).JSEquals(new JSValueArray { "0" }), "Assert 15006");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { 1 }), "Assert 15007");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { "1" }), "Assert 15008");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { true }), "Assert 15009");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { "true" }), "Assert 15010");
      Assert.IsTrue(new JSValue(false).JSEquals(""), "Assert 15011");
      Assert.IsTrue(new JSValue(false).JSEquals("0"), "Assert 15012");
      Assert.IsFalse(new JSValue(false).JSEquals("1"), "Assert 15013");
      Assert.IsFalse(new JSValue(false).JSEquals("true"), "Assert 15014");
      Assert.IsFalse(new JSValue(false).JSEquals("false"), "Assert 15015");
      Assert.IsFalse(new JSValue(false).JSEquals("Hello"), "Assert 15016");
      Assert.IsTrue(new JSValue(false).JSEquals(false), "Assert 15017");
      Assert.IsFalse(new JSValue(false).JSEquals(true), "Assert 15018");
      Assert.IsTrue(new JSValue(false).JSEquals(0), "Assert 15019");
      Assert.IsFalse(new JSValue(false).JSEquals(5), "Assert 15020");
      Assert.IsFalse(new JSValue(false).JSEquals(1), "Assert 15021");
      Assert.IsTrue(new JSValue(false).JSEquals(0.0), "Assert 15022");
      Assert.IsFalse(new JSValue(false).JSEquals(0.5), "Assert 15023");
      Assert.IsFalse(new JSValue(false).JSEquals(1.0), "Assert 15024");

      Assert.IsFalse(new JSValue(0).JSEquals(JSValue.Null), "Assert 16001");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueObject { }), "Assert 16002");
      Assert.IsTrue(new JSValue(0).JSEquals(new JSValueArray { }), "Assert 16003");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { "Hello" }), "Assert 16004");
      Assert.IsTrue(new JSValue(0).JSEquals(new JSValueArray { 0 }), "Assert 16005");
      Assert.IsTrue(new JSValue(0).JSEquals(new JSValueArray { "0" }), "Assert 16006");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { 1 }), "Assert 16007");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { "1" }), "Assert 16008");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { true }), "Assert 16009");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { "true" }), "Assert 16010");
      Assert.IsTrue(new JSValue(0).JSEquals(""), "Assert 16011");
      Assert.IsTrue(new JSValue(0).JSEquals("0"), "Assert 16012");
      Assert.IsFalse(new JSValue(0).JSEquals("1"), "Assert 16013");
      Assert.IsFalse(new JSValue(0).JSEquals("true"), "Assert 16014");
      Assert.IsFalse(new JSValue(0).JSEquals("false"), "Assert 16015");
      Assert.IsFalse(new JSValue(0).JSEquals("Hello"), "Assert 16016");
      Assert.IsTrue(new JSValue(0).JSEquals(false), "Assert 16017");
      Assert.IsFalse(new JSValue(0).JSEquals(true), "Assert 16018");
      Assert.IsTrue(new JSValue(0).JSEquals(0), "Assert 16019");
      Assert.IsFalse(new JSValue(0).JSEquals(5), "Assert 16020");
      Assert.IsFalse(new JSValue(0).JSEquals(1), "Assert 16021");
      Assert.IsTrue(new JSValue(0).JSEquals(0.0), "Assert 16022");
      Assert.IsFalse(new JSValue(0).JSEquals(0.5), "Assert 16023");
      Assert.IsFalse(new JSValue(0).JSEquals(1.0), "Assert 16024");

      Assert.IsFalse(new JSValue(1).JSEquals(JSValue.Null), "Assert 17001");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueObject { }), "Assert 17002");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { }), "Assert 17003");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { "Hello" }), "Assert 17004");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { 0 }), "Assert 17005");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { "0" }), "Assert 17006");
      Assert.IsTrue(new JSValue(1).JSEquals(new JSValueArray { 1 }), "Assert 17007");
      Assert.IsTrue(new JSValue(1).JSEquals(new JSValueArray { "1" }), "Assert 17008");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { true }), "Assert 17009");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { "true" }), "Assert 17010");
      Assert.IsFalse(new JSValue(1).JSEquals(""), "Assert 17011");
      Assert.IsFalse(new JSValue(1).JSEquals("0"), "Assert 17012");
      Assert.IsTrue(new JSValue(1).JSEquals("1"), "Assert 17013");
      Assert.IsFalse(new JSValue(1).JSEquals("true"), "Assert 17014");
      Assert.IsFalse(new JSValue(1).JSEquals("false"), "Assert 17015");
      Assert.IsFalse(new JSValue(1).JSEquals("Hello"), "Assert 17016");
      Assert.IsFalse(new JSValue(1).JSEquals(false), "Assert 17017");
      Assert.IsTrue(new JSValue(1).JSEquals(true), "Assert 17018");
      Assert.IsFalse(new JSValue(1).JSEquals(0), "Assert 17019");
      Assert.IsFalse(new JSValue(1).JSEquals(5), "Assert 17020");
      Assert.IsTrue(new JSValue(1).JSEquals(1), "Assert 17021");
      Assert.IsFalse(new JSValue(1).JSEquals(0.0), "Assert 17022");
      Assert.IsFalse(new JSValue(1).JSEquals(0.5), "Assert 17023");
      Assert.IsTrue(new JSValue(1).JSEquals(1.0), "Assert 17024");

      Assert.IsFalse(new JSValue(5).JSEquals(JSValue.Null), "Assert 18001");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueObject { }), "Assert 18002");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { }), "Assert 18003");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "Hello" }), "Assert 18004");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { 0 }), "Assert 18005");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "0" }), "Assert 18006");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { 1 }), "Assert 18007");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "1" }), "Assert 18008");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { true }), "Assert 18009");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "true" }), "Assert 18010");
      Assert.IsFalse(new JSValue(5).JSEquals(""), "Assert 18011");
      Assert.IsFalse(new JSValue(5).JSEquals("0"), "Assert 18012");
      Assert.IsFalse(new JSValue(5).JSEquals("1"), "Assert 18013");
      Assert.IsFalse(new JSValue(5).JSEquals("true"), "Assert 18014");
      Assert.IsFalse(new JSValue(5).JSEquals("false"), "Assert 18015");
      Assert.IsFalse(new JSValue(5).JSEquals("Hello"), "Assert 18016");
      Assert.IsFalse(new JSValue(5).JSEquals(false), "Assert 18017");
      Assert.IsFalse(new JSValue(5).JSEquals(true), "Assert 18018");
      Assert.IsFalse(new JSValue(5).JSEquals(0), "Assert 18019");
      Assert.IsTrue(new JSValue(5).JSEquals(5), "Assert 18020");
      Assert.IsFalse(new JSValue(5).JSEquals(1), "Assert 18021");
      Assert.IsFalse(new JSValue(5).JSEquals(0.0), "Assert 18022");
      Assert.IsFalse(new JSValue(5).JSEquals(0.5), "Assert 18023");
      Assert.IsFalse(new JSValue(5).JSEquals(1.0), "Assert 18024");

      Assert.IsFalse(new JSValue(0.0).JSEquals(JSValue.Null), "Assert 19001");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueObject { }), "Assert 19002");
      Assert.IsTrue(new JSValue(0.0).JSEquals(new JSValueArray { }), "Assert 19003");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { "Hello" }), "Assert 19004");
      Assert.IsTrue(new JSValue(0.0).JSEquals(new JSValueArray { 0 }), "Assert 19005");
      Assert.IsTrue(new JSValue(0.0).JSEquals(new JSValueArray { "0" }), "Assert 19006");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { 1 }), "Assert 19007");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { "1" }), "Assert 19008");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { true }), "Assert 19009");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { "true" }), "Assert 19010");
      Assert.IsTrue(new JSValue(0.0).JSEquals(""), "Assert 19011");
      Assert.IsTrue(new JSValue(0.0).JSEquals("0"), "Assert 19012");
      Assert.IsFalse(new JSValue(0.0).JSEquals("1"), "Assert 19013");
      Assert.IsFalse(new JSValue(0.0).JSEquals("true"), "Assert 19014");
      Assert.IsFalse(new JSValue(0.0).JSEquals("false"), "Assert 19015");
      Assert.IsFalse(new JSValue(0.0).JSEquals("Hello"), "Assert 19016");
      Assert.IsTrue(new JSValue(0.0).JSEquals(false), "Assert 19017");
      Assert.IsFalse(new JSValue(0.0).JSEquals(true), "Assert 19018");
      Assert.IsTrue(new JSValue(0.0).JSEquals(0), "Assert 19019");
      Assert.IsFalse(new JSValue(0.0).JSEquals(5), "Assert 19020");
      Assert.IsFalse(new JSValue(0.0).JSEquals(1), "Assert 19021");
      Assert.IsTrue(new JSValue(0.0).JSEquals(0.0), "Assert 19022");
      Assert.IsFalse(new JSValue(0.0).JSEquals(0.5), "Assert 19023");
      Assert.IsFalse(new JSValue(0.0).JSEquals(1.0), "Assert 19024");

      Assert.IsFalse(new JSValue(1.0).JSEquals(JSValue.Null), "Assert 20001");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueObject { }), "Assert 20002");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { }), "Assert 20003");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { "Hello" }), "Assert 20004");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { 0 }), "Assert 20005");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { "0" }), "Assert 20006");
      Assert.IsTrue(new JSValue(1.0).JSEquals(new JSValueArray { 1 }), "Assert 20007");
      Assert.IsTrue(new JSValue(1.0).JSEquals(new JSValueArray { "1" }), "Assert 20008");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { true }), "Assert 20009");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { "true" }), "Assert 20010");
      Assert.IsFalse(new JSValue(1.0).JSEquals(""), "Assert 20011");
      Assert.IsFalse(new JSValue(1.0).JSEquals("0"), "Assert 20012");
      Assert.IsTrue(new JSValue(1.0).JSEquals("1"), "Assert 20013");
      Assert.IsFalse(new JSValue(1.0).JSEquals("true"), "Assert 20014");
      Assert.IsFalse(new JSValue(1.0).JSEquals("false"), "Assert 20015");
      Assert.IsFalse(new JSValue(1.0).JSEquals("Hello"), "Assert 20016");
      Assert.IsFalse(new JSValue(1.0).JSEquals(false), "Assert 20017");
      Assert.IsTrue(new JSValue(1.0).JSEquals(true), "Assert 20018");
      Assert.IsFalse(new JSValue(1.0).JSEquals(0), "Assert 20019");
      Assert.IsFalse(new JSValue(1.0).JSEquals(5), "Assert 20020");
      Assert.IsTrue(new JSValue(1.0).JSEquals(1), "Assert 20021");
      Assert.IsFalse(new JSValue(1.0).JSEquals(0.0), "Assert 20022");
      Assert.IsFalse(new JSValue(1.0).JSEquals(0.5), "Assert 20023");
      Assert.IsTrue(new JSValue(1.0).JSEquals(1.0), "Assert 20024");

      Assert.IsFalse(new JSValue(0.5).JSEquals(JSValue.Null), "Assert 21001");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueObject { }), "Assert 21002");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { }), "Assert 21003");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "Hello" }), "Assert 21004");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { 0 }), "Assert 21005");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "0" }), "Assert 21006");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { 1 }), "Assert 21007");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "1" }), "Assert 21008");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { true }), "Assert 21009");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "true" }), "Assert 21010");
      Assert.IsFalse(new JSValue(0.5).JSEquals(""), "Assert 21011");
      Assert.IsFalse(new JSValue(0.5).JSEquals("0"), "Assert 21012");
      Assert.IsFalse(new JSValue(0.5).JSEquals("1"), "Assert 21013");
      Assert.IsFalse(new JSValue(0.5).JSEquals("true"), "Assert 21014");
      Assert.IsFalse(new JSValue(0.5).JSEquals("false"), "Assert 21015");
      Assert.IsFalse(new JSValue(0.5).JSEquals("Hello"), "Assert 21016");
      Assert.IsFalse(new JSValue(0.5).JSEquals(false), "Assert 21017");
      Assert.IsFalse(new JSValue(0.5).JSEquals(true), "Assert 21018");
      Assert.IsFalse(new JSValue(0.5).JSEquals(0), "Assert 21019");
      Assert.IsFalse(new JSValue(0.5).JSEquals(5), "Assert 21020");
      Assert.IsFalse(new JSValue(0.5).JSEquals(1), "Assert 21021");
      Assert.IsFalse(new JSValue(0.5).JSEquals(0.0), "Assert 21022");
      Assert.IsTrue(new JSValue(0.5).JSEquals(0.5), "Assert 21023");
      Assert.IsFalse(new JSValue(0.5).JSEquals(1.0), "Assert 21024");
    }
  }
}

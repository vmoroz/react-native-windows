// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using Newtonsoft.Json.Linq;

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

    private void CheckConversion(
    JSValue value, string stringValue, bool boolValue, double doubleValue)
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
  }
}

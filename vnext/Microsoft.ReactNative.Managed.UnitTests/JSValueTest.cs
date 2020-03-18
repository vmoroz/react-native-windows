// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.VisualStudio.TestTools.UnitTesting;
using Newtonsoft.Json.Linq;
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
              ""NullValue"": null,
              ""ObjValue"": {""prop1"": 2},
              ""ArrayValue"": [1, 2],
              ""StringValue"": ""Hello"",
              ""BoolValue"": true,
              ""IntValue"": 42,
              ""DoubleValue"": 4.5
            }");
      IJSValueReader reader = new JTokenJSValueReader(jobj);
      JSValue jsValue = JSValue.ReadFrom(reader);

      Assert.AreEqual(JSValueType.Object, jsValue.Type, "tag_a101");
      Assert.AreEqual(JSValueType.Null, jsValue["NullValue"].Type, "tag_1a02");
      Assert.AreEqual(JSValueType.Object, jsValue["ObjValue"].Type, "tag_a103");
      Assert.AreEqual(JSValueType.Array, jsValue["ArrayValue"].Type, "tag_a104");
      Assert.AreEqual(JSValueType.String, jsValue["StringValue"].Type, "tag_a105");
      Assert.AreEqual(JSValueType.Boolean, jsValue["BoolValue"].Type, "tag_a106");
      Assert.AreEqual(JSValueType.Int64, jsValue["IntValue"].Type, "tag_a107");
      Assert.AreEqual(JSValueType.Double, jsValue["DoubleValue"].Type, "tag_a108");

      Assert.IsTrue(jsValue["NullValue"].IsNull, "tag_a201");
      Assert.IsTrue(jsValue["ObjValue"].TryGetObject(out var objValue), "tag_a202");
      Assert.IsTrue(jsValue["ArrayValue"].TryGetArray(out var arrayValue), "tag_a203");
      Assert.IsTrue(jsValue["StringValue"].TryGetString(out var stringValue), "tag_a204");
      Assert.IsTrue(jsValue["BoolValue"].TryGetBoolean(out var boolValue), "tag_a205");
      Assert.IsTrue(jsValue["IntValue"].TryGetInt64(out var intValue), "tag_a206");
      Assert.IsTrue(jsValue["DoubleValue"].TryGetDouble(out var doubleValue), "tag_a207");

      Assert.AreEqual(1, objValue.Count, "tag_a301");
      Assert.AreEqual(1, jsValue["ObjValue"].PropertyCount, "tag_a302");
      Assert.AreEqual(2, arrayValue.Count, "tag_a303");
      Assert.AreEqual(2, jsValue["ArrayValue"].ItemCount, "tag_a304");
      Assert.AreEqual("Hello", stringValue, "tag_a305");
      Assert.AreEqual(true, boolValue, "tag_a306");
      Assert.AreEqual(42, intValue, "tag_a307");
      Assert.AreEqual(4.5, doubleValue, "tag_a308");
    }

    [TestMethod]
    public void TestReadNestedObject()
    {
      JObject jobj = JObject.Parse(@"{
              ""NestedObj"": {
                ""NullValue"": null,
                ""ObjValue"": {},
                ""ArrayValue"": [],
                ""StringValue"": ""Hello"",
                ""BoolValue"": true,
                ""IntValue"": 42,
                ""DoubleValue"": 4.5
              }
            }");
      IJSValueReader reader = new JTokenJSValueReader(jobj);
      JSValue jsValue = JSValue.ReadFrom(reader);

      Assert.AreEqual(JSValueType.Object, jsValue.Type, "tag_b101");
      Assert.AreEqual(JSValueType.Object, jsValue["NestedObj"].Type, "tag_b102");
      jsValue["NestedObj"].TryGetObject(out var nestedObj);

      Assert.AreEqual(JSValueType.Null, nestedObj["NullValue"].Type, "tag_b201");
      Assert.AreEqual(JSValueType.Object, nestedObj["ObjValue"].Type, "tag_b202");
      Assert.AreEqual(JSValueType.Array, nestedObj["ArrayValue"].Type, "tag_b203");
      Assert.AreEqual(JSValueType.String, nestedObj["StringValue"].Type, "tag_b204");
      Assert.AreEqual(JSValueType.Boolean, nestedObj["BoolValue"].Type, "tag_b205");
      Assert.AreEqual(JSValueType.Int64, nestedObj["IntValue"].Type, "tag_b206");
      Assert.AreEqual(JSValueType.Double, nestedObj["DoubleValue"].Type, "tag_b207");

      Assert.IsTrue(nestedObj["NullValue"].IsNull, "tag_b301");
      Assert.IsTrue(nestedObj["ObjValue"].TryGetObject(out var _), "tag_b302");
      Assert.IsTrue(nestedObj["ArrayValue"].TryGetArray(out var _), "tag_b303");
      Assert.AreEqual("Hello", nestedObj["StringValue"], "tag_b304");
      Assert.AreEqual(true, nestedObj["BoolValue"], "tag_b305");
      Assert.AreEqual(42, nestedObj["IntValue"], "tag_b306");
      Assert.AreEqual(4.5, nestedObj["DoubleValue"], "tag_b307");
    }

    [TestMethod]
    public void TestReadArray()
    {
      JArray jarr = JArray.Parse(@"[null, {}, [], ""Hello"", true, 42, 4.5]");
      IJSValueReader reader = new JTokenJSValueReader(jarr);
      JSValue jsValue = JSValue.ReadFrom(reader);

      Assert.AreEqual(JSValueType.Array, jsValue.Type, "tag_c101");
      Assert.AreEqual(JSValueType.Null, jsValue[0].Type, "tag_c102");
      Assert.AreEqual(JSValueType.Object, jsValue[1].Type, "tag_c103");
      Assert.AreEqual(JSValueType.Array, jsValue[2].Type, "tag_c104");
      Assert.AreEqual(JSValueType.String, jsValue[3].Type, "tag_c105");
      Assert.AreEqual(JSValueType.Boolean, jsValue[4].Type, "tag_c106");
      Assert.AreEqual(JSValueType.Int64, jsValue[5].Type, "tag_c107");
      Assert.AreEqual(JSValueType.Double, jsValue[6].Type, "tag_c108");

      Assert.IsTrue(jsValue[0].IsNull, "tag_c201");
      Assert.IsTrue(jsValue[1].TryGetObject(out var _), "tag_c202");
      Assert.IsTrue(jsValue[2].TryGetArray(out var _), "tag_c203");
      Assert.AreEqual("Hello", jsValue[3], "tag_c204");
      Assert.AreEqual(true, jsValue[4], "tag_c205");
      Assert.AreEqual(42, jsValue[5], "tag_c206");
      Assert.AreEqual(4.5, jsValue[6], "tag_c207");
    }

    [TestMethod]
    public void TestReadNestedArray()
    {
      JArray jarr = JArray.Parse(@"[[null, {}, [], ""Hello"", true, 42, 4.5]]");
      IJSValueReader reader = new JTokenJSValueReader(jarr);
      JSValue jsValue = JSValue.ReadFrom(reader);

      Assert.AreEqual(JSValueType.Array, jsValue.Type, "tag_d101");
      Assert.AreEqual(JSValueType.Array, jsValue[0].Type, "tag_d102");
      jsValue[0].TryGetArray(out var nestedArr);

      Assert.AreEqual(JSValueType.Null, nestedArr[0].Type, "tag_d201");
      Assert.AreEqual(JSValueType.Object, nestedArr[1].Type, "tag_d202");
      Assert.AreEqual(JSValueType.Array, nestedArr[2].Type, "tag_d203");
      Assert.AreEqual(JSValueType.String, nestedArr[3].Type, "tag_d204");
      Assert.AreEqual(JSValueType.Boolean, nestedArr[4].Type, "tag_d205");
      Assert.AreEqual(JSValueType.Int64, nestedArr[5].Type, "tag_d206");
      Assert.AreEqual(JSValueType.Double, nestedArr[6].Type, "tag_d207");

      Assert.IsTrue(nestedArr[0].IsNull, "tag_d301");
      Assert.IsTrue(nestedArr[1].TryGetObject(out var _), "tag_d302");
      Assert.IsTrue(nestedArr[2].TryGetArray(out var _), "tag_d303");
      Assert.AreEqual("Hello", nestedArr[3], "tag_d304");
      Assert.AreEqual(true, nestedArr[4], "tag_d305");
      Assert.AreEqual(42, nestedArr[5], "tag_d306");
      Assert.AreEqual(4.5, nestedArr[6], "tag_d307");
    }

    [TestMethod]
    public void TestJSSimpleLiterals()
    {
      JSValue jsValue01 = JSValue.Null;
      JSValue jsValue02 = "Hello";
      JSValue jsValue03 = "";
      JSValue jsValue04 = true;
      JSValue jsValue05 = false;
      JSValue jsValue06 = 42;
      JSValue jsValue07 = 0;
      JSValue jsValue08 = 4.5;
      JSValue jsValue09 = 0.0;
      JSValue jsValue10 = double.NaN;
      JSValue jsValue11 = double.PositiveInfinity;
      JSValue jsValue12 = double.NegativeInfinity;

      Assert.AreEqual(JSValueType.Null, jsValue01.Type, "tag_e101");
      Assert.AreEqual(JSValueType.String, jsValue02.Type, "tag_e102");
      Assert.AreEqual(JSValueType.String, jsValue03.Type, "tag_e103");
      Assert.AreEqual(JSValueType.Boolean, jsValue04.Type, "tag_e104");
      Assert.AreEqual(JSValueType.Boolean, jsValue05.Type, "tag_e105");
      Assert.AreEqual(JSValueType.Int64, jsValue06.Type, "tag_e106");
      Assert.AreEqual(JSValueType.Int64, jsValue07.Type, "tag_e107");
      Assert.AreEqual(JSValueType.Double, jsValue08.Type, "tag_e108");
      Assert.AreEqual(JSValueType.Double, jsValue09.Type, "tag_e109");
      Assert.AreEqual(JSValueType.Double, jsValue10.Type, "tag_e110");
      Assert.AreEqual(JSValueType.Double, jsValue11.Type, "tag_e111");
      Assert.AreEqual(JSValueType.Double, jsValue12.Type, "tag_e112");

      Assert.IsTrue(jsValue01.IsNull, "tag_e201");
      Assert.IsTrue(jsValue02.TryGetString(out var str1) && str1 == "Hello", "tag_e202");
      Assert.IsTrue(jsValue03.TryGetString(out var str2) && str2 == "", "tag_e203");
      Assert.IsTrue(jsValue04.TryGetBoolean(out var bool1) && bool1 == true, "tag_e204");
      Assert.IsTrue(jsValue05.TryGetBoolean(out var bool2) && bool2 == false, "tag_e205");
      Assert.IsTrue(jsValue06.TryGetInt64(out var int1) && int1 == 42, "tag_e206");
      Assert.IsTrue(jsValue07.TryGetInt64(out var int2) && int2 == 0, "tag_e207");
      Assert.IsTrue(jsValue08.TryGetDouble(out var double1) && double1 == 4.5, "tag_e208");
      Assert.IsTrue(jsValue09.TryGetDouble(out var double2) && double2 == 0, "tag_e209");
      Assert.IsTrue(jsValue10.TryGetDouble(out var double3) && double.IsNaN(double3), "tag_e210");
      Assert.IsTrue(jsValue11.TryGetDouble(out var double4) && double4 == double.PositiveInfinity, "tag_e211");
      Assert.IsTrue(jsValue12.TryGetDouble(out var double5) && double5 == double.NegativeInfinity, "tag_e212");
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

      Assert.AreEqual(JSValueType.Object, jsValue.Type, "tag_f101");
      Assert.AreEqual(JSValueType.Null, jsValue["NullValue"].Type, "tag_f102");
      Assert.AreEqual(JSValueType.Object, jsValue["ObjValue"].Type, "tag_f103");
      Assert.AreEqual(JSValueType.Object, jsValue["ObjValueEmpty"].Type, "tag_f104");
      Assert.AreEqual(JSValueType.Array, jsValue["ArrayValue"].Type, "tag_f105");
      Assert.AreEqual(JSValueType.Array, jsValue["ArrayValueEmpty"].Type, "tag_f106");
      Assert.AreEqual(JSValueType.String, jsValue["StringValue"].Type, "tag_f107");
      Assert.AreEqual(JSValueType.Boolean, jsValue["BoolValue"].Type, "tag_f108");
      Assert.AreEqual(JSValueType.Int64, jsValue["IntValue"].Type, "tag_f109");
      Assert.AreEqual(JSValueType.Double, jsValue["DoubleValue"].Type, "tag_f110");

      Assert.IsTrue(jsValue["NullValue"].IsNull, "tag_f201");
      Assert.AreEqual(1, jsValue["ObjValue"].PropertyCount, "tag_f202");
      Assert.AreEqual(2, jsValue["ObjValue"]["prop1"], "tag_f203");
      Assert.AreEqual(0, jsValue["ObjValueEmpty"].PropertyCount, "tag_f204");
      Assert.AreEqual(2, jsValue["ArrayValue"].ItemCount, "tag_f205");
      Assert.AreEqual(1, jsValue["ArrayValue"][0], "tag_f206");
      Assert.AreEqual(2, jsValue["ArrayValue"][1], "tag_f207");
      Assert.AreEqual(0, jsValue["ArrayValueEmpty"].ItemCount, "tag_f208");
      Assert.AreEqual("Hello", jsValue["StringValue"], "tag_f209");
      Assert.AreEqual(true, jsValue["BoolValue"], "tag_f210");
      Assert.AreEqual(42, jsValue["IntValue"], "tag_f211");
      Assert.AreNotEqual(24, jsValue["IntValue"], "tag_f212");
      Assert.AreEqual(4.5, jsValue["DoubleValue"], "tag_f213");
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

      Assert.AreEqual(JSValueType.Array, jsValue.Type, "tag_g101");
      Assert.AreEqual(JSValueType.Null, jsValue[0].Type, "tag_g102");
      Assert.AreEqual(JSValueType.Object, jsValue[1].Type, "tag_g103");
      Assert.AreEqual(JSValueType.Object, jsValue[2].Type, "tag_g104");
      Assert.AreEqual(JSValueType.Array, jsValue[3].Type, "tag_g105");
      Assert.AreEqual(JSValueType.Array, jsValue[4].Type, "tag_g106");
      Assert.AreEqual(JSValueType.String, jsValue[5].Type, "tag_g107");
      Assert.AreEqual(JSValueType.Boolean, jsValue[6].Type, "tag_g108");
      Assert.AreEqual(JSValueType.Int64, jsValue[7].Type, "tag_g109");
      Assert.AreEqual(JSValueType.Double, jsValue[8].Type, "tag_g110");

      Assert.IsTrue(jsValue["NullValue"].IsNull, "tag_g201");
      Assert.AreEqual(1, jsValue[1].PropertyCount, "tag_g202");
      Assert.AreEqual(2, jsValue[1]["prop1"], "tag_g203");
      Assert.AreEqual(0, jsValue[2].PropertyCount, "tag_g204");
      Assert.AreEqual(2, jsValue[3].ItemCount, "tag_g205");
      Assert.AreEqual(1, jsValue[3][0], "tag_g206");
      Assert.AreEqual(2, jsValue[3][1], "tag_g207");
      Assert.AreEqual(0, jsValue[4].ItemCount, "tag_g208");
      Assert.AreEqual("Hello", jsValue[5], "tag_g209");
      Assert.AreEqual(true, jsValue[6], "tag_g210");
      Assert.AreEqual(42, jsValue[7], "tag_g211");
      Assert.AreNotEqual(24, jsValue[7], "tag_g212");
      Assert.AreEqual(4.5, jsValue[8], "tag_g213");
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

      Assert.AreEqual(JSValueType.Null, value01.Type, "tag_h101");
      Assert.AreEqual(JSValueType.Object, value02.Type, "tag_h102");
      Assert.AreEqual(JSValueType.Object, value03.Type, "tag_h103");
      Assert.AreEqual(JSValueType.Array, value04.Type, "tag_h104");
      Assert.AreEqual(JSValueType.Array, value05.Type, "tag_h105");
      Assert.AreEqual(JSValueType.String, value06.Type, "tag_h106");
      Assert.AreEqual(JSValueType.Boolean, value07.Type, "tag_h107");
      Assert.AreEqual(JSValueType.Boolean, value08.Type, "tag_h108");
      Assert.AreEqual(JSValueType.Int64, value09.Type, "tag_h109");
      Assert.AreEqual(JSValueType.Int64, value10.Type, "tag_h110");
      Assert.AreEqual(JSValueType.Double, value11.Type, "tag_h111");

      Assert.IsTrue(value01.IsNull, "tag_h2001");
      Assert.IsTrue(value02.TryGetObject(out var objValue02) && objValue02.Count == 1, "tag_h202");
      Assert.IsTrue(value03.TryGetObject(out var objValue03) && objValue03.Count == 0, "tag_h203");
      Assert.IsTrue(value04.TryGetArray(out var arrValue04) && arrValue04.Count == 2, "tag_h204");
      Assert.IsTrue(value05.TryGetArray(out var arrValue05) && arrValue05.Count == 0, "tag_h205");
      Assert.IsTrue(value06.TryGetString(out var strValue06) && strValue06 == "Hello", "tag_h206");
      Assert.IsTrue(value07.TryGetBoolean(out var boolValue07) && boolValue07 == true, "tag_h207");
      Assert.IsTrue(value08.TryGetBoolean(out var boolValue08) && boolValue08 == false, "tag_h208");
      Assert.IsTrue(value09.TryGetInt64(out var intValue09) && intValue09 == 0, "tag_h209");
      Assert.IsTrue(value10.TryGetInt64(out var intValue10) && intValue10 == 42, "tag_h210");
      Assert.IsTrue(value11.TryGetDouble(out var doubleValue11) && doubleValue11 == 4.2, "tag_h211");
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

      Assert.AreEqual(JSValueType.Object, value01.Type, "tag_i101");
      Assert.AreEqual(JSValueType.Object, value02.Type, "tag_i102");
      Assert.AreEqual(JSValueType.Object, value03.Type, "tag_i103");
      Assert.AreEqual(JSValueType.Array, value04.Type, "tag_i104");
      Assert.AreEqual(JSValueType.Array, value05.Type, "tag_i105");
      Assert.AreEqual(JSValueType.Array, value06.Type, "tag_i106");
      Assert.AreEqual(JSValueType.String, value07.Type, "tag_i107");
      Assert.AreEqual(JSValueType.Boolean, value08.Type, "tag_i108");
      Assert.AreEqual(JSValueType.Boolean, value09.Type, "tag_i109");
      Assert.AreEqual(JSValueType.Int64, value10.Type, "tag_i110");
      Assert.AreEqual(JSValueType.Int64, value11.Type, "tag_i111");
      Assert.AreEqual(JSValueType.Int64, value12.Type, "tag_i112");
      Assert.AreEqual(JSValueType.Int64, value13.Type, "tag_i113");
      Assert.AreEqual(JSValueType.Int64, value14.Type, "tag_i114");
      Assert.AreEqual(JSValueType.Int64, value15.Type, "tag_i115");
      Assert.AreEqual(JSValueType.Int64, value16.Type, "tag_i116");
      Assert.AreEqual(JSValueType.Int64, value17.Type, "tag_i117");
      Assert.AreEqual(JSValueType.Double, value18.Type, "tag_i118");
      Assert.AreEqual(JSValueType.Double, value19.Type, "tag_i119");

      Assert.IsTrue(value01.TryGetObject(out var objValue01) && objValue01.Count == 1, "tag_i201");
      Assert.IsTrue(value02.TryGetObject(out var objValue02) && objValue02.Count == 1, "tag_i202");
      Assert.IsTrue(value03.TryGetObject(out var objValue03) && objValue03.Count == 1, "tag_i203");
      Assert.IsTrue(value04.TryGetArray(out var arrValue04) && arrValue04.Count == 2, "tag_i204");
      Assert.IsTrue(value05.TryGetArray(out var arrValue05) && arrValue05.Count == 2, "tag_i205");
      Assert.IsTrue(value06.TryGetArray(out var arrValue06) && arrValue06.Count == 2, "tag_i206");
      Assert.IsTrue(value07.TryGetString(out var strValue07) && strValue07 == "Hello", "tag_i207");
      Assert.IsTrue(value08.TryGetBoolean(out var boolValue08) && boolValue08 == true, "tag_i208");
      Assert.IsTrue(value09.TryGetBoolean(out var boolValue09) && boolValue09 == false, "tag_i209");
      Assert.IsTrue(value10.TryGetInt64(out var intValue10) && intValue10 == 42, "tag_i210");
      Assert.IsTrue(value11.TryGetInt64(out var intValue11) && intValue11 == 42, "tag_i211");
      Assert.IsTrue(value12.TryGetInt64(out var intValue12) && intValue12 == 42, "tag_i212");
      Assert.IsTrue(value13.TryGetInt64(out var intValue13) && intValue13 == 42, "tag_i213");
      Assert.IsTrue(value14.TryGetInt64(out var intValue14) && intValue14 == 42, "tag_i214");
      Assert.IsTrue(value15.TryGetInt64(out var intValue15) && intValue15 == 42, "tag_i215");
      Assert.IsTrue(value16.TryGetInt64(out var intValue16) && intValue16 == 42, "tag_i216");
      Assert.IsTrue(value17.TryGetInt64(out var intValue17) && intValue17 == 42, "tag_i217");
      Assert.IsTrue(value18.TryGetDouble(out var doubleValue18) && doubleValue18 == (float)4.2, "tag_i218");
      Assert.IsTrue(value19.TryGetDouble(out var doubleValue19) && doubleValue19 == 4.2, "tag_i219");
    }

    [TestMethod]
    public void TestAsObject()
    {
      // Any type except for Object is returned as EmptyObject.
      bool AsObjectIsEmpty(JSValue value) => value.AsObject().Count == 0;

      Assert.IsFalse(AsObjectIsEmpty(new JSValueObject { ["prop1"] = 42 }), "tag_j01");
      Assert.IsTrue(AsObjectIsEmpty(JSValue.EmptyObject), "tag_j02");
      Assert.IsTrue(AsObjectIsEmpty(new JSValueArray { 42, 78 }), "tag_j03");
      Assert.IsTrue(AsObjectIsEmpty(JSValue.EmptyArray), "tag_j04");
      Assert.IsTrue(AsObjectIsEmpty(""), "tag_j05");
      Assert.IsTrue(AsObjectIsEmpty("Hello"), "tag_j06");
      Assert.IsTrue(AsObjectIsEmpty(true), "tag_j07");
      Assert.IsTrue(AsObjectIsEmpty(false), "tag_j08");
      Assert.IsTrue(AsObjectIsEmpty(0), "tag_j09");
      Assert.IsTrue(AsObjectIsEmpty(42), "tag_j10");
      Assert.IsTrue(AsObjectIsEmpty(long.MaxValue), "tag_j11");
      Assert.IsTrue(AsObjectIsEmpty(long.MinValue), "tag_j12");
      Assert.IsTrue(AsObjectIsEmpty(0.0), "tag_j13");
      Assert.IsTrue(AsObjectIsEmpty(4.2), "tag_j14");
      Assert.IsTrue(AsObjectIsEmpty(double.NaN), "tag_j15");
      Assert.IsTrue(AsObjectIsEmpty(double.PositiveInfinity), "tag_j16");
      Assert.IsTrue(AsObjectIsEmpty(double.NegativeInfinity), "tag_j17");
      Assert.IsTrue(AsObjectIsEmpty(JSValue.Null), "tag_j18");
    }

    [TestMethod]
    public void TestAsArray()
    {
      // Any type except for Array is returned as EmptyObject.
      bool AsArrayIsEmpty(JSValue value) => value.AsArray().Count == 0;

      Assert.IsTrue(AsArrayIsEmpty(new JSValueObject { ["prop1"] = 42 }), "tag_k01");
      Assert.IsTrue(AsArrayIsEmpty(JSValue.EmptyObject), "tag_k02");
      Assert.IsFalse(AsArrayIsEmpty(new JSValueArray { 42, 78 }), "tag_k03");
      Assert.IsTrue(AsArrayIsEmpty(JSValue.EmptyArray), "tag_k04");
      Assert.IsTrue(AsArrayIsEmpty(""), "tag_k05");
      Assert.IsTrue(AsArrayIsEmpty("Hello"), "tag_k06");
      Assert.IsTrue(AsArrayIsEmpty(true), "tag_k07");
      Assert.IsTrue(AsArrayIsEmpty(false), "tag_k08");
      Assert.IsTrue(AsArrayIsEmpty(0), "tag_k09");
      Assert.IsTrue(AsArrayIsEmpty(42), "tag_k10");
      Assert.IsTrue(AsArrayIsEmpty(long.MaxValue), "tag_k11");
      Assert.IsTrue(AsArrayIsEmpty(long.MinValue), "tag_k12");
      Assert.IsTrue(AsArrayIsEmpty(0.0), "tag_k13");
      Assert.IsTrue(AsArrayIsEmpty(4.2), "tag_k14");
      Assert.IsTrue(AsArrayIsEmpty(double.NaN), "tag_k15");
      Assert.IsTrue(AsArrayIsEmpty(double.PositiveInfinity), "tag_k16");
      Assert.IsTrue(AsArrayIsEmpty(double.NegativeInfinity), "tag_k17");
      Assert.IsTrue(AsArrayIsEmpty(JSValue.Null), "tag_k18");
    }

    [TestMethod]
    public void TestAsConverters()
    {
      // Check AsString, AsBoolean, AsInt64, and AsDouble conversions.
      void CheckAsConverter(JSValue value, string asString, bool asBoolean, long asInt64, double asDouble, string tag)
      {
        Assert.AreEqual(asString, value.AsString(), "AsString: {0}", tag);
        Assert.AreEqual(asBoolean, value.AsBoolean(), "AsBoolean: {0}", tag);
        Assert.AreEqual(asInt64, value.AsInt64(), "AsInt64: {0}", tag);
        Assert.AreEqual(asDouble, value.AsDouble(), "AsDouble: {0}", tag);

        // Explicit cast is an alternative to the As conversion.
        Assert.AreEqual(asString, (string)value, "(string): {0}", tag);
        Assert.AreEqual(asBoolean, (bool)value, "(bool): {0}", tag);
        Assert.AreEqual(asInt64, (long)value, "(long): {0}", tag);
        Assert.AreEqual(asDouble, (double)value, "(double): {0}", tag);
      }

      CheckAsConverter(new JSValueObject { ["prop1"] = 42 }, "", true, 0, 0, "tag_l01");
      CheckAsConverter(JSValue.EmptyObject, "", false, 0, 0, "tag_l02");
      CheckAsConverter(new JSValueArray { 42, 78 }, "", true, 0, 0, "tag_l03");
      CheckAsConverter(JSValue.EmptyArray, "", false, 0, 0, "tag_l04");
      CheckAsConverter("", "", false, 0, 0, "tag_l05");
      CheckAsConverter("  ", "  ", false, 0, 0, "tag_l06");
      CheckAsConverter("42", "42", false, 42, 42, "tag_l07");
      CheckAsConverter("  42  ", "  42  ", false, 42, 42, "tag_l08");
      CheckAsConverter("4.2", "4.2", false, 4, 4.2, "tag_l09");
      CheckAsConverter("Hello", "Hello", false, 0, double.NaN, "tag_l10");
      CheckAsConverter("true", "true", true, 0, double.NaN, "tag_l11");
      CheckAsConverter("false", "false", false, 0, double.NaN, "tag_l12");
      CheckAsConverter("True", "True", true, 0, double.NaN, "tag_l13");
      CheckAsConverter("False", "False", false, 0, double.NaN, "tag_l14");
      CheckAsConverter("TRUE", "TRUE", true, 0, double.NaN, "tag_l15");
      CheckAsConverter("FALSE", "FALSE", false, 0, double.NaN, "tag_l16");
      CheckAsConverter("on", "on", true, 0, double.NaN, "tag_l17");
      CheckAsConverter("off", "off", false, 0, double.NaN, "tag_l18");
      CheckAsConverter("On", "On", true, 0, double.NaN, "tag_l19");
      CheckAsConverter("Off", "Off", false, 0, double.NaN, "tag_l20");
      CheckAsConverter("ON", "ON", true, 0, double.NaN, "tag_l21");
      CheckAsConverter("OFF", "OFF", false, 0, double.NaN, "tag_l22");
      CheckAsConverter("yes", "yes", true, 0, double.NaN, "tag_l23");
      CheckAsConverter("no", "no", false, 0, double.NaN, "tag_l24");
      CheckAsConverter("y", "y", true, 0, double.NaN, "tag_l25");
      CheckAsConverter("n", "n", false, 0, double.NaN, "tag_l26");
      CheckAsConverter("Y", "Y", true, 0, double.NaN, "tag_l27");
      CheckAsConverter("N", "N", false, 0, double.NaN, "tag_l28");
      CheckAsConverter("1", "1", true, 1, 1, "tag_l29");
      CheckAsConverter("0", "0", false, 0, 0, "tag_l20");
      CheckAsConverter(true, "true", true, 1, 1, "tag_l31");
      CheckAsConverter(false, "false", false, 0, 0, "tag_l32");
      CheckAsConverter(0, "0", false, 0, 0, "tag_l33");
      CheckAsConverter(42, "42", true, 42, 42, "tag_l34");
      CheckAsConverter(long.MaxValue, "9223372036854775807", true, long.MaxValue, long.MaxValue, "tag_l35");
      CheckAsConverter(long.MinValue, "-9223372036854775808", true, long.MinValue, long.MinValue, "tag_l36");
      CheckAsConverter(0.0, "0", false, 0, 0, "tag_l37");
      CheckAsConverter(4.2, "4.2", true, 4, 4.2, "tag_l38");
      CheckAsConverter(-4.2, "-4.2", true, -4, -4.2, "tag_l39");
      CheckAsConverter(double.MaxValue, "1.79769313486232E+308", true, 0, double.MaxValue, "tag_l40");
      CheckAsConverter(double.MinValue, "-1.79769313486232E+308", true, 0, double.MinValue, "tag_l41");
      CheckAsConverter(double.NaN, "NaN", false, 0, double.NaN, "tag_l42");
      CheckAsConverter(double.PositiveInfinity, "Infinity", true, 0, double.PositiveInfinity, "tag_l43");
      CheckAsConverter(double.NegativeInfinity, "-Infinity", true, 0, double.NegativeInfinity, "tag_l44");
      CheckAsConverter(JSValue.Null, "null", false, 0, 0, "tag_l45");
    }

    [TestMethod]
    public void TestExplicitNumberConversion()
    {
      // Check that explicit number conversions are defined
      Assert.AreEqual(42, (sbyte)new JSValue(42), "tag_l01");
      Assert.AreEqual(42, (short)new JSValue(42), "tag_l02");
      Assert.AreEqual(42, (int)new JSValue(42), "tag_l03");
      Assert.AreEqual(42, (long)new JSValue(42), "tag_l04");
      Assert.AreEqual(42, (byte)new JSValue(42), "tag_l05");
      Assert.AreEqual(42, (ushort)new JSValue(42), "tag_l06");
      Assert.AreEqual(42u, (uint)new JSValue(42), "tag_l07");
      Assert.AreEqual(42u, (ulong)new JSValue(42), "tag_l08");
      Assert.AreEqual((float)4.2, (float)new JSValue(4.2), "tag_l09");
      Assert.AreEqual(4.2, (double)new JSValue(4.2), "tag_l10");
    }

    [TestMethod]
    public void TestAsJSConverters()
    {
      // Check AsJSString, AsJSBoolean, and AsJSNumber conversions.
      // They must match the JavaScript String(), Boolean(), and Number() conversions. 
      void CheckAsJSConverter(JSValue value, string asJSString, bool asJSBoolean, double asJSNumber, string tag)
      {
        Assert.AreEqual(asJSString, value.AsJSString(), "AsJSString: {0}", tag);
        Assert.AreEqual(asJSBoolean, value.AsJSBoolean(), "AsJSBoolean: {0}", tag);
        Assert.AreEqual(asJSNumber, value.AsJSNumber(), "AsJSNumber: {0}", tag);
      }

      CheckAsJSConverter(false, "false", false, 0, "tag_m01");
      CheckAsJSConverter(true, "true", true, 1, "tag_m02");
      CheckAsJSConverter(0, "0", false, 0, "tag_m03");
      CheckAsJSConverter(1, "1", true, 1, "tag_m04");
      CheckAsJSConverter("0", "0", true, 0, "tag_m05");
      CheckAsJSConverter("000", "000", true, 0, "tag_m06");
      CheckAsJSConverter("1", "1", true, 1, "tag_m07");
      CheckAsJSConverter(double.NaN, "NaN", false, double.NaN, "tag_m08");
      CheckAsJSConverter(double.PositiveInfinity, "Infinity", true, double.PositiveInfinity, "tag_m09");
      CheckAsJSConverter(double.NegativeInfinity, "-Infinity", true, double.NegativeInfinity, "tag_m10");
      CheckAsJSConverter("", "", false, 0, "tag_m11");
      CheckAsJSConverter("20", "20", true, 20, "tag_m12");
      CheckAsJSConverter("twenty", "twenty", true, double.NaN, "tag_m13");
      CheckAsJSConverter(new JSValueArray { }, "", true, 0, "tag_m14");
      CheckAsJSConverter(new JSValueArray { 20 }, "20", true, 20, "tag_m15");
      CheckAsJSConverter(new JSValueArray { 10, 20 }, "10,20", true, double.NaN, "tag_m16");
      CheckAsJSConverter(new JSValueArray { "twenty" }, "twenty", true, double.NaN, "tag_m17");
      CheckAsJSConverter(new JSValueArray { "ten", "twenty" }, "ten,twenty", true, double.NaN, "tag_m18");
      CheckAsJSConverter(new JSValueArray { double.NaN }, "NaN", true, double.NaN, "tag_m19");
      CheckAsJSConverter(new JSValueObject { }, "[object Object]", true, double.NaN, "tag_m20");
      CheckAsJSConverter(JSValue.Null, "null", false, 0, "tag_m21");
    }

    [TestMethod]
    public void TestJSEquals()
    {
      Assert.IsTrue(new JSValue().JSEquals(JSValue.Null), "tag_n1001");
      Assert.IsFalse(new JSValue().JSEquals(new JSValueObject { }), "tag_n1002");
      Assert.IsFalse(new JSValue().JSEquals(new JSValueArray { }), "tag_n1003");
      Assert.IsFalse(new JSValue().JSEquals(""), "tag_n1004");
      Assert.IsFalse(new JSValue().JSEquals(false), "tag_n1005");
      Assert.IsFalse(new JSValue().JSEquals(0), "tag_n1006");
      Assert.IsFalse(new JSValue().JSEquals(0.0), "tag_n1007");

      Assert.IsFalse(new JSValue(new JSValueObject()).JSEquals(JSValue.Null), "tag_n2001");
      Assert.IsTrue(new JSValue(new JSValueObject { }).JSEquals(new JSValueObject { }), "tag_n2002");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { }), "tag_n2003");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "Hello" }), "tag_n2004");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { 0 }), "tag_n2005");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "0" }), "tag_n2006");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { 1 }), "tag_n2007");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "1" }), "tag_n2008");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { true }), "tag_n2009");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(new JSValueArray { "true" }), "tag_n2010");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(""), "tag_n2011");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("0"), "tag_n2012");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("1"), "tag_n2013");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("true"), "tag_n2014");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("false"), "tag_n2015");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals("Hello"), "tag_n2016");
      Assert.IsTrue(new JSValue(new JSValueObject { }).JSEquals("[object Object]"), "tag_n2017");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(false), "tag_n2018");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(true), "tag_n2019");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(0), "tag_n2020");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(5), "tag_n2021");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(1), "tag_n2022");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(0.0), "tag_n2023");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(0.5), "tag_n2024");
      Assert.IsFalse(new JSValue(new JSValueObject { }).JSEquals(1.0), "tag_n2025");

      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop2", 0 }, { "prop1", "2" } }), "tag_n3001");
      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop2", 0 } }), "tag_n3002");
      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop1", 2 }, { "prop25", false } }), "tag_n3003");
      Assert.IsTrue(new JSValue(new JSValueObject { { "prop1", 2 }, { "prop2", false } })
                      .JSEquals(new JSValueObject { { "prop1", 2 }, { "prop2", true } }), "tag_n3004");

      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(JSValue.Null), "tag_n4001");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueObject { }), "tag_n4002");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { }), "tag_n4003");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "Hello" }), "tag_n4004");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { 0 }), "tag_n4005");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "0" }), "tag_n4006");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { 1 }), "tag_n4007");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "1" }), "tag_n4008");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { true }), "tag_n4009");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(new JSValueArray { "true" }), "tag_n4010");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(""), "tag_n4011");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("0"), "tag_n4012");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("1"), "tag_n4013");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("true"), "tag_n4014");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("false"), "tag_n4015");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals("Hello"), "tag_n4016");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(false), "tag_n4017");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(true), "tag_n4018");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(0), "tag_n4019");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(5), "tag_n4020");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(1), "tag_n4021");
      Assert.IsTrue(new JSValue(new JSValueArray { }).JSEquals(0.0), "tag_n4022");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(0.5), "tag_n4023");
      Assert.IsFalse(new JSValue(new JSValueArray { }).JSEquals(1.0), "tag_n4024");

      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(JSValue.Null), "tag_n5001");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueObject { }), "tag_n5002");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { }), "tag_n5003");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "Hello" }), "tag_n5004");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { 0 }), "tag_n5005");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "0" }), "tag_n5006");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { 1 }), "tag_n5007");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "1" }), "tag_n5008");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { true }), "tag_n5009");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(new JSValueArray { "true" }), "tag_n5010");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(""), "tag_n5011");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("0"), "tag_n5012");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals("1"), "tag_n5013");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("true"), "tag_n5014");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("false"), "tag_n5015");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals("Hello"), "tag_n5016");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(false), "tag_n5017");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(true), "tag_n5018");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(0), "tag_n5019");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(5), "tag_n5020");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(1), "tag_n5021");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(0.0), "tag_n5022");
      Assert.IsFalse(new JSValue(new JSValueArray { 1 }).JSEquals(0.5), "tag_n5023");
      Assert.IsTrue(new JSValue(new JSValueArray { 1 }).JSEquals(1.0), "tag_n5024");

      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(JSValue.Null), "tag_n6001");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueObject { }), "tag_n6002");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { }), "tag_n6003");
      Assert.IsTrue(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "Hello" }), "tag_n6004");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { 0 }), "tag_n6005");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "0" }), "tag_n6006");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { 1 }), "tag_n6007");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "1" }), "tag_n6008");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { true }), "tag_n6009");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(new JSValueArray { "true" }), "tag_n6010");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(""), "tag_n6011");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("0"), "tag_n6012");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("1"), "tag_n6013");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("true"), "tag_n6014");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals("false"), "tag_n6015");
      Assert.IsTrue(new JSValue(new JSValueArray { "Hello" }).JSEquals("Hello"), "tag_n6016");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(false), "tag_n6017");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(true), "tag_n6018");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(0), "tag_n6019");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(5), "tag_n6020");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(1), "tag_n6021");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(0.0), "tag_n6022");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(0.5), "tag_n6023");
      Assert.IsFalse(new JSValue(new JSValueArray { "Hello" }).JSEquals(1.0), "tag_n6024");

      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(JSValue.Null), "tag_n7001");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueObject { }), "tag_n7002");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { }), "tag_n7003");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "Hello" }), "tag_n7004");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { 0 }), "tag_n7005");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "0" }), "tag_n7006");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { 1 }), "tag_n7007");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "1" }), "tag_n7008");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { true }), "tag_n7009");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "true" }), "tag_n7010");
      Assert.IsTrue(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { 0, 1 }), "tag_n7011");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { false, true }), "tag_n7012");
      Assert.IsTrue(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "0", "1" }), "tag_n7013");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(new JSValueArray { "0", true }), "tag_n7014");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(""), "tag_n7015");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("0"), "tag_n7016");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("1"), "tag_n7017");
      Assert.IsTrue(new JSValue(new JSValueArray { 0, 1 }).JSEquals("0,1"), "tag_n7018");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("true"), "tag_n7019");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("false"), "tag_n7020");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals("Hello"), "tag_n7021");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(false), "tag_n7022");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(true), "tag_n7023");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(0), "tag_n7024");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(5), "tag_n7025");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(1), "tag_n7026");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(0.0), "tag_n7027");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(0.5), "tag_n7028");
      Assert.IsFalse(new JSValue(new JSValueArray { 0, 1 }).JSEquals(1.0), "tag_n7029");

      Assert.IsFalse(new JSValue("").JSEquals(JSValue.Null), "tag_n8001");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueObject { }), "tag_n8002");
      Assert.IsTrue(new JSValue("").JSEquals(new JSValueArray { }), "tag_n8003");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueArray { 0 }), "tag_n8004");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueArray { 1 }), "tag_n8005");
      Assert.IsFalse(new JSValue("").JSEquals(new JSValueArray { true }), "tag_n8006");
      Assert.IsTrue(new JSValue("").JSEquals(new JSValueArray { "" }), "tag_n8007");
      Assert.IsTrue(new JSValue("").JSEquals(""), "tag_n8008");
      Assert.IsFalse(new JSValue("").JSEquals("1"), "tag_n8009");
      Assert.IsFalse(new JSValue("").JSEquals("Hello"), "tag_n8010");
      Assert.IsTrue(new JSValue("").JSEquals(false), "tag_n8011");
      Assert.IsFalse(new JSValue("").JSEquals(true), "tag_n8012");
      Assert.IsTrue(new JSValue("").JSEquals(0), "tag_n8013");
      Assert.IsFalse(new JSValue("").JSEquals(5), "tag_n8014");
      Assert.IsFalse(new JSValue("").JSEquals(1), "tag_n8015");
      Assert.IsTrue(new JSValue("").JSEquals(0.0), "tag_n8016");
      Assert.IsFalse(new JSValue("").JSEquals(0.5), "tag_n8017");
      Assert.IsFalse(new JSValue("").JSEquals(1.0), "tag_n8018");

      Assert.IsFalse(new JSValue("Hello").JSEquals(JSValue.Null), "tag_n9001");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueObject { }), "tag_n9002");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { }), "tag_n9003");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { 0 }), "tag_n9004");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { 1 }), "tag_n9005");
      Assert.IsFalse(new JSValue("Hello").JSEquals(new JSValueArray { true }), "tag_n9006");
      Assert.IsTrue(new JSValue("Hello").JSEquals(new JSValueArray { "Hello" }), "tag_n9007");
      Assert.IsFalse(new JSValue("Hello").JSEquals(""), "tag_n9008");
      Assert.IsFalse(new JSValue("Hello").JSEquals("1"), "tag_n9009");
      Assert.IsTrue(new JSValue("Hello").JSEquals("Hello"), "tag_n9010");
      Assert.IsFalse(new JSValue("Hello").JSEquals(false), "tag_n9011");
      Assert.IsFalse(new JSValue("Hello").JSEquals(true), "tag_n9012");
      Assert.IsFalse(new JSValue("Hello").JSEquals(0), "tag_n9013");
      Assert.IsFalse(new JSValue("Hello").JSEquals(5), "tag_n9014");
      Assert.IsFalse(new JSValue("Hello").JSEquals(1), "tag_n9015");
      Assert.IsFalse(new JSValue("Hello").JSEquals(0.0), "tag_n9016");
      Assert.IsFalse(new JSValue("Hello").JSEquals(0.5), "tag_n9017");
      Assert.IsFalse(new JSValue("Hello").JSEquals(1.0), "tag_n9018");

      Assert.IsFalse(new JSValue("0").JSEquals(JSValue.Null), "tag_n10001");
      Assert.IsFalse(new JSValue("0").JSEquals(new JSValueObject { }), "tag_n10002");
      Assert.IsFalse(new JSValue("0").JSEquals(new JSValueArray { }), "tag_n10003");
      Assert.IsFalse(new JSValue("0").JSEquals(new JSValueArray { "Hello" }), "tag_n10004");
      Assert.IsTrue(new JSValue("0").JSEquals(new JSValueArray { "0" }), "tag_n10005");
      Assert.IsTrue(new JSValue("0").JSEquals(new JSValueArray { 0 }), "tag_n10006");
      Assert.IsFalse(new JSValue("0").JSEquals(""), "tag_n10007");
      Assert.IsTrue(new JSValue("0").JSEquals("0"), "tag_n10008");
      Assert.IsFalse(new JSValue("0").JSEquals("Hello"), "tag_n10009");
      Assert.IsTrue(new JSValue("0").JSEquals(false), "tag_n10010");
      Assert.IsFalse(new JSValue("0").JSEquals(true), "tag_n10011");
      Assert.IsTrue(new JSValue("0").JSEquals(0), "tag_n10012");
      Assert.IsFalse(new JSValue("0").JSEquals(5), "tag_n10013");
      Assert.IsFalse(new JSValue("0").JSEquals(1), "tag_n10014");
      Assert.IsTrue(new JSValue("0").JSEquals(0.0), "tag_n10015");
      Assert.IsFalse(new JSValue("0").JSEquals(0.5), "tag_n10016");
      Assert.IsFalse(new JSValue("0").JSEquals(1.0), "tag_n10017");

      Assert.IsFalse(new JSValue("1").JSEquals(JSValue.Null), "tag_n11001");
      Assert.IsFalse(new JSValue("1").JSEquals(new JSValueObject { }), "tag_n11002");
      Assert.IsFalse(new JSValue("1").JSEquals(new JSValueArray { }), "tag_n11003");
      Assert.IsFalse(new JSValue("1").JSEquals(new JSValueArray { "Hello" }), "tag_n11004");
      Assert.IsTrue(new JSValue("1").JSEquals(new JSValueArray { "1" }), "tag_n11005");
      Assert.IsTrue(new JSValue("1").JSEquals(new JSValueArray { 1 }), "tag_n11006");
      Assert.IsFalse(new JSValue("1").JSEquals(""), "tag_n11007");
      Assert.IsTrue(new JSValue("1").JSEquals("1"), "tag_n11008");
      Assert.IsFalse(new JSValue("1").JSEquals("Hello"), "tag_n11009");
      Assert.IsFalse(new JSValue("1").JSEquals(false), "tag_n11010");
      Assert.IsTrue(new JSValue("1").JSEquals(true), "tag_n11011");
      Assert.IsFalse(new JSValue("1").JSEquals(0), "tag_n11012");
      Assert.IsFalse(new JSValue("1").JSEquals(5), "tag_n11013");
      Assert.IsTrue(new JSValue("1").JSEquals(1), "tag_n11014");
      Assert.IsFalse(new JSValue("1").JSEquals(0.0), "tag_n11015");
      Assert.IsFalse(new JSValue("1").JSEquals(0.5), "tag_n11016");
      Assert.IsTrue(new JSValue("1").JSEquals(1.0), "tag_n11017");

      Assert.IsFalse(new JSValue("true").JSEquals(JSValue.Null), "tag_n12001");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueObject { }), "tag_n12002");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueArray { }), "tag_n12003");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueArray { 0 }), "tag_n12004");
      Assert.IsFalse(new JSValue("true").JSEquals(new JSValueArray { 1 }), "tag_n12005");
      Assert.IsTrue(new JSValue("true").JSEquals(new JSValueArray { true }), "tag_n12006");
      Assert.IsTrue(new JSValue("true").JSEquals(new JSValueArray { "true" }), "tag_n12007");
      Assert.IsFalse(new JSValue("true").JSEquals(""), "tag_n12008");
      Assert.IsFalse(new JSValue("true").JSEquals("1"), "tag_n12009");
      Assert.IsFalse(new JSValue("true").JSEquals("Hello"), "tag_n12010");
      Assert.IsTrue(new JSValue("true").JSEquals("true"), "tag_n12011");
      Assert.IsFalse(new JSValue("true").JSEquals(false), "tag_n12012");
      Assert.IsFalse(new JSValue("true").JSEquals(true), "tag_n12013");
      Assert.IsFalse(new JSValue("true").JSEquals(0), "tag_n12014");
      Assert.IsFalse(new JSValue("true").JSEquals(5), "tag_n12015");
      Assert.IsFalse(new JSValue("true").JSEquals(1), "tag_n12016");
      Assert.IsFalse(new JSValue("true").JSEquals(0.0), "tag_n12017");
      Assert.IsFalse(new JSValue("true").JSEquals(0.5), "tag_n12018");
      Assert.IsFalse(new JSValue("true").JSEquals(1.0), "tag_n12019");

      Assert.IsTrue(new JSValue("[object Object]").JSEquals(new JSValueObject { }), "tag_n13001");

      Assert.IsFalse(new JSValue(true).JSEquals(JSValue.Null), "tag_n14001");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueObject { }), "tag_n14002");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { }), "tag_n14003");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { "Hello" }), "tag_n14004");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { 0 }), "tag_n14005");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { "0" }), "tag_n14006");
      Assert.IsTrue(new JSValue(true).JSEquals(new JSValueArray { 1 }), "tag_n14007");
      Assert.IsTrue(new JSValue(true).JSEquals(new JSValueArray { "1" }), "tag_n14008");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { true }), "tag_n14009");
      Assert.IsFalse(new JSValue(true).JSEquals(new JSValueArray { "true" }), "tag_n14010");
      Assert.IsFalse(new JSValue(true).JSEquals(""), "tag_n14011");
      Assert.IsFalse(new JSValue(true).JSEquals("0"), "tag_n14012");
      Assert.IsTrue(new JSValue(true).JSEquals("1"), "tag_n14013");
      Assert.IsFalse(new JSValue(true).JSEquals("true"), "tag_n14014");
      Assert.IsFalse(new JSValue(true).JSEquals("false"), "tag_n14015");
      Assert.IsFalse(new JSValue(true).JSEquals("Hello"), "tag_n14016");
      Assert.IsFalse(new JSValue(true).JSEquals(false), "tag_n14017");
      Assert.IsTrue(new JSValue(true).JSEquals(true), "tag_n14018");
      Assert.IsFalse(new JSValue(true).JSEquals(0), "tag_n14019");
      Assert.IsFalse(new JSValue(true).JSEquals(5), "tag_n14020");
      Assert.IsTrue(new JSValue(true).JSEquals(1), "tag_n14021");
      Assert.IsFalse(new JSValue(true).JSEquals(0.0), "tag_n14022");
      Assert.IsFalse(new JSValue(true).JSEquals(0.5), "tag_n14023");
      Assert.IsTrue(new JSValue(true).JSEquals(1.0), "tag_n14024");

      Assert.IsFalse(new JSValue(false).JSEquals(JSValue.Null), "tag_n15001");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueObject { }), "tag_n15002");
      Assert.IsTrue(new JSValue(false).JSEquals(new JSValueArray { }), "tag_n15003");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { "Hello" }), "tag_n15004");
      Assert.IsTrue(new JSValue(false).JSEquals(new JSValueArray { 0 }), "tag_n15005");
      Assert.IsTrue(new JSValue(false).JSEquals(new JSValueArray { "0" }), "tag_n15006");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { 1 }), "tag_n15007");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { "1" }), "tag_n15008");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { true }), "tag_n15009");
      Assert.IsFalse(new JSValue(false).JSEquals(new JSValueArray { "true" }), "tag_n15010");
      Assert.IsTrue(new JSValue(false).JSEquals(""), "tag_n15011");
      Assert.IsTrue(new JSValue(false).JSEquals("0"), "tag_n15012");
      Assert.IsFalse(new JSValue(false).JSEquals("1"), "tag_n15013");
      Assert.IsFalse(new JSValue(false).JSEquals("true"), "tag_n15014");
      Assert.IsFalse(new JSValue(false).JSEquals("false"), "tag_n15015");
      Assert.IsFalse(new JSValue(false).JSEquals("Hello"), "tag_n15016");
      Assert.IsTrue(new JSValue(false).JSEquals(false), "tag_n15017");
      Assert.IsFalse(new JSValue(false).JSEquals(true), "tag_n15018");
      Assert.IsTrue(new JSValue(false).JSEquals(0), "tag_n15019");
      Assert.IsFalse(new JSValue(false).JSEquals(5), "tag_n15020");
      Assert.IsFalse(new JSValue(false).JSEquals(1), "tag_n15021");
      Assert.IsTrue(new JSValue(false).JSEquals(0.0), "tag_n15022");
      Assert.IsFalse(new JSValue(false).JSEquals(0.5), "tag_n15023");
      Assert.IsFalse(new JSValue(false).JSEquals(1.0), "tag_n15024");

      Assert.IsFalse(new JSValue(0).JSEquals(JSValue.Null), "tag_n16001");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueObject { }), "tag_n16002");
      Assert.IsTrue(new JSValue(0).JSEquals(new JSValueArray { }), "tag_n16003");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { "Hello" }), "tag_n16004");
      Assert.IsTrue(new JSValue(0).JSEquals(new JSValueArray { 0 }), "tag_n16005");
      Assert.IsTrue(new JSValue(0).JSEquals(new JSValueArray { "0" }), "tag_n16006");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { 1 }), "tag_n16007");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { "1" }), "tag_n16008");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { true }), "tag_n16009");
      Assert.IsFalse(new JSValue(0).JSEquals(new JSValueArray { "true" }), "tag_n16010");
      Assert.IsTrue(new JSValue(0).JSEquals(""), "tag_n16011");
      Assert.IsTrue(new JSValue(0).JSEquals("0"), "tag_n16012");
      Assert.IsFalse(new JSValue(0).JSEquals("1"), "tag_n16013");
      Assert.IsFalse(new JSValue(0).JSEquals("true"), "tag_n16014");
      Assert.IsFalse(new JSValue(0).JSEquals("false"), "tag_n16015");
      Assert.IsFalse(new JSValue(0).JSEquals("Hello"), "tag_n16016");
      Assert.IsTrue(new JSValue(0).JSEquals(false), "tag_n16017");
      Assert.IsFalse(new JSValue(0).JSEquals(true), "tag_n16018");
      Assert.IsTrue(new JSValue(0).JSEquals(0), "tag_n16019");
      Assert.IsFalse(new JSValue(0).JSEquals(5), "tag_n16020");
      Assert.IsFalse(new JSValue(0).JSEquals(1), "tag_n16021");
      Assert.IsTrue(new JSValue(0).JSEquals(0.0), "tag_n16022");
      Assert.IsFalse(new JSValue(0).JSEquals(0.5), "tag_n16023");
      Assert.IsFalse(new JSValue(0).JSEquals(1.0), "tag_n16024");

      Assert.IsFalse(new JSValue(1).JSEquals(JSValue.Null), "tag_n17001");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueObject { }), "tag_n17002");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { }), "tag_n17003");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { "Hello" }), "tag_n17004");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { 0 }), "tag_n17005");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { "0" }), "tag_n17006");
      Assert.IsTrue(new JSValue(1).JSEquals(new JSValueArray { 1 }), "tag_n17007");
      Assert.IsTrue(new JSValue(1).JSEquals(new JSValueArray { "1" }), "tag_n17008");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { true }), "tag_n17009");
      Assert.IsFalse(new JSValue(1).JSEquals(new JSValueArray { "true" }), "tag_n17010");
      Assert.IsFalse(new JSValue(1).JSEquals(""), "tag_n17011");
      Assert.IsFalse(new JSValue(1).JSEquals("0"), "tag_n17012");
      Assert.IsTrue(new JSValue(1).JSEquals("1"), "tag_n17013");
      Assert.IsFalse(new JSValue(1).JSEquals("true"), "tag_n17014");
      Assert.IsFalse(new JSValue(1).JSEquals("false"), "tag_n17015");
      Assert.IsFalse(new JSValue(1).JSEquals("Hello"), "tag_n17016");
      Assert.IsFalse(new JSValue(1).JSEquals(false), "tag_n17017");
      Assert.IsTrue(new JSValue(1).JSEquals(true), "tag_n17018");
      Assert.IsFalse(new JSValue(1).JSEquals(0), "tag_n17019");
      Assert.IsFalse(new JSValue(1).JSEquals(5), "tag_n17020");
      Assert.IsTrue(new JSValue(1).JSEquals(1), "tag_n17021");
      Assert.IsFalse(new JSValue(1).JSEquals(0.0), "tag_n17022");
      Assert.IsFalse(new JSValue(1).JSEquals(0.5), "tag_n17023");
      Assert.IsTrue(new JSValue(1).JSEquals(1.0), "tag_n17024");

      Assert.IsFalse(new JSValue(5).JSEquals(JSValue.Null), "tag_n18001");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueObject { }), "tag_n18002");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { }), "tag_n18003");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "Hello" }), "tag_n18004");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { 0 }), "tag_n18005");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "0" }), "tag_n18006");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { 1 }), "tag_n18007");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "1" }), "tag_n18008");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { true }), "tag_n18009");
      Assert.IsFalse(new JSValue(5).JSEquals(new JSValueArray { "true" }), "tag_n18010");
      Assert.IsFalse(new JSValue(5).JSEquals(""), "tag_n18011");
      Assert.IsFalse(new JSValue(5).JSEquals("0"), "tag_n18012");
      Assert.IsFalse(new JSValue(5).JSEquals("1"), "tag_n18013");
      Assert.IsFalse(new JSValue(5).JSEquals("true"), "tag_n18014");
      Assert.IsFalse(new JSValue(5).JSEquals("false"), "tag_n18015");
      Assert.IsFalse(new JSValue(5).JSEquals("Hello"), "tag_n18016");
      Assert.IsFalse(new JSValue(5).JSEquals(false), "tag_n18017");
      Assert.IsFalse(new JSValue(5).JSEquals(true), "tag_n18018");
      Assert.IsFalse(new JSValue(5).JSEquals(0), "tag_n18019");
      Assert.IsTrue(new JSValue(5).JSEquals(5), "tag_n18020");
      Assert.IsFalse(new JSValue(5).JSEquals(1), "tag_n18021");
      Assert.IsFalse(new JSValue(5).JSEquals(0.0), "tag_n18022");
      Assert.IsFalse(new JSValue(5).JSEquals(0.5), "tag_n18023");
      Assert.IsFalse(new JSValue(5).JSEquals(1.0), "tag_n18024");

      Assert.IsFalse(new JSValue(0.0).JSEquals(JSValue.Null), "tag_n19001");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueObject { }), "tag_n19002");
      Assert.IsTrue(new JSValue(0.0).JSEquals(new JSValueArray { }), "tag_n19003");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { "Hello" }), "tag_n19004");
      Assert.IsTrue(new JSValue(0.0).JSEquals(new JSValueArray { 0 }), "tag_n19005");
      Assert.IsTrue(new JSValue(0.0).JSEquals(new JSValueArray { "0" }), "tag_n19006");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { 1 }), "tag_n19007");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { "1" }), "tag_n19008");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { true }), "tag_n19009");
      Assert.IsFalse(new JSValue(0.0).JSEquals(new JSValueArray { "true" }), "tag_n19010");
      Assert.IsTrue(new JSValue(0.0).JSEquals(""), "tag_n19011");
      Assert.IsTrue(new JSValue(0.0).JSEquals("0"), "tag_n19012");
      Assert.IsFalse(new JSValue(0.0).JSEquals("1"), "tag_n19013");
      Assert.IsFalse(new JSValue(0.0).JSEquals("true"), "tag_n19014");
      Assert.IsFalse(new JSValue(0.0).JSEquals("false"), "tag_n19015");
      Assert.IsFalse(new JSValue(0.0).JSEquals("Hello"), "tag_n19016");
      Assert.IsTrue(new JSValue(0.0).JSEquals(false), "tag_n19017");
      Assert.IsFalse(new JSValue(0.0).JSEquals(true), "tag_n19018");
      Assert.IsTrue(new JSValue(0.0).JSEquals(0), "tag_n19019");
      Assert.IsFalse(new JSValue(0.0).JSEquals(5), "tag_n19020");
      Assert.IsFalse(new JSValue(0.0).JSEquals(1), "tag_n19021");
      Assert.IsTrue(new JSValue(0.0).JSEquals(0.0), "tag_n19022");
      Assert.IsFalse(new JSValue(0.0).JSEquals(0.5), "tag_n19023");
      Assert.IsFalse(new JSValue(0.0).JSEquals(1.0), "tag_n19024");

      Assert.IsFalse(new JSValue(1.0).JSEquals(JSValue.Null), "tag_n20001");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueObject { }), "tag_n20002");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { }), "tag_n20003");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { "Hello" }), "tag_n20004");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { 0 }), "tag_n20005");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { "0" }), "tag_n20006");
      Assert.IsTrue(new JSValue(1.0).JSEquals(new JSValueArray { 1 }), "tag_n20007");
      Assert.IsTrue(new JSValue(1.0).JSEquals(new JSValueArray { "1" }), "tag_n20008");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { true }), "tag_n20009");
      Assert.IsFalse(new JSValue(1.0).JSEquals(new JSValueArray { "true" }), "tag_n20010");
      Assert.IsFalse(new JSValue(1.0).JSEquals(""), "tag_n20011");
      Assert.IsFalse(new JSValue(1.0).JSEquals("0"), "tag_n20012");
      Assert.IsTrue(new JSValue(1.0).JSEquals("1"), "tag_n20013");
      Assert.IsFalse(new JSValue(1.0).JSEquals("true"), "tag_n20014");
      Assert.IsFalse(new JSValue(1.0).JSEquals("false"), "tag_n20015");
      Assert.IsFalse(new JSValue(1.0).JSEquals("Hello"), "tag_n20016");
      Assert.IsFalse(new JSValue(1.0).JSEquals(false), "tag_n20017");
      Assert.IsTrue(new JSValue(1.0).JSEquals(true), "tag_n20018");
      Assert.IsFalse(new JSValue(1.0).JSEquals(0), "tag_n20019");
      Assert.IsFalse(new JSValue(1.0).JSEquals(5), "tag_n20020");
      Assert.IsTrue(new JSValue(1.0).JSEquals(1), "tag_n20021");
      Assert.IsFalse(new JSValue(1.0).JSEquals(0.0), "tag_n20022");
      Assert.IsFalse(new JSValue(1.0).JSEquals(0.5), "tag_n20023");
      Assert.IsTrue(new JSValue(1.0).JSEquals(1.0), "tag_n20024");

      Assert.IsFalse(new JSValue(0.5).JSEquals(JSValue.Null), "tag_n21001");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueObject { }), "tag_n21002");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { }), "tag_n21003");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "Hello" }), "tag_n21004");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { 0 }), "tag_n21005");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "0" }), "tag_n21006");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { 1 }), "tag_n21007");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "1" }), "tag_n21008");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { true }), "tag_n21009");
      Assert.IsFalse(new JSValue(0.5).JSEquals(new JSValueArray { "true" }), "tag_n21010");
      Assert.IsFalse(new JSValue(0.5).JSEquals(""), "tag_n21011");
      Assert.IsFalse(new JSValue(0.5).JSEquals("0"), "tag_n21012");
      Assert.IsFalse(new JSValue(0.5).JSEquals("1"), "tag_n21013");
      Assert.IsFalse(new JSValue(0.5).JSEquals("true"), "tag_n21014");
      Assert.IsFalse(new JSValue(0.5).JSEquals("false"), "tag_n21015");
      Assert.IsFalse(new JSValue(0.5).JSEquals("Hello"), "tag_n21016");
      Assert.IsFalse(new JSValue(0.5).JSEquals(false), "tag_n21017");
      Assert.IsFalse(new JSValue(0.5).JSEquals(true), "tag_n21018");
      Assert.IsFalse(new JSValue(0.5).JSEquals(0), "tag_n21019");
      Assert.IsFalse(new JSValue(0.5).JSEquals(5), "tag_n21020");
      Assert.IsFalse(new JSValue(0.5).JSEquals(1), "tag_n21021");
      Assert.IsFalse(new JSValue(0.5).JSEquals(0.0), "tag_n21022");
      Assert.IsTrue(new JSValue(0.5).JSEquals(0.5), "tag_n21023");
      Assert.IsFalse(new JSValue(0.5).JSEquals(1.0), "tag_n21024");
    }

    [TestMethod]
    public void TestEquals()
    {
      void CheckEquals(JSValue left, JSValue right, string tag)
      {
        Assert.IsTrue(left.Equals(right), "Equals: {0}", tag);
        Assert.IsTrue(left.Equals((object)right), "Equals(object): {0}", tag);
        Assert.IsTrue(left == right, "operator ==: {0}", tag);
      }

      void CheckNotEquals(JSValue left, JSValue right, string tag)
      {
        Assert.IsFalse(left.Equals(right), "!Equals: {0}", tag);
        Assert.IsFalse(left.Equals((object)right), "!Equals(object): {0}", tag);
        Assert.IsTrue(left != right, "operator !=: {0}", tag);
      }

      CheckEquals(new JSValueObject { }, new JSValueObject { }, "tag_o101");
      CheckEquals(new JSValueObject { ["prop1"] = 1 }, new JSValueObject { ["prop1"] = 1 }, "tag_o102");
      CheckEquals(new JSValueObject { ["prop1"] = 1, ["prop2"] = "Hello" },
                  new JSValueObject { ["prop1"] = 1, ["prop2"] = "Hello" }, "tag_o103");
      CheckEquals(new JSValueObject { ["prop1"] = new JSValueObject { } },
                  new JSValueObject { ["prop1"] = new JSValueObject { } }, "tag_o104");
      CheckEquals(new JSValueObject { ["prop1"] = new JSValueObject { ["prop1"] = 1 } },
                  new JSValueObject { ["prop1"] = new JSValueObject { ["prop1"] = 1 } }, "tag_o105");
      CheckEquals(new JSValueObject { ["prop1"] = new JSValueArray { } },
                  new JSValueObject { ["prop1"] = new JSValueArray { } }, "tag_o106");
      CheckEquals(new JSValueObject { ["prop1"] = new JSValueArray { 1 } },
                  new JSValueObject { ["prop1"] = new JSValueArray { 1 } }, "tag_o107");
      CheckNotEquals(new JSValueObject { ["prop1"] = 1 }, new JSValueObject { }, "tag_o108");
      CheckNotEquals(new JSValueObject { ["prop1"] = 1 }, new JSValueObject { ["prop1"] = 2 }, "tag_o109");
      CheckNotEquals(new JSValueObject { }, new JSValueArray { }, "tag_o110");
      CheckNotEquals(new JSValueObject { }, "", "tag_o111");
      CheckNotEquals(new JSValueObject { }, false, "tag_o112");
      CheckNotEquals(new JSValueObject { }, true, "tag_o113");
      CheckNotEquals(new JSValueObject { }, 0, "tag_o114");
      CheckNotEquals(new JSValueObject { }, 0.0, "tag_o115");


      CheckEquals(new JSValueArray { }, new JSValueArray { }, "tag_o201");
      CheckEquals(new JSValueArray { 1 }, new JSValueArray { 1 }, "tag_o202");
      CheckEquals(new JSValueArray { 1, "Hello" },
                  new JSValueArray { 1, "Hello" }, "tag_o203");
      CheckEquals(new JSValueArray { new JSValueArray { } },
                  new JSValueArray { new JSValueArray { } }, "tag_o204");
      CheckEquals(new JSValueArray { new JSValueArray { 1 } },
                  new JSValueArray { new JSValueArray { 1 } }, "tag_o205");
      CheckEquals(new JSValueArray { new JSValueObject { } },
                  new JSValueArray { new JSValueObject { } }, "tag_o206");
      CheckEquals(new JSValueArray { new JSValueObject { ["prop1"] = 1 } },
                  new JSValueArray { new JSValueObject { ["prop1"] = 1 } }, "tag_o207");
      CheckNotEquals(new JSValueArray { 1 }, new JSValueArray { }, "tag_o208");
      CheckNotEquals(new JSValueArray { 1 }, new JSValueArray { 2 }, "tag_o209");
      CheckNotEquals(new JSValueArray { }, new JSValueObject { }, "tag_o210");
      CheckNotEquals(new JSValueArray { }, "", "tag_o211");
      CheckNotEquals(new JSValueArray { }, false, "tag_o212");
      CheckNotEquals(new JSValueArray { }, true, "tag_o213");
      CheckNotEquals(new JSValueArray { }, 0, "tag_o214");
      CheckNotEquals(new JSValueArray { }, 0.0, "tag_o215");

      CheckEquals("", "", "tag_o301");
      CheckEquals("Hello", "Hello", "tag_o302");
      CheckNotEquals("Hello1", "Hello2", "tag_o303");
      CheckNotEquals("", new JSValueObject { }, "tag_o304");
      CheckNotEquals("", new JSValueArray { }, "tag_o305");
      CheckNotEquals("", false, "tag_o306");
      CheckNotEquals("", 0, "tag_o307");
      CheckNotEquals("", 0.0, "tag_o308");

      CheckEquals(false, false, "tag_o401");
      CheckEquals(true, true, "tag_o402");
      CheckNotEquals(false, true, "tag_o403");
      CheckNotEquals(true, false, "tag_o404");
      CheckNotEquals(false, new JSValueObject { }, "tag_o405");
      CheckNotEquals(false, new JSValueArray { }, "tag_o406");
      CheckNotEquals(false, "", "tag_o407");
      CheckNotEquals(false, 0, "tag_o408");
      CheckNotEquals(false, 0.0, "tag_o409");

      CheckEquals(0, 0, "tag_o501");
      CheckEquals(42, 42, "tag_o502");
      CheckNotEquals(2, 3, "tag_o503");
      CheckNotEquals(-1, 1, "tag_o504");
      CheckNotEquals(0, new JSValueObject { }, "tag_o505");
      CheckNotEquals(0, new JSValueArray { }, "tag_o506");
      CheckNotEquals(0, "", "tag_o507");
      CheckNotEquals(0, false, "tag_o508");
      CheckNotEquals(0, 0.0, "tag_o509");

      CheckEquals(0.0, 0.0, "tag_o601");
      CheckEquals(4.2, 4.2, "tag_o602");
      CheckNotEquals(0.2, 0.3, "tag_o603");
      CheckNotEquals(-0.1, 0.1, "tag_o604");
      CheckNotEquals(0.0, new JSValueObject { }, "tag_o605");
      CheckNotEquals(0.0, new JSValueArray { }, "tag_o606");
      CheckNotEquals(0.0, "", "tag_o607");
      CheckNotEquals(0.0, false, "tag_o608");
      CheckNotEquals(0.0, 0, "tag_o609");
    }
  }
}

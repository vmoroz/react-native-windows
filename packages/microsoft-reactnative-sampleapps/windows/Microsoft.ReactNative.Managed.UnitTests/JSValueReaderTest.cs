
using System;
using Microsoft.ReactNative.Bridge;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Newtonsoft.Json.Linq;

// Serialization and deserialization of C# types.
// For a struct or field we serialize public fields and properties.
// For any IEnumerable type we serialize it content.
// For any ICollection we can create deserialization for content.
// For IDictonary we serialize/deserialize as object.
// Simple types: numbers, Boolean, String.
// 
namespace Microsoft.ReactNative.Managed.UnitTests
{
    enum RobotModel
    {
        T2,
        R2D2,
        C3PO,
    };

    // We are generating ReadValue for it.
    enum RobotShape
    {
        Humanoid,
        Trashcan,
        Beercan,
        Quadrocopter,
    }

    class RobotInfo
    {
        public RobotModel Model { get; set; }
        public string Name { get; set; }
        public int Age;
        public RobotShape Shape { get; set; }
    }

    static class RobotSerialization
    {
        public static void ReadValue(this IJSValueReader reader, out RobotInfo value)
        {
            value = new RobotInfo();
            while (reader.GetNextObjectProperty(out string propertyName))
            {
                switch (propertyName)
                {
                    case nameof(value.Model): reader.ReadValue(out RobotModel model); value.Model = model; break;
                    case nameof(value.Name): reader.ReadValue(out string name); value.Name = name; break;
                    case nameof(value.Age): reader.ReadValue(out value.Age); break;
                    case nameof(value.Shape): reader.ReadValue(out RobotShape shape); value.Shape = shape; break;
                }
            }
        }

        public static void ReadValue(this IJSValueReader reader, out RobotModel value)
        {
            reader.ReadValue(out int model);
            value = (RobotModel)model;
        }
    }

    [TestClass]
    public class JSValueReaderTest
    {
        [TestMethod]
        public void TestReadRobotInfo()
        {
            JObject jobj = JObject.Parse(@"{ Model: 1, Name: ""Bob"", Age: 42, Shape: 1 }");
            IJSValueReader reader = new JTokenReader(jobj);

            reader.ReadValue(out RobotInfo robot);
            Assert.AreEqual(RobotModel.R2D2, robot.Model);
            Assert.AreEqual("Bob", robot.Name);
            Assert.AreEqual(42, robot.Age);
            Assert.AreEqual(RobotShape.Trashcan, robot.Shape);
        }

        [TestMethod]
        public void TestReadValueDefaultExtensions()
        {
            JObject jobj = JObject.Parse(@"{
                StringValue1: """",
                StringValue2: ""5"",
                StringValue3: ""Hello"",
                BoolValue1: false,
                BoolValue2: true,
                IntValue1: 0,
                IntValue2: 42,
                FloatValue: 3.14,
                NullValue: null
            }");
            IJSValueReader reader = new JTokenReader(jobj);

            Assert.AreEqual(JSValueType.Object, reader.ValueType);
            int properyCount = 0;
            while (reader.GetNextObjectProperty(out string propertyName))
            {
                switch (propertyName)
                {
                    case "StringValue1":
                        Assert.AreEqual("", reader.ReadValue<string>());
                        Assert.AreEqual(false, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)0, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)0, reader.ReadValue<short>());
                        Assert.AreEqual(0, reader.ReadValue<int>());
                        Assert.AreEqual(0, reader.ReadValue<long>());
                        Assert.AreEqual((byte)0, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)0, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)0, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)0, reader.ReadValue<ulong>());
                        Assert.AreEqual(0, reader.ReadValue<float>());
                        Assert.AreEqual(0, reader.ReadValue<double>());
                        break;
                    case "StringValue2":
                        Assert.AreEqual("5", reader.ReadValue<string>());
                        Assert.AreEqual(true, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)5, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)5, reader.ReadValue<short>());
                        Assert.AreEqual(5, reader.ReadValue<int>());
                        Assert.AreEqual(5, reader.ReadValue<long>());
                        Assert.AreEqual((byte)5, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)5, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)5, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)5, reader.ReadValue<ulong>());
                        Assert.AreEqual(5, reader.ReadValue<float>());
                        Assert.AreEqual(5, reader.ReadValue<double>());
                        break;
                    case "StringValue3":
                        Assert.AreEqual("Hello", reader.ReadValue<string>());
                        Assert.AreEqual(true, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)0, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)0, reader.ReadValue<short>());
                        Assert.AreEqual(0, reader.ReadValue<int>());
                        Assert.AreEqual(0, reader.ReadValue<long>());
                        Assert.AreEqual((byte)0, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)0, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)0, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)0, reader.ReadValue<ulong>());
                        Assert.AreEqual(0, reader.ReadValue<float>());
                        Assert.AreEqual(0, reader.ReadValue<double>());
                        break;
                    case "BoolValue1":
                        Assert.AreEqual("false", reader.ReadValue<string>());
                        Assert.AreEqual(false, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)0, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)0, reader.ReadValue<short>());
                        Assert.AreEqual(0, reader.ReadValue<int>());
                        Assert.AreEqual(0, reader.ReadValue<long>());
                        Assert.AreEqual((byte)0, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)0, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)0, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)0, reader.ReadValue<ulong>());
                        Assert.AreEqual(0, reader.ReadValue<float>());
                        Assert.AreEqual(0, reader.ReadValue<double>());
                        break;
                    case "BoolValue2":
                        Assert.AreEqual("true", reader.ReadValue<string>());
                        Assert.AreEqual(true, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)1, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)1, reader.ReadValue<short>());
                        Assert.AreEqual(1, reader.ReadValue<int>());
                        Assert.AreEqual(1, reader.ReadValue<long>());
                        Assert.AreEqual((byte)1, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)1, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)1, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)1, reader.ReadValue<ulong>());
                        Assert.AreEqual(1, reader.ReadValue<float>());
                        Assert.AreEqual(1, reader.ReadValue<double>());
                        break;
                    case "IntValue1":
                        Assert.AreEqual("0", reader.ReadValue<string>());
                        Assert.AreEqual(false, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)0, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)0, reader.ReadValue<short>());
                        Assert.AreEqual(0, reader.ReadValue<int>());
                        Assert.AreEqual(0, reader.ReadValue<long>());
                        Assert.AreEqual((byte)0, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)0, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)0, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)0, reader.ReadValue<ulong>());
                        Assert.AreEqual(0, reader.ReadValue<float>());
                        Assert.AreEqual(0, reader.ReadValue<double>());
                        break;
                    case "IntValue2":
                        Assert.AreEqual("42", reader.ReadValue<string>());
                        Assert.AreEqual(true, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)42, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)42, reader.ReadValue<short>());
                        Assert.AreEqual(42, reader.ReadValue<int>());
                        Assert.AreEqual(42, reader.ReadValue<long>());
                        Assert.AreEqual((byte)42, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)42, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)42, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)42, reader.ReadValue<ulong>());
                        Assert.AreEqual(42, reader.ReadValue<float>());
                        Assert.AreEqual(42, reader.ReadValue<double>());
                        break;
                    case "FloatValue":
                        Assert.AreEqual("3.14", reader.ReadValue<string>());
                        Assert.AreEqual(true, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)3, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)3, reader.ReadValue<short>());
                        Assert.AreEqual(3, reader.ReadValue<int>());
                        Assert.AreEqual(3, reader.ReadValue<long>());
                        Assert.AreEqual((byte)3, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)3, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)3, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)3, reader.ReadValue<ulong>());
                        Assert.AreEqual((float)3.14, reader.ReadValue<float>());
                        Assert.AreEqual(3.14, reader.ReadValue<double>());
                        break;
                    case "NullValue":
                        Assert.AreEqual("", reader.ReadValue<string>());
                        Assert.AreEqual(false, reader.ReadValue<bool>());
                        Assert.AreEqual((sbyte)0, reader.ReadValue<sbyte>());
                        Assert.AreEqual((short)0, reader.ReadValue<short>());
                        Assert.AreEqual(0, reader.ReadValue<int>());
                        Assert.AreEqual(0, reader.ReadValue<long>());
                        Assert.AreEqual((byte)0, reader.ReadValue<byte>());
                        Assert.AreEqual((ushort)0, reader.ReadValue<ushort>());
                        Assert.AreEqual((uint)0, reader.ReadValue<uint>());
                        Assert.AreEqual((ulong)0, reader.ReadValue<ulong>());
                        Assert.AreEqual(0, reader.ReadValue<float>());
                        Assert.AreEqual(0, reader.ReadValue<double>());
                        break;
                }
                ++properyCount;
            }
            Assert.AreEqual(9, properyCount);
        }
    }
}

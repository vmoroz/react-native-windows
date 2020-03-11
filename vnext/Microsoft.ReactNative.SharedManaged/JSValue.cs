// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using Windows.UI.Notifications;

namespace Microsoft.ReactNative.Managed
{
  // An alias to help build JSValue Object values
  class JSValueObject : Dictionary<string, JSValue>
  {
    public JSValueObject() { }

    public JSValueObject(IReadOnlyDictionary<string, JSValue> other)
    {
      foreach (var keyValue in other)
      {
        Add(keyValue.Key, keyValue.Value);
      }
    }
  }

  // An alias to help build JSValue Array values
  class JSValueArray : List<JSValue>
  {
    public JSValueArray() { }

    public JSValueArray(IReadOnlyList<JSValue> other)
    {
      foreach (var item in other)
      {
        Add(item);
      }
    }
  }

  // JSValue represents an immutable JavaScript value passed as a parameter.
  // It is created to simplify working with IJSValueReader.
  //
  // JSValue is designed to minimize number of memory allocations:
  // - It is a struct that avoid extra memory allocations when it is stored in a container.
  // - It avoids boxing simple values by using a union type to store them.
  // The JSValue is an immutable and is safe to be used from multiple threads.
  // It does not implement GetHashCode() and must not be used as a key in a dictionary.
  struct JSValue : IEquatable<JSValue>
  {
    public static readonly JSValue Null = new JSValue();
    public static readonly JSValue EmptyObject = new JSValueObject();
    public static readonly JSValue EmptyArray = new JSValueArray();
    public static readonly JSValue EmptyString = "";

    private static readonly HashSet<string> StringToBoolean =
      new HashSet<string>(StringComparer.OrdinalIgnoreCase) { "true", "1", "yes", "y", "on" };

    // The Void type could be used instead of void in generic types.
    public struct Void { };

    private readonly SimpleValue m_simpleValue;
    private readonly object m_objValue;

    public JSValue(IReadOnlyDictionary<string, JSValue> value)
    {
      Type = JSValueType.Object;
      m_simpleValue = new SimpleValue();
      if (value is ReadOnlyDictionary<string, JSValue>)
      {
        m_objValue = value;
      }
      else if (value is IDictionary<string, JSValue> dictionary)
      {
        m_objValue = new ReadOnlyDictionary<string, JSValue>(dictionary);
      }
      else
      {
        m_objValue = value ?? EmptyObject.ObjectValue;
      }
    }

    public JSValue(IReadOnlyList<JSValue> value)
    {
      Type = JSValueType.Array;
      m_simpleValue = new SimpleValue();
      if (value is ReadOnlyCollection<JSValue>)
      {
        m_objValue = value;
      }
      else if (value is IList<JSValue> list)
      {
        m_objValue = new ReadOnlyCollection<JSValue>(list);
      }
      else
      {
        m_objValue = value ?? EmptyArray.ArrayValue;
      }
    }

    public JSValue(string value)
    {
      Type = JSValueType.String;
      m_simpleValue = new SimpleValue();
      m_objValue = value ?? "";
    }

    public JSValue(bool value)
    {
      Type = JSValueType.Boolean;
      m_simpleValue = new SimpleValue { BooleanValue = value };
      m_objValue = null;
    }

    public JSValue(long value)
    {
      Type = JSValueType.Int64;
      m_simpleValue = new SimpleValue { Int64Value = value };
      m_objValue = null;
    }

    public JSValue(double value)
    {
      Type = JSValueType.Double;
      m_simpleValue = new SimpleValue { DoubleValue = value };
      m_objValue = null;
    }

    public static implicit operator JSValue(ReadOnlyDictionary<string, JSValue> value) => new JSValue(value);
    public static implicit operator JSValue(Dictionary<string, JSValue> value) => new JSValue(value);
    public static implicit operator JSValue(JSValueObject value) => new JSValue(value);
    public static implicit operator JSValue(ReadOnlyCollection<JSValue> value) => new JSValue(value);
    public static implicit operator JSValue(List<JSValue> value) => new JSValue(value);
    public static implicit operator JSValue(JSValueArray value) => new JSValue(value);
    public static implicit operator JSValue(string value) => new JSValue(value);
    public static implicit operator JSValue(bool value) => new JSValue(value);
    public static implicit operator JSValue(sbyte value) => new JSValue(value);
    public static implicit operator JSValue(short value) => new JSValue(value);
    public static implicit operator JSValue(int value) => new JSValue(value);
    public static implicit operator JSValue(long value) => new JSValue(value);
    public static implicit operator JSValue(byte value) => new JSValue(value);
    public static implicit operator JSValue(ushort value) => new JSValue(value);
    public static implicit operator JSValue(uint value) => new JSValue(value);
    public static implicit operator JSValue(ulong value) => new JSValue((long)value);
    public static implicit operator JSValue(float value) => new JSValue(value);
    public static implicit operator JSValue(double value) => new JSValue(value);

    public JSValueType Type { get; }

    public bool IsNull => Type == JSValueType.Null;

    public bool TryGetObject(out IReadOnlyDictionary<string, JSValue> value)
    {
      value = (Type == JSValueType.Object) ? (IReadOnlyDictionary<string, JSValue>)m_objValue : null;
      return value != null;
    }

    public bool TryGetArray(out IReadOnlyList<JSValue> value)
    {
      value = (Type == JSValueType.Array) ? (IReadOnlyList<JSValue>)m_objValue : null;
      return value != null;
    }

    public bool TryGetString(out string value)
    {
      value = (Type == JSValueType.String) ? (string)m_objValue : null;
      return value != null;
    }

    public bool TryGetBoolean(out bool value)
    {
      if (Type == JSValueType.Boolean)
      {
        value = m_simpleValue.BooleanValue;
        return true;
      }

      value = false;
      return false;
    }

    public bool TryGetInt64(out long value)
    {
      if (Type == JSValueType.Int64)
      {
        value = m_simpleValue.Int64Value;
        return true;
      }

      value = 0;
      return false;
    }

    public bool TryGetDouble(out double value)
    {
      if (Type == JSValueType.Double)
      {
        value = m_simpleValue.DoubleValue;
        return true;
      }

      value = 0;
      return false;
    }

    public IReadOnlyDictionary<string, JSValue> AsObject() =>
      Type == JSValueType.Object ? ObjectValue : EmptyObject.ObjectValue;

    public IReadOnlyList<JSValue> AsArray() =>
      Type == JSValueType.Array ? ArrayValue : EmptyArray.ArrayValue;

    public string AsString()
    {
      switch (Type)
      {
        case JSValueType.Null: return JSConverter.NullString;
        case JSValueType.String: return StringValue;
        case JSValueType.Boolean: return JSConverter.ToJSString(BooleanValue);
        case JSValueType.Int64: return Int64Value.ToString();
        case JSValueType.Double: return JSConverter.ToJSString(DoubleValue);
        default: return "";
      }
    }

    public bool AsBoolean()
    {
      switch (Type)
      {
        case JSValueType.Object: return ObjectValue.Count != 0;
        case JSValueType.Array: return ArrayValue.Count != 0;
        case JSValueType.String: return StringToBoolean.Contains(StringValue);
        case JSValueType.Boolean: return BooleanValue;
        case JSValueType.Int64: return Int64Value != 0;
        case JSValueType.Double: return DoubleValue != 0;
        default: return false;
      }
    }

    public long AsInt64()
    {
      switch (Type)
      {
        case JSValueType.String: return long.TryParse(StringValue, out long result) ? result : 0;
        case JSValueType.Boolean: return BooleanValue ? 1 : 0;
        case JSValueType.Int64: return Int64Value;
        case JSValueType.Double:
          {
            double value = DoubleValue;
            if (double.IsNaN(value))
            {
              return 0;
            }
            else if (double.IsPositiveInfinity(value))
            {
              return long.MaxValue;
            }
            else if (double.IsNegativeInfinity(value))
            {
              return long.MinValue;
            }
            else
            {
              return (long)DoubleValue;
            }
          }
        default: return 0;
      }
    }

    public double AsDouble()
    {
      switch (Type)
      {
        case JSValueType.String: return JSConverter.ToJSNumber(StringValue);
        case JSValueType.Boolean: return BooleanValue ? 1.0 : 0.0;
        case JSValueType.Int64: return Int64Value;
        case JSValueType.Double: return DoubleValue;
        default: return 0;
      }
    }

    public static explicit operator string(JSValue value) => value.AsString();

    public static explicit operator bool(JSValue value) => value.AsBoolean();

    public static explicit operator sbyte(JSValue value) => (sbyte)value.AsInt64();

    public static explicit operator short(JSValue value) => (short)value.AsInt64();

    public static explicit operator int(JSValue value) => (int)value.AsInt64();

    public static explicit operator long(JSValue value) => value.AsInt64();

    public static explicit operator byte(JSValue value) => (byte)value.AsInt64();

    public static explicit operator ushort(JSValue value) => (ushort)value.AsInt64();

    public static explicit operator uint(JSValue value) => (uint)value.AsInt64();

    public static explicit operator ulong(JSValue value) => (ulong)value.AsInt64();

    public static explicit operator float(JSValue value) => (float)value.AsDouble();

    public static explicit operator double(JSValue value) => value.AsDouble();

    public string AsJSString()
    {
      StringBuilder WriteValue(StringBuilder sb, JSValue node)
      {
        switch (node.Type)
        {
          case JSValueType.Null: return sb.Append(JSConverter.NullString);
          case JSValueType.Object: return sb.Append(JSConverter.ObjectString);
          case JSValueType.Array:
            {
              bool start = true;
              foreach (var item in node.ArrayValue)
              {
                if (start)
                {
                  start = false;
                }
                else
                {
                  sb.Append(',');
                }

                WriteValue(sb, item);
              }
              return sb;
            }
          case JSValueType.String: return sb.Append(node.StringValue);
          case JSValueType.Boolean: return sb.Append(JSConverter.ToJSString(node.BooleanValue));
          case JSValueType.Int64: return sb.Append(node.Int64Value);
          case JSValueType.Double: return sb.Append(JSConverter.ToJSString(node.DoubleValue));
          default: return sb;
        }
      }

      switch (Type)
      {
        case JSValueType.Null: return JSConverter.NullString;
        case JSValueType.Object: return JSConverter.ObjectString;
        case JSValueType.Array:
          {
            StringBuilder sb = new StringBuilder();
            WriteValue(sb, this);
            return sb.ToString();
          }
        case JSValueType.String: return StringValue;
        case JSValueType.Boolean: return JSConverter.ToJSString(BooleanValue);
        case JSValueType.Int64: return Int64Value.ToString();
        case JSValueType.Double: return JSConverter.ToJSString(DoubleValue);
        default: return "";
      }
    }

    public bool AsJSBoolean()
    {
      switch (Type)
      {
        case JSValueType.Object: return true;
        case JSValueType.Array: return true;
        case JSValueType.String: return !string.IsNullOrEmpty(StringValue);
        case JSValueType.Boolean: return BooleanValue;
        case JSValueType.Int64: return Int64Value != 0;
        case JSValueType.Double: return !double.IsNaN(DoubleValue) && DoubleValue != 0;
        default: return false;
      }
    }

    public double AsJSNumber()
    {
      switch (Type)
      {
        case JSValueType.Object:
        case JSValueType.Array:
        case JSValueType.String: return JSConverter.ToJSNumber(AsJSString());
        case JSValueType.Boolean: return BooleanValue ? 1 : 0;
        case JSValueType.Int64: return Int64Value;
        case JSValueType.Double: return DoubleValue;
        default: return 0;
      }
    }

    public override string ToString()
    {
      switch (Type)
      {
        case JSValueType.Null: return "null";
        case JSValueType.Object:
          {
            var sb = new StringBuilder();
            sb.Append("{");
            bool start = true;
            foreach (var prop in ObjectValue)
            {
              if (start)
              {
                start = false;
              }
              else
              {
                sb.Append(", ");
              }

              sb.Append(prop.Key);
              sb.Append(": ");
              sb.Append(prop.Value.ToString());
            }

            sb.Append("}");
            return sb.ToString();
          }
        case JSValueType.Array:
          {
            var sb = new StringBuilder();
            sb.Append("[");
            bool start = true;
            foreach (var item in ArrayValue)
            {
              if (start)
              {
                start = false;
              }
              else
              {
                sb.Append(", ");
              }

              sb.Append(item.ToString());
            }

            sb.Length -= 2;
            sb.Append("]");
            return sb.ToString();
          }
        case JSValueType.String: return "\"" + StringValue + "\"";
        case JSValueType.Boolean: return JSConverter.ToJSString(BooleanValue);
        case JSValueType.Int64: return Int64Value.ToString();
        case JSValueType.Double: return JSConverter.ToJSString(DoubleValue);
        default: return "<Unexpected>";
      }
    }

    public T To<T>() => (new JSValueTreeReader(this)).ReadValue<T>();

    public JSValue From<T>(T value)
    {
      var writer = new JSValueTreeWriter();
      writer.WriteValue(value);
      return writer.TakeValue();
    }

    public int PropertyCount => Type == JSValueType.Object ? ObjectValue.Count : 0;

    public bool TryGetObjectProperty(string propertyName, out JSValue value)
    {
      if (TryGetObject(out var objectValue) && objectValue.TryGetValue(propertyName, out value))
      {
        return true;
      }

      value = Null;
      return false;
    }

    public JSValue GetObjectProperty(string propertyName)
    {
      TryGetObjectProperty(propertyName, out JSValue result);
      return result;
    }

    public JSValue this[string propertyName] => GetObjectProperty(propertyName);

    public int ItemCount => Type == JSValueType.Array ? ArrayValue.Count : 0;

    public bool TryGetArrayItem(int index, out JSValue value)
    {
      if (index >= 0 && TryGetArray(out var arrayValue) && index < arrayValue.Count)
      {
        value = arrayValue[index];
        return true;
      }

      value = Null;
      return false;
    }

    public JSValue GetArrayItem(int index)
    {
      TryGetArrayItem(index, out JSValue result);
      return result;
    }

    public JSValue this[int index] => GetArrayItem(index);

    public static bool operator ==(JSValue left, JSValue right)
    {
      return left.Equals(right);
    }

    public static bool operator !=(JSValue left, JSValue right)
    {
      return !left.Equals(right);
    }

    public bool Equals(JSValue other)
    {
      if (Type != other.Type)
      {
        return false;
      }

      switch (Type)
      {
        case JSValueType.Null: return true;
        case JSValueType.Object: return ObjectEquals(ObjectValue, other.ObjectValue);
        case JSValueType.Array: return ArrayEquals(ArrayValue, other.ArrayValue);
        case JSValueType.String: return StringValue == other.StringValue;
        case JSValueType.Boolean: return BooleanValue == other.BooleanValue;
        case JSValueType.Int64: return Int64Value == other.Int64Value;
        case JSValueType.Double: return DoubleValue == other.DoubleValue;
        default: return false;
      }
    }

    public override bool Equals(object obj)
    {
      return (obj is JSValue) && Equals((JSValue)obj);
    }

    public bool JSEquals(JSValue other)
    {
      if (Type == JSValueType.Null || other.Type == JSValueType.Null)
      {
        return Type == other.Type;
      }

      // If one of the types Boolean, Int64, or Double, then compare as Numbers,
      // otherwise compare as strings.
      JSValueType greatestType = Type > other.Type ? Type : other.Type;
      if (greatestType >= JSValueType.Boolean)
      {
        return AsJSNumber() == other.AsJSNumber();
      }
      else
      {
        return AsJSString() == other.AsJSString();
      }
    }

    public override int GetHashCode()
    {
      throw new NotImplementedException("JSValue currently does not support hash codes");
    }

    public static JSValue ReadFrom(IJSValueReader reader)
    {
      if (reader is IJSValueTreeReader treeReader)
      {
        return treeReader.Current;
      }

      return InternalReadFrom(reader);
    }

    public static JSValueObject ReadObjectFrom(IJSValueReader reader)
    {
      if (reader.ValueType == JSValueType.Object)
      {
        if (reader is IJSValueTreeReader treeReader)
        {
          return new JSValueObject(treeReader.Current.ObjectValue);
        }

        return InternalReadObjectFrom(reader);
      }

      return new JSValueObject();
    }

    public static JSValueArray ReadArrayFrom(IJSValueReader reader)
    {
      if (reader.ValueType == JSValueType.Array)
      {
        if (reader is IJSValueTreeReader treeReader)
        {
          return new JSValueArray(treeReader.Current.ArrayValue);
        }

        return InternalReadArrayFrom(reader);
      }

      return new JSValueArray();
    }

    public void WriteTo(IJSValueWriter writer)
    {
      switch (Type)
      {
        case JSValueType.Null: writer.WriteNull(); break;
        case JSValueType.Object: WriteObject(writer, ObjectValue); break;
        case JSValueType.Array: WriteArray(writer, ArrayValue); break;
        case JSValueType.String: writer.WriteString(StringValue); break;
        case JSValueType.Boolean: writer.WriteBoolean(BooleanValue); break;
        case JSValueType.Int64: writer.WriteInt64(Int64Value); break;
        case JSValueType.Double: writer.WriteDouble(DoubleValue); break;
      }
    }

    public static void WriteObject(IJSValueWriter writer, IEnumerable<KeyValuePair<string, JSValue>> value)
    {
      writer.WriteObjectBegin();
      foreach (var keyValue in value)
      {
        writer.WritePropertyName(keyValue.Key);
        keyValue.Value.WriteTo(writer);
      }

      writer.WriteObjectEnd();
    }

    public static void WriteArray(IJSValueWriter writer, IEnumerable<JSValue> value)
    {
      writer.WriteArrayBegin();
      foreach (var item in value)
      {
        item.WriteTo(writer);
      }

      writer.WriteArrayEnd();
    }

    private static bool ObjectEquals(IReadOnlyDictionary<string, JSValue> left, IReadOnlyDictionary<string, JSValue> right)
    {
      if (left == right)
      {
        return true;
      }

      if (left.Count != right.Count)
      {
        return false;
      }

      foreach (var entry in left)
      {
        if (!right.TryGetValue(entry.Key, out var value) || !value.Equals(entry.Value))
        {
          return false;
        }
      }

      return true;
    }

    private static bool ArrayEquals(IReadOnlyList<JSValue> left, IReadOnlyList<JSValue> right)
    {
      if (left == right)
      {
        return true;
      }

      if (left.Count != right.Count)
      {
        return false;
      }

      for (int i = 0; i < left.Count; ++i)
      {
        if (!left[i].Equals(right[i]))
        {
          return false;
        }
      }

      return true;
    }

    private static JSValue InternalReadFrom(IJSValueReader reader)
    {
      switch (reader.ValueType)
      {
        case JSValueType.Null: return Null;
        case JSValueType.Object: return new JSValue(InternalReadObjectFrom(reader));
        case JSValueType.Array: return new JSValue(InternalReadArrayFrom(reader));
        case JSValueType.String: return new JSValue(reader.GetString());
        case JSValueType.Boolean: return new JSValue(reader.GetBoolean());
        case JSValueType.Int64: return new JSValue(reader.GetInt64());
        case JSValueType.Double: return new JSValue(reader.GetDouble());
        default: throw new ReactException("Unexpected JSValueType");
      }
    }

    private static JSValueObject InternalReadObjectFrom(IJSValueReader reader)
    {
      var jsObject = new JSValueObject();
      while (reader.GetNextObjectProperty(out string propertyName))
      {
        jsObject.Add(propertyName, InternalReadFrom(reader));
      }

      return jsObject;
    }

    private static JSValueArray InternalReadArrayFrom(IJSValueReader reader)
    {
      var jsArray = new JSValueArray();
      while (reader.GetNextArrayItem())
      {
        jsArray.Add(InternalReadFrom(reader));
      }

      return jsArray;
    }

    #region Obsolete members

    [Obsolete("Use TryGetObject or AsObject")] public IReadOnlyDictionary<string, JSValue> Object => (IReadOnlyDictionary<string, JSValue>)m_objValue;
    [Obsolete("Use TryGetArray or AsArray")] public IReadOnlyList<JSValue> Array => (IReadOnlyList<JSValue>)m_objValue;
    [Obsolete("Use TryGetString, AsString, or AsJSString")] public string String => (string)m_objValue;
    [Obsolete("Use TryGetBoolean, AsBoolean, or AsJSBoolean")] public bool Boolean => m_simpleValue.BooleanValue;
    [Obsolete("Use TryGetInt64 or AsInt64")] public long Int64 => m_simpleValue.Int64Value;
    [Obsolete("Use TryGetDouble, AsDouble, or AsJSNumber")] public double Double => m_simpleValue.DoubleValue;

    [Obsolete("Use ReadObjectFrom")]
    public static Dictionary<string, JSValue> ReadObjectPropertiesFrom(IJSValueReader reader)
    {
      return ReadObjectFrom(reader);
    }

    [Obsolete("Use ReadArrayFrom")]
    public static List<JSValue> ReadArrayItemsFrom(IJSValueReader reader)
    {
      return ReadArrayFrom(reader);
    }

    #endregion

    private IReadOnlyDictionary<string, JSValue> ObjectValue => (IReadOnlyDictionary<string, JSValue>)m_objValue;
    private IReadOnlyList<JSValue> ArrayValue => (IReadOnlyList<JSValue>)m_objValue;
    private string StringValue => (string)m_objValue;
    private bool BooleanValue => m_simpleValue.BooleanValue;
    private long Int64Value => m_simpleValue.Int64Value;
    private double DoubleValue => m_simpleValue.DoubleValue;

    // This is a 'union' type that may store in the same location bool, long, or double.
    [StructLayout(LayoutKind.Explicit)]
    private struct SimpleValue
    {
      [FieldOffset(0)] public bool BooleanValue;
      [FieldOffset(0)] public long Int64Value;
      [FieldOffset(0)] public double DoubleValue;
    }

    private static class JSConverter
    {
      public static readonly string NullString = "null";
      public static readonly string ObjectString = "[object Object]";

      public static string ToJSString(bool value)
      {
        return value ? "true" : "false";
      }

      public static string ToJSString(double value)
      {
        if (double.IsNaN(value))
        {
          return "NaN";
        }
        else if (double.IsPositiveInfinity(value))
        {
          return "Infinity";
        }
        else if (double.IsNegativeInfinity(value))
        {
          return "-Infinity";
        }
        else
        {
          return value.ToString();
        }
      }

      public static double ToJSNumber(string value)
      {
        var trimmed = value.Trim();
        if (trimmed.Length == 0)
        {
          return 0;
        }

        // NumberStyles.Float does not allow numbers to have thousand's commas insider. 
        return double.TryParse(trimmed, NumberStyles.Float, CultureInfo.InvariantCulture, out double doubleValue)
          ? doubleValue : double.NaN;
      }
    }
  }
}


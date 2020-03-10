// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using Windows.UI.Notifications;

namespace Microsoft.ReactNative.Managed
{
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
    public static readonly JSValue EmptyObject = new Dictionary<string, JSValue>();
    public static readonly JSValue EmptyArray = new List<JSValue>();

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
        m_objValue = value;
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
        m_objValue = value;
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
    public static implicit operator JSValue(ReadOnlyCollection<JSValue> value) => new JSValue(value);
    public static implicit operator JSValue(List<JSValue> value) => new JSValue(value);
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
        case JSValueType.String: return StringValue;
        case JSValueType.Boolean: return BooleanValue ? "true" : "false";
        case JSValueType.Int64: return Int64Value.ToString();
        case JSValueType.Double: return DoubleValue.ToString();
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
        case JSValueType.Double: return (long)DoubleValue;
        default: return 0;
      }
    }

    public double AsDouble()
    {
      switch (Type)
      {
        case JSValueType.String: return double.TryParse(StringValue, out double result) ? result : 0;
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

    public T To<T>() => (new JSValueTreeReader(this)).ReadValue<T>();

    public JSValue From<T>(T value)
    {
      var writer = new JSValueTreeWriter();
      writer.WriteValue(value);
      return writer.TakeValue();
    }

    public string AsJSString()
    {
      switch (Type)
      {
        case JSValueType.Null: return JSConverter.NullString;
        case JSValueType.Object: return JSConverter.ObjectString;
        case JSValueType.Array: return JSConverter.ToJSString(this);
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
        case JSValueType.Object: return double.NaN;
        case JSValueType.Array:
          switch (ArrayValue.Count)
          {
            case 0:
              return 0;
            case 1:
              return ArrayValue[0].AsJSNumber();
            default:
              return double.NaN;
          }
        case JSValueType.String:
          {
            var trimmed = StringValue.Trim();
            if (trimmed.Length == 0)
            {
              return 0;
            }

            return double.TryParse(trimmed, out double doubleValue) ? doubleValue : double.NaN;
          }
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
            foreach (var prop in ObjectValue)
            {
              sb.Append(prop.Key);
              sb.Append(": ");
              sb.Append(prop.Value.ToString());
              sb.Append(", ");
            }
            sb.Length -= 2;
            sb.Append("}");
            return sb.ToString();
          }
        case JSValueType.Array:
          {
            var sb = new StringBuilder();
            sb.Append("[");
            foreach (var item in ArrayValue)
            {
              sb.Append(item.ToString());
              sb.Append(", ");
            }
            sb.Length -= 2;
            sb.Append("]");
            return sb.ToString();
          }
        case JSValueType.String: return "\"" + StringValue + "\"";
        case JSValueType.Boolean: return BooleanValue ? "true" : "false";
        case JSValueType.Int64: return Int64Value.ToString();
        case JSValueType.Double: return DoubleValue.ToString();
        default: return "<Unexpected>";
      }
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
        case JSValueType.Object: return JSValueObject.Equals(ObjectValue, other.ObjectValue);
        case JSValueType.Array: return JSValueArray.Equals(ArrayValue, other.ArrayValue);
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

    public static JSValue InternalReadFrom(IJSValueReader reader)
    {
      switch (reader.ValueType)
      {
        case JSValueType.Null: return Null;
        case JSValueType.Object: return new JSValue(JSValueObject.InternalReadFrom(reader));
        case JSValueType.Array: return new JSValue(JSValueArray.InternalReadFrom(reader));
        case JSValueType.String: return new JSValue(reader.GetString());
        case JSValueType.Boolean: return new JSValue(reader.GetBoolean());
        case JSValueType.Int64: return new JSValue(reader.GetInt64());
        case JSValueType.Double: return new JSValue(reader.GetDouble());
        default: throw new ReactException("Unexpected JSValueType");
      }
    }

    public void WriteTo(IJSValueWriter writer)
    {
      switch (Type)
      {
        case JSValueType.Null: writer.WriteNull(); break;
        case JSValueType.Object: JSValueObject.WriteTo(writer, ObjectValue); break;
        case JSValueType.Array: JSValueArray.WriteTo(writer, ArrayValue); break;
        case JSValueType.String: writer.WriteString(StringValue); break;
        case JSValueType.Boolean: writer.WriteBoolean(BooleanValue); break;
        case JSValueType.Int64: writer.WriteInt64(Int64Value); break;
        case JSValueType.Double: writer.WriteDouble(DoubleValue); break;
      }
    }

    #region Obsolete members

    [Obsolete("Use TryGetObject or AsObject")] public IReadOnlyDictionary<string, JSValue> Object => (IReadOnlyDictionary<string, JSValue>)m_objValue;
    [Obsolete("Use TryGetArray or AsArray")] public IReadOnlyList<JSValue> Array => (IReadOnlyList<JSValue>)m_objValue;
    [Obsolete("Use TryGetString or AsString")] public string String => (string)m_objValue;
    [Obsolete("Use TryGetBoolean or AsBoolean")] public bool Boolean => m_simpleValue.BooleanValue;
    [Obsolete("Use TryGetInt64 or AsInt64")] public long Int64 => m_simpleValue.Int64Value;
    [Obsolete("Use TryGetDouble or AsDouble")] public double Double => m_simpleValue.DoubleValue;

    [Obsolete("Use JSValueObject.ReadFrom")]
    public static Dictionary<string, JSValue> ReadObjectPropertiesFrom(IJSValueReader reader)
    {
      return JSValueObject.ReadFrom(reader).ToDictionary(entry => entry.Key, entry => entry.Value);
    }

    [Obsolete("Use JSValueArray.ReadFrom")]
    public static List<JSValue> ReadArrayItemsFrom(IJSValueReader reader)
    {
      return JSValueArray.ReadFrom(reader).ToList();
    }

    [Obsolete("Use JSValueObject.WriteTo")]
    public static void WriteObject(IJSValueWriter writer, IEnumerable<KeyValuePair<string, JSValue>> value)
    {
      JSValueObject.WriteTo(writer, value);
    }

    [Obsolete("Use JSValueArray.WriteTo")]
    public static void WriteArray(IJSValueWriter writer, IEnumerable<JSValue> value)
    {
      JSValueArray.WriteTo(writer, value);
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
  }

  class JSValueObject : Dictionary<string, JSValue>
  {
    public static implicit operator JSValue(JSValueObject properties) =>
      new JSValue(new ReadOnlyDictionary<string, JSValue>(properties));

    public static bool Equals(IReadOnlyDictionary<string, JSValue> left, IReadOnlyDictionary<string, JSValue> right)
    {
      if (left == right)
      {
        return true;
      }

      if (left == null || right == null || left.Count != right.Count)
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

    public static IReadOnlyDictionary<string, JSValue> ReadFrom(IJSValueReader reader)
    {
      if (reader is IJSValueTreeReader treeReader && treeReader.Current.TryGetObject(out var result))
      {
        return result;
      }

      return InternalReadFrom(reader);
    }

    public static IReadOnlyDictionary<string, JSValue> InternalReadFrom(IJSValueReader reader)
    {
      if (reader.ValueType == JSValueType.Object)
      {
        var result = new JSValueObject();
        while (reader.GetNextObjectProperty(out string propertyName))
        {
          result.Add(propertyName, JSValue.InternalReadFrom(reader));
        }

        return result;
      }

      return JSValue.EmptyObject.AsObject();
    }

    public void WriteTo(IJSValueWriter writer)
    {
      WriteTo(writer, this);
    }

    public static void WriteTo(IJSValueWriter writer, IEnumerable<KeyValuePair<string, JSValue>> value)
    {
      writer.WriteObjectBegin();
      foreach (var keyValue in value)
      {
        writer.WritePropertyName(keyValue.Key);
        keyValue.Value.WriteTo(writer);
      }

      writer.WriteObjectEnd();
    }
  }

  class JSValueArray : List<JSValue>
  {
    public static implicit operator JSValue(JSValueArray items) =>
      new JSValue(new ReadOnlyCollection<JSValue>(items));

    public static bool Equals(IReadOnlyList<JSValue> left, IReadOnlyList<JSValue> right)
    {
      if (left == right)
      {
        return true;
      }

      if (left == null || right == null || left.Count != right.Count)
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

    public static IReadOnlyList<JSValue> ReadFrom(IJSValueReader reader)
    {
      if (reader is IJSValueTreeReader treeReader && treeReader.Current.TryGetArray(out var result))
      {
        return result;
      }

      return InternalReadFrom(reader);
    }

    public static IReadOnlyList<JSValue> InternalReadFrom(IJSValueReader reader)
    {
      if (reader.ValueType == JSValueType.Array)
      {
        var result = new JSValueArray();
        while (reader.GetNextArrayItem())
        {
          result.Add(JSValue.InternalReadFrom(reader));
        }

        return result;
      }

      return JSValue.EmptyArray.AsArray();
    }

    public void WriteTo(IJSValueWriter writer)
    {
      WriteTo(writer, this);
    }

    public static void WriteTo(IJSValueWriter writer, IEnumerable<JSValue> value)
    {
      writer.WriteArrayBegin();
      foreach (var item in value)
      {
        item.WriteTo(writer);
      }

      writer.WriteArrayEnd();
    }
  }

  static class JSConverter
  {
    public static readonly string NullString = "null";
    public static readonly string ObjectString = "[object Object]";

    public static string ToJSString(JSValue value)
    {
      StringBuilder sb = new StringBuilder();

      void WriteValue(JSValue node)
      {
        if (node.IsNull)
        {
          sb.Append(NullString);
        }
        else if (node.Type == JSValueType.Object)
        {
          sb.Append(ObjectString);
        }
        else if (node.TryGetArray(out var arrayValue))
        {
          bool start = true;
          foreach (var item in arrayValue)
          {
            if (start)
            {
              start = false;
            }
            else
            {
              sb.Append(',');
            }

            WriteValue(item);
          }
        }
        else if (node.TryGetString(out var stringValue))
        {
          sb.Append(stringValue);
        }
        else if (node.TryGetBoolean(out var boolValue))
        {
          sb.Append(ToJSString(boolValue));
        }
        else if (node.TryGetInt64(out var intValue))
        {
          sb.Append(intValue);
        }
        else if (node.TryGetDouble(out var doubleValue))
        {
          sb.Append(doubleValue);
        }
      }

      WriteValue(value);
      return sb.ToString();
    }

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
  }
}


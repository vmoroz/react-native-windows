// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Runtime.InteropServices;

namespace Microsoft.ReactNative.Managed
{
  enum JSValueType
  {
    Null,
    Object,
    Array,
    String,
    Boolean,
    Int64,
    Double,
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

    private readonly SimpleValue m_simpleValue;
    private readonly object m_objValue;

    public JSValue(IReadOnlyDictionary<string, JSValue> value)
    {
      Type = JSValueType.Object;
      m_simpleValue = new SimpleValue();
      m_objValue = value;
    }

    public JSValue(IReadOnlyList<JSValue> value)
    {
      Type = JSValueType.Array;
      m_simpleValue = new SimpleValue();
      m_objValue = value;
    }

    public JSValue(string value)
    {
      Type = JSValueType.String;
      m_simpleValue = new SimpleValue();
      m_objValue = value;
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

    public JSValueType Type { get; }

    public bool IsNull => Type == JSValueType.Null;

    public IReadOnlyDictionary<string, JSValue> Object => (IReadOnlyDictionary<string, JSValue>)m_objValue;
    public IReadOnlyList<JSValue> Array => (IReadOnlyList<JSValue>)m_objValue;
    public string String => (string)m_objValue;
    public bool Boolean => m_simpleValue.BooleanValue;
    public long Int64 => m_simpleValue.Int64Value;
    public double Double => m_simpleValue.DoubleValue;

    public static bool operator ==(in JSValue lhs, in JSValue rhs)
    {
      return lhs.ValueEquals(rhs);
    }

    public static bool operator !=(in JSValue lhs, in JSValue rhs)
    {
      return !lhs.ValueEquals(rhs);
    }

    public bool Equals(JSValue other)
    {
      return ValueEquals(other);
    }

    private bool ValueEquals(in JSValue other)
    {
      if (Type == other.Type)
      {
        switch (Type)
        {
          case JSValueType.Null: return true;
          case JSValueType.Object: return ObjectEquals(other.Object);
          case JSValueType.Array: return ArrayEquals(other.Array);
          case JSValueType.String: return String == other.String;
          case JSValueType.Boolean: return Boolean == other.Boolean;
          case JSValueType.Int64: return Int64 == other.Int64;
          case JSValueType.Double: return Double == other.Double;
          default: return false;
        }
      }

      return false;
    }

    private bool ObjectEquals(IReadOnlyDictionary<string, JSValue> other)
    {
      var obj = Object;
      if (obj.Count != other.Count)
      {
        return false;
      }

      foreach (var entry in obj)
      {
        if (!other.TryGetValue(entry.Key, out var value) || !value.ValueEquals(entry.Value))
        {
          return false;
        }
      }

      return true;
    }

    private bool ArrayEquals(IReadOnlyList<JSValue> other)
    {
      var arr = Array;
      if (arr.Count != other.Count)
      {
        return false;
      }

      for (int i = 0; i < arr.Count; ++i)
      {
        if (!arr[i].ValueEquals(other[i]))
        {
          return false;
        }
      }

      return true;
    }

    public override bool Equals(object obj)
    {
      return (obj is JSValue) && Equals((JSValue)obj);
    }

    public override int GetHashCode()
    {
      throw new NotImplementedException("JSValue currently does not support hash codes");
    }

    public static JSValue ReadFrom(IJSValueReader reader)
    {
      var treeReader = reader as IJSValueTreeReader;
      if (treeReader != null)
      {
        return treeReader.Current;
      }

      return ReadValue(reader, reader.ReadNext());
    }

    private static JSValue ReadValue(IJSValueReader reader, JSValueReaderState state)
    {
      switch (state)
      {
        case JSValueReaderState.NullValue: return Null;
        case JSValueReaderState.ObjectBegin: return ReadObject(reader);
        case JSValueReaderState.ArrayBegin: return ReadArray(reader);
        case JSValueReaderState.StringValue: return new JSValue(reader.GetString());
        case JSValueReaderState.BooleanValue: return new JSValue(reader.GetBoolean());
        case JSValueReaderState.Int64Value: return new JSValue(reader.GetInt64());
        case JSValueReaderState.DoubleValue: return new JSValue(reader.GetDouble());
        default: throw new ReactException("Unexpected JSValueReader state");
      }
    }

    private static JSValue ReadObject(IJSValueReader reader)
    {
      var properties = new Dictionary<string, JSValue>();
      JSValueReaderState state;
      while ((state = reader.ReadNext()) != JSValueReaderState.ObjectEnd)
      {
        properties.Add(reader.GetPropertyName(), ReadValue(reader, state));
      }

      return new JSValue(new ReadOnlyDictionary<string, JSValue>(properties));
    }

    private static JSValue ReadArray(IJSValueReader reader)
    {
      var items = new List<JSValue>();
      JSValueReaderState state;
      while ((state = reader.ReadNext()) != JSValueReaderState.ArrayEnd)
      {
        items.Add(ReadValue(reader, state));
      }

      return new JSValue(new ReadOnlyCollection<JSValue>(items));
    }

    public void WriteTo(IJSValueWriter writer)
    {
      switch (Type)
      {
        case JSValueType.Null: writer.WriteNull(); break;
        case JSValueType.Object: WriteObject(writer); break;
        case JSValueType.Array: WriteArray(writer); break;
        case JSValueType.String: writer.WriteString(String); break;
        case JSValueType.Boolean: writer.WriteBoolean(Boolean); break;
        case JSValueType.Int64: writer.WriteInt64(Int64); break;
        case JSValueType.Double: writer.WriteDouble(Double); break;
      }
    }

    private void WriteObject(IJSValueWriter writer)
    {
      writer.WriteObjectBegin();
      foreach (var keyValue in Object)
      {
        writer.WritePropertyName(keyValue.Key);
        keyValue.Value.WriteTo(writer);
      }
      writer.WriteObjectEnd();
    }

    private void WriteArray(IJSValueWriter writer)
    {
      writer.WriteArrayBegin();
      foreach (var value in Array)
      {
        value.WriteTo(writer);
      }
      writer.WriteArrayEnd();
    }

    // This is a 'union' type that may store in the same location bool, long, or double.
    [StructLayout(LayoutKind.Explicit)]
    private struct SimpleValue
    {
      [FieldOffset(0)] public bool BooleanValue;
      [FieldOffset(0)] public long Int64Value;
      [FieldOffset(0)] public double DoubleValue;
    }
  }
}

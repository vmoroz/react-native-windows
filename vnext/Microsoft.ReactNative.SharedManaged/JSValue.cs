using Microsoft.ReactNative.Bridge;
using System.Collections.Generic;
using System.Runtime.InteropServices;

namespace Microsoft.ReactNative.Managed
{
  enum JSValueType
  {
    Null, // Null value must be first for the default initialization.
    Object,
    Array,
    String,
    Boolean,
    Int64,
    Double,
  }

  struct JSValue
  {
    private readonly JSValueType m_type;
    private readonly SimpleValue m_simpleValue;
    private readonly object m_objValue;

    public JSValue(IDictionary<string, JSValue> value)
    {
      m_type = JSValueType.Object;
      m_simpleValue = new SimpleValue();
      m_objValue = value;
    }

    public JSValue(IList<JSValue> value)
    {
      m_type = JSValueType.Array;
      m_simpleValue = new SimpleValue();
      m_objValue = value;
    }

    public JSValue(string value)
    {
      m_type = JSValueType.String;
      m_simpleValue = new SimpleValue();
      m_objValue = value;
    }

    public JSValue(bool value)
    {
      m_type = JSValueType.Boolean;
      m_simpleValue = new SimpleValue { BooleanValue = value };
      m_objValue = null;
    }

    public JSValue(long value)
    {
      m_type = JSValueType.Int64;
      m_simpleValue = new SimpleValue { Int64Value = value };
      m_objValue = null;
    }

    public JSValue(double value)
    {
      m_type = JSValueType.Double;
      m_simpleValue = new SimpleValue { DoubleValue = value };
      m_objValue = null;
    }

    public JSValueType Type { get { return m_type; } }

    public IDictionary<string, JSValue> Object { get { return (IDictionary<string, JSValue>)m_objValue; } }
    public IList<JSValue> Array { get { return (IList<JSValue>)m_objValue; } }
    public string String { get { return (string)m_objValue; } }
    public bool Boolean { get { return m_simpleValue.BooleanValue; } }
    public long Int64 { get { return m_simpleValue.Int64Value; } }
    public double Double { get { return m_simpleValue.DoubleValue; } }

    public static JSValue Null = new JSValue();

    public bool IsNull { get { return m_type == JSValueType.Null; } }

    public static JSValue ReadFrom(IJSValueReader reader)
    {
      return ReadValue(reader, reader.ReadNext());
    }

    private static JSValue ReadValue(IJSValueReader reader, JSValueReaderState state)
    {
      switch (state)
      {
        case JSValueReaderState.ObjectBegin: return ReadObject(reader);
        case JSValueReaderState.ArrayBegin: return ReadArray(reader);
        case JSValueReaderState.NullValue: return Null;
        case JSValueReaderState.BooleanValue: return new JSValue(reader.GetBoolean());
        case JSValueReaderState.Int64Value: return new JSValue(reader.GetInt64());
        case JSValueReaderState.DoubleValue: return new JSValue(reader.GetDouble());
        case JSValueReaderState.StringValue: return new JSValue(reader.GetString());
        default: throw new ReactException("Unexpected JSValueReader state");
      }
    }

    public static JSValue ReadObject(IJSValueReader reader)
    {
      var obj = new Dictionary<string, JSValue>();
      JSValueReaderState state;

      string ReadPropertyName()
      {
        if (state != JSValueReaderState.PropertyName)
        {
          throw new ReactException($"Unexpected JSValueReader state: {state}");
        }

        string result = reader.GetString();
        state = reader.ReadNext();
        return result;
      }

      while ((state = reader.ReadNext()) != JSValueReaderState.ObjectEnd)
      {
        obj.Add(ReadPropertyName(), ReadValue(reader, state));
      }
      return new JSValue(obj);
    }

    public static JSValue ReadArray(IJSValueReader reader)
    {
      var array = new List<JSValue>();
      JSValueReaderState state;
      while ((state = reader.ReadNext()) != JSValueReaderState.ArrayEnd)
      {
        array.Add(ReadValue(reader, state));
      }
      return new JSValue(array);
    }

    public void WriteTo(IJSValueWriter writer)
    {
      switch (m_type)
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

    [StructLayout(LayoutKind.Explicit)]
    private struct SimpleValue
    {
      [FieldOffset(0)] public bool BooleanValue;
      [FieldOffset(0)] public long Int64Value;
      [FieldOffset(0)] public double DoubleValue;
    }
  }
}

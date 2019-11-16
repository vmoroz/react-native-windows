// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Threading;

namespace Microsoft.ReactNative.Managed
{
  delegate void ReadValueDelegate<T>(IJSValueReader reader, out T value);

  // A value can be read from IJSValueReader in one of two ways:
  // 1. There is a ReadValue extension for IJSValueReader interface that matches the type.
  // 2. We can auto-generate the read method for the type.
  static class JSValueReader
  {
    private static IReadOnlyDictionary<Type, Delegate> s_readerDelegates = new Dictionary<Type, Delegate>();

    public static void ReadValue(this IJSValueReader reader, out string value)
    {
      switch (reader.ValueType)
      {
        case JSValueType.String: value = reader.GetString(); break;
        case JSValueType.Boolean: value = reader.GetBoolean() ? "true" : "false"; break;
        case JSValueType.Int64: value = reader.GetInt64().ToString(); break;
        case JSValueType.Double: value = reader.GetDouble().ToString(); break;
        default: value = ""; break;
      }
    }

    public static void ReadValue(this IJSValueReader reader, out bool value)
    {
      switch (reader.ValueType)
      {
        case JSValueType.String: value = !string.IsNullOrEmpty(reader.GetString()); break;
        case JSValueType.Boolean: value = reader.GetBoolean(); break;
        case JSValueType.Int64: value = reader.GetInt64() != 0; break;
        case JSValueType.Double: value = reader.GetDouble() != 0; break;
        default: value = false; break;
      }
    }

    public static void ReadValue(this IJSValueReader reader, out sbyte value)
    {
      reader.ReadValue(out long val);
      value = (sbyte)val;
    }

    public static void ReadValue(this IJSValueReader reader, out short value)
    {
      reader.ReadValue(out long val);
      value = (short)val;
    }

    public static void ReadValue(this IJSValueReader reader, out int value)
    {
      reader.ReadValue(out long val);
      value = (int)val;
    }

    public static void ReadValue(this IJSValueReader reader, out long value)
    {
      switch (reader.ValueType)
      {
        case JSValueType.String:
          {
            if (!long.TryParse(reader.GetString(), out value)) { value = 0; }
            break;
          }
        case JSValueType.Boolean: value = reader.GetBoolean() ? 1 : 0; break;
        case JSValueType.Int64: value = reader.GetInt64(); break;
        case JSValueType.Double: value = (long)reader.GetDouble(); break;
        default: value = 0; break;
      }
    }

    public static void ReadValue(this IJSValueReader reader, out byte value)
    {
      reader.ReadValue(out long val);
      value = (byte)val;
    }

    public static void ReadValue(this IJSValueReader reader, out ushort value)
    {
      reader.ReadValue(out long val);
      value = (ushort)val;
    }

    public static void ReadValue(this IJSValueReader reader, out uint value)
    {
      reader.ReadValue(out long val);
      value = (uint)val;
    }

    public static void ReadValue(this IJSValueReader reader, out ulong value)
    {
      reader.ReadValue(out long val);
      value = (ulong)val;
    }

    public static void ReadValue(this IJSValueReader reader, out float value)
    {
      reader.ReadValue(out double val);
      value = (float)val;
    }

    public static void ReadValue(this IJSValueReader reader, out double value)
    {
      switch (reader.ValueType)
      {
        case JSValueType.String:
          {
            if (!double.TryParse(reader.GetString(), out value)) { value = 0; }
            break;
          }
        case JSValueType.Boolean: value = reader.GetBoolean() ? 1 : 0; break;
        case JSValueType.Int64: value = reader.GetInt64(); break;
        case JSValueType.Double: value = reader.GetDouble(); break;
        default: value = 0; break;
      }
    }

    public static void ReadValue(this IJSValueReader reader, out JSValue value)
    {
      value = JSValue.ReadFrom(reader);
    }

    public static void ReadValue(this IJSValueReader reader, out IDictionary<string, JSValue> value)
    {
      value = JSValue.ReadObjectPropertiesFrom(reader);      
    }

    public static void ReadValue(this IJSValueReader reader, out Dictionary<string, JSValue> value)
    {
      value = JSValue.ReadObjectPropertiesFrom(reader);
    }

    public static void ReadValue(this IJSValueReader reader, out IList<JSValue> value)
    {
      value = JSValue.ReadArrayItemsFrom(reader);
    }

    public static void ReadValue(this IJSValueReader reader, out List<JSValue> value)
    {
      value = JSValue.ReadArrayItemsFrom(reader);
    }

    public static void ReadValue<T>(this IJSValueReader reader, out T value)
    {
      JSValueReader<T>.ReadValue(reader, out value);
    }

    public static T ReadValue<T>(this IJSValueReader reader)
    {
      JSValueReader<T>.ReadValue(reader, out T value);
      return value;
    }

    public static T ReadValue<T>(this JSValue jsValue)
    {
      JSValueReader<T>.ReadValue(new JSValueTreeReader(jsValue), out T value);
      return value;
    }

    public static ReadValueDelegate<T> GetReadValueDelegate<T>()
    {
      return (ReadValueDelegate<T>)GetReadValueDelegate(typeof(T));
    }

    public static Delegate GetReadValueDelegate(Type valueType)
    {
      var generatedDelegate = new Lazy<Delegate>(
        () => JSValueReaderGenerator.GenerateReadValueDelegate(valueType));

      while (true)
      {
        var readerDelegates = s_readerDelegates;
        if (readerDelegates.TryGetValue(valueType, out Delegate readDelegate))
        {
          return readDelegate;
        }

        // The ReadValue delegate is not found. Generate it add try to add to the dictionary atomically.
        var updatedReaderDelegates = new Dictionary<Type, Delegate>(readerDelegates as IDictionary<Type, Delegate>);
        updatedReaderDelegates.Add(valueType, generatedDelegate.Value);
        Interlocked.CompareExchange(ref s_readerDelegates, updatedReaderDelegates, readerDelegates);
      }
    }
  }

  // This class provides constant time access to the ReadValue delegate.
  static class JSValueReader<T>
  {
    public static ReadValueDelegate<T> ReadValue = JSValueReader.GetReadValueDelegate<T>();
  }
}

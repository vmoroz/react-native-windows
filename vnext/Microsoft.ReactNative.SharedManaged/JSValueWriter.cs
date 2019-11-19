// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;

namespace Microsoft.ReactNative.Managed
{
  delegate void WriteValueDelegate<T>(IJSValueWriter writer, T value);

  static class JSValueWriter
  {
    private static IReadOnlyDictionary<Type, Delegate> s_writerDelegates = new Dictionary<Type, Delegate>();

    public static void WriteValue(this IJSValueWriter writer, string value)
    {
      writer.WriteString(value);
    }

    public static void WriteValue(this IJSValueWriter writer, bool value)
    {
      writer.WriteBoolean(value);
    }

    public static void WriteValue(this IJSValueWriter writer, sbyte value)
    {
      writer.WriteInt64(value);
    }

    public static void WriteValue(this IJSValueWriter writer, short value)
    {
      writer.WriteInt64(value);
    }

    public static void WriteValue(this IJSValueWriter writer, int value)
    {
      writer.WriteInt64(value);
    }

    public static void WriteValue(this IJSValueWriter writer, long value)
    {
      writer.WriteInt64(value);
    }

    public static void WriteValue(this IJSValueWriter writer, byte value)
    {
      writer.WriteInt64((long)value);
    }

    public static void WriteValue(this IJSValueWriter writer, ushort value)
    {
      writer.WriteInt64((long)value);
    }

    public static void WriteValue(this IJSValueWriter writer, uint value)
    {
      writer.WriteInt64((long)value);
    }

    public static void WriteValue(this IJSValueWriter writer, ulong value)
    {
      writer.WriteInt64((long)value);
    }

    public static void WriteValue(this IJSValueWriter writer, float value)
    {
      writer.WriteDouble(value);
    }

    public static void WriteValue(this IJSValueWriter writer, double value)
    {
      writer.WriteDouble(value);
    }

    public static void WriteValue(this IJSValueWriter writer, JSValue value)
    {
      value.WriteTo(writer);
    }

    public static void WriteValue(this IJSValueWriter writer, IDictionary<string, JSValue> value)
    {
      JSValue.WriteObject(writer, value);
    }

    public static void WriteValue(this IJSValueWriter writer, Dictionary<string, JSValue> value)
    {
      JSValue.WriteObject(writer, value);
    }

    public static void WriteValue(this IJSValueWriter writer, IList<JSValue> value)
    {
      JSValue.WriteArray(writer, value);
    }

    public static void WriteValue(this IJSValueWriter writer, List<JSValue> value)
    {
      JSValue.WriteArray(writer, value);
    }

    public static void WriteValue<T>(this IJSValueWriter writer, T value)
    {
      JSValueWriter<T>.WriteValue(writer, value);
    }

    public static void WriteProperty<T>(this IJSValueWriter writer, string name, T value)
    {
      writer.WritePropertyName(name);
      writer.WriteValue<T>(value);
    }

    public static WriteValueDelegate<T> GetWriteValueDelegate<T>()
    {
      return (WriteValueDelegate<T>)GetWriteValueDelegate(typeof(T));
    }

    public static Delegate GetWriteValueDelegate(Type valueType)
    {
      var generatedDelegate = new Lazy<Delegate>(
        () => JSValueWriterGenerator.GenerateWriteValueDelegate(valueType));

      while (true)
      {
        var writerDelegates = s_writerDelegates;
        if (writerDelegates.TryGetValue(valueType, out Delegate writeDelegate))
        {
          return writeDelegate;
        }

        // The ReadValue delegate is not found. Generate it add try to add to the dictionary atomically.
        var updatedWriterDelegates = new Dictionary<Type, Delegate>(writerDelegates as IDictionary<Type, Delegate>);
        updatedWriterDelegates.Add(valueType, generatedDelegate.Value);
        Interlocked.CompareExchange(ref s_writerDelegates, updatedWriterDelegates, writerDelegates);
      }
    }
  }

  static class JSValueWriter<T>
  {
    public static WriteValueDelegate<T> WriteValue = JSValueWriter.GetWriteValueDelegate<T>();
  }
}

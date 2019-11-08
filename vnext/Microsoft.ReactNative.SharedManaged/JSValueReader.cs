// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;
using Windows.Devices.Geolocation;
using ReflectionMethodInfo = System.Reflection.MethodInfo;

namespace Microsoft.ReactNative.Managed
{
  delegate bool ReadValueDelegate<T>(IJSValueReader reader, out T value);

  // A value can be read from IJSValueReader in one of two ways:
  // 1. There is a TryReadValue extension for IJSValueReader interface that matches the type.
  // 2. We can auto-generate the read method for the type.
  class JSValueReader
  {
    private IJSValueReader m_reader;
    private JSValueReaderState m_state;

    public void ReadValue(out string value)
    {
      switch (m_state)
      {
        case JSValueReaderState.StringValue: value = m_reader.GetString(); break;
        case JSValueReaderState.BooleanValue: value = m_reader.GetBoolean() ? "true" : "false"; break;
        case JSValueReaderState.Int64Value: value = m_reader.GetInt64().ToString(); break;
        case JSValueReaderState.DoubleValue: value = m_reader.GetDouble().ToString(); break;
        default: value = ""; break;
      }
    }

    public void ReadValue(out bool value)
    {
      switch (m_state)
      {
        case JSValueReaderState.StringValue: value = string.IsNullOrEmpty(m_reader.GetString()); break;
        case JSValueReaderState.BooleanValue: value = m_reader.GetBoolean(); break;
        case JSValueReaderState.Int64Value: value = m_reader.GetInt64() != 0; break;
        case JSValueReaderState.DoubleValue: value = m_reader.GetDouble() != 0; break;
        default: value = false; break;
      }
    }

    public void ReadValue(out sbyte value)
    {
      ReadValue(out long val);
      value = (sbyte)val;
    }

    public void ReadValue(out short value)
    {
      ReadValue(out long val);
      value = (short)val;
    }

    public void ReadValue(out int value)
    {
      ReadValue(out long val);
      value = (int)val;
    }

    public void ReadValue(out long value)
    {
      switch (m_state)
      {
        case JSValueReaderState.StringValue:
          {
            if (!long.TryParse(m_reader.GetString(), out value)) { value = 0; }
            break;
          }
        case JSValueReaderState.BooleanValue: value = m_reader.GetBoolean() ? 1 : 0; break;
        case JSValueReaderState.Int64Value: value = m_reader.GetInt64(); break;
        case JSValueReaderState.DoubleValue: value = (long)m_reader.GetDouble(); break;
        default: value = 0; break;
      }
    }

    public void ReadValue(out byte value)
    {
      ReadValue(out long val);
      value = (byte)val;
    }

    public void ReadValue(out ushort value)
    {
      ReadValue(out long val);
      value = (ushort)val;
    }

    public void ReadValue(out uint value)
    {
      ReadValue(out long val);
      value = (uint)val;
    }

    public void ReadValue(out ulong value)
    {
      ReadValue(out long val);
      value = (ulong)val;
    }

    public void ReadValue(out float value)
    {
      ReadValue(out double val);
      value = (float)val;
    }

    public void ReadValue(out double value)
    {
      switch (m_state)
      {
        case JSValueReaderState.StringValue:
          {
            if (!double.TryParse(m_reader.GetString(), out value)) { value = 0; }
            break;
          }
        case JSValueReaderState.BooleanValue: value = m_reader.GetBoolean() ? 1 : 0; break;
        case JSValueReaderState.Int64Value: value = m_reader.GetInt64(); break;
        case JSValueReaderState.DoubleValue: value = m_reader.GetDouble(); break;
        default: value = 0; break;
      }
    }

#if false
    private static Lazy<Dictionary<Type, ReflectionMethodInfo>> s_readValueMethods =
        new Lazy<Dictionary<Type, ReflectionMethodInfo>>(InitReadValueMethods, LazyThreadSafetyMode.PublicationOnly);

    private static Dictionary<Type, Delegate> s_readers = new Dictionary<Type, Delegate>();

    public static ReflectionMethodInfo GetReadValueMethod(Type valueType)
    {
      ReflectionMethodInfo readValueMethod;
      if (s_readValueMethods.Value.TryGetValue(valueType.MakeByRefType(), out readValueMethod))
      {
        return readValueMethod;
      }

      throw new ReactException($"TryReadValue extension method is not found for type {valueType.MakeByRefType().FullName}.");
    }

    private static Dictionary<Type, ReflectionMethodInfo> InitReadValueMethods()
    {
      var query = from type in typeof(JSValueReader).GetTypeInfo().Assembly.GetTypes()
                  where /*type.IsSealed && !type.IsGenericType && */!type.IsNested
                  from method in type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic)
                  where method.IsDefined(typeof(ExtensionAttribute), false) && method.Name == "TryReadValue"
                  let parameters = method.GetParameters()
                  where parameters.Length == 2 && parameters[0].ParameterType == typeof(IJSValueReader)
                  select new { valueType = parameters[1].ParameterType, method };
      return query.ToDictionary(i => i.valueType, i => i.method);
    }

    public static bool TryReadValue<T>(this IJSValueReader reader, out T value)
    {
      return GetReadValueDelegate<T>()(reader, out value);
    }

    public static bool StartReadArray(this IJSValueReader reader)
    {
      //if (reader.State == JSValueReaderState.ObjectBegin)
      //{
      //  while (reader.ReadNext() != JSValueReaderState.ObjectEnd)
      //  {

      //  }
      //}


      return reader.ReadNext() == JSValueReaderState.ArrayBegin;
    }

    public static bool EndReadArray(this IJSValueReader reader)
    {
      return reader.ReadNext() == JSValueReaderState.ArrayEnd;
    }

    public static ReadValueDelegate<T> GetReadValueDelegate<T>()
    {
      return (ReadValueDelegate<T>)GetReadValueDelegate(typeof(T));
    }

    public static Delegate GetReadValueDelegate(Type valueType)
    {
      return null;
      //var readers = s_readers; // Get the current collection of readers
      //if (readers.TryGetValue(valueType, out Delegate readerDelegate))
      //{
      //  return readerDelegate;
      //}

      //// Initialize it if it was never initialized before.
      //if (!readers.Any())
      //{
      //  InitializeReaders();
      //  return GetReadValueDelegate(valueType);
      //}

      //GenerateReaderDelegate(valueType);
      //return GetReadValueDelegate(valueType);
    }

    //public static bool TryReadArgs<T0>(this IJSValueReader reader, out T0 arg0)
    //{
    //  return reader.StartReadArray()
    //    && GetReadValueDelegate<T0>()(reader, out arg0)
    //    && reader.EndReadArray();
    //}

    private static void InitializeReaders()
    {
      // Only initialize if it is not initialized yet.
      var currentReaders = s_readers;
      if (currentReaders.Any())
      {
        return;
      }

      var newReaders = new Dictionary<Type, Delegate>();
      var readMethods = InitReadValueMethods();
      foreach (var readMethodPair in readMethods)
      {
        Type delegateType = typeof(ReadValueDelegate<>).MakeGenericType(readMethodPair.Key);
        newReaders.Add(readMethodPair.Key, readMethodPair.Value.CreateDelegate(delegateType));
      }

      // Only override the s_readers if another thread did not do it yet.
      Interlocked.CompareExchange(ref s_readers, newReaders, currentReaders);
    }

    private static void GenerateReadValueDelegate(Type valueType)
    {

      //TypeInfo typeInfo = valueType.GetTypeInfo();
      //if (typeInfo.IsEnum)
      //{

      //}
      //else
      //{
      //  valueType.GetInterfaces
      //  bool isStruct = type.IsValueType && !type.IsPrimitive;
      //  bool isClass = type.IsClass;
      //}
      //// Only generate if it is not defined yet.
      //var currentReaders = s_readers;
      //if (currentReaders.Any())
      //{
      //  return;
      //}

    }
#endif
  }
}

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
using ReflectionMethodInfo = System.Reflection.MethodInfo;

namespace Microsoft.ReactNative.Managed
{
  delegate void ReadValueDelegate<T>(IJSValueReader reader, out T value);

  // A value can be read from IJSValueReader in one of two ways:
  // 1. There is a ReadValue extension for IJSValueReader interface that matches the type.
  // 2. We can auto-generate the read method for the type.
  static class JSValueReader
  {
    private static Lazy<IReadOnlyDictionary<Type, ReflectionMethodInfo>> s_readValueExtensionMethods =
        new Lazy<IReadOnlyDictionary<Type, ReflectionMethodInfo>>(GetReadValueExtensionMethods, LazyThreadSafetyMode.PublicationOnly);

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

    public static void ReadValue<T>(this IJSValueReader reader, out T value)
    {
      JSValueReader<T>.ReadValue(reader, out value);
    }

    public static T ReadValue<T>(this IJSValueReader reader)
    {
      JSValueReader<T>.ReadValue(reader, out T value);
      return value;
    }

    public static ReflectionMethodInfo GetReadValueExtensionMethodInfo(Type valueType)
    {
      return s_readValueExtensionMethods.Value.TryGetValue(valueType, out ReflectionMethodInfo methodInfo) ? methodInfo : null;
    }

    public static ReadValueDelegate<T> GetReadValueDelegate<T>()
    {
      return (ReadValueDelegate<T>)GetReadValueDelegate(typeof(T));
    }

    public static Delegate GetReadValueDelegate(Type valueType)
    {
      var generatedDelegate = new Lazy<Delegate>(() => GenerateReadValueDelegate(valueType));
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

    private static Delegate GenerateReadValueDelegate(Type valueType)
    {
      if (TryGenerateReadValueFromExtension(valueType, out Delegate readDelegate)
        || TryGenerateReadValueFromJSValueExtension(valueType, out readDelegate)
        || TryGenerateReadValueForEnum(valueType, out readDelegate)
        || TryGenerateReadValueForNullable(valueType, out readDelegate)
        || TryGenerateReadValueForIDictionary(valueType, out readDelegate)
        || TryGenerateReadValueForIList(valueType, out readDelegate)
        || TryGenerateReadValueForTuple(valueType, out readDelegate)
        || TryGenerateReadValueForClass(valueType, out readDelegate))
      {
        return readDelegate;
      }

      throw new Exception($"Cannot generate ReadValue delegate for type {valueType}");
    }

    private static bool TryGenerateReadValueFromExtension(Type valueType, out Delegate readDelegate)
    {
      // Creates a delegate from the ReadValue extension method
      ReflectionMethodInfo methodInfo = GetReadValueExtensionMethodInfo(valueType);
      if (methodInfo != null)
      {
        readDelegate = methodInfo.CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType));
        return true;
      }

      readDelegate = null;
      return false;
    }

    private static bool TryGenerateReadValueFromJSValueExtension(Type valueType, out Delegate readDelegate)
    {
      readDelegate = null;
      return false;
    }

    private static bool TryGenerateReadValueForEnum(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out EnumType value) =>
      // {
      //   int temp;
      //   reader.ReadValue(out temp);
      //   value = (EnumType)temp;
      // }

      if (valueType.GetTypeInfo().IsEnum)
      {
        ReflectionMethodInfo readValueMethodInfo = GetReadValueExtensionMethodInfo(typeof(int));
        if (readValueMethodInfo != null)
        {
          // Lambda parameters (IJSValueReader reader, out Type value)
          var readerParam = Expression.Parameter(typeof(IJSValueReader), "reader");
          var valueParam = Expression.Parameter(valueType.MakeByRefType(), "value");

          // int temp;
          var varTemp = Expression.Variable(typeof(int), "temp");

          var lambdaBody = Expression.Block(
            new ParameterExpression[] { varTemp },
            // reader.ReadValue(out temp);
            Expression.Call(readValueMethodInfo, readerParam, varTemp),
            // value = (Type)temp;
            Expression.Assign(valueParam, Expression.Convert(varTemp, valueType)));

          var lambdaType = typeof(ReadValueDelegate<>).MakeGenericType(valueType);
          var lambda = Expression.Lambda(lambdaType, lambdaBody, readerParam, valueParam);

          readDelegate = lambda.Compile();
          return true;
        }
      }

      readDelegate = null;
      return false;
    }

    private static bool TryGenerateReadValueForNullable(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out Nullable<Type> value) =>
      // {
      //   if (reader.ValueType != JSValueType.Null)
      //   {
      //     Type temp;
      //     reader.ReadValue(out temp);
      //     value = temp;
      //   }
      //   else
      //   {
      //     value = null;
      //   }
      // }

      var typeInfo = valueType.GetTypeInfo();
      if (typeInfo.IsGenericType && typeInfo.GetGenericTypeDefinition() == typeof(Nullable<>))
      {
        var typeArg = typeInfo.GenericTypeArguments[0];
        Delegate typeArgReadValue = GetReadValueDelegate(typeArg);
        if (typeArgReadValue != null)
        {
          // Lambda parameters (IJSValueReader reader, out Type value)
          var readerParam = Expression.Parameter(typeof(IJSValueReader), "reader");
          var valueParam = Expression.Parameter(valueType.MakeByRefType(), "value");

          // (reader.ValueType != JSValueType.Null)
          var testExpr = Expression.NotEqual(
            Expression.Property(readerParam, nameof(IJSValueReader.ValueType)),
            Expression.Constant(JSValueType.Null));

          var typeArgReadValueType = typeof(ReadValueDelegate<>).MakeGenericType(typeArg);
          var typeArgReadValueDelegate = Expression.Convert(Expression.Constant(typeArgReadValue), typeArgReadValueType);

          // Type temp;
          var varTemp = Expression.Variable(typeArg, "temp");
          var assignValue = Expression.Block(
            new ParameterExpression[] { varTemp },
            // reader.ReadValue(out temp);
            Expression.Invoke(typeArgReadValueDelegate, readerParam, varTemp),
            // value = (Type)temp;
            Expression.Assign(valueParam, Expression.Convert(varTemp, valueType)));

          // value = null;
          var assignNull = Expression.Assign(valueParam, Expression.New(valueType));

          var lambdaBody = Expression.IfThenElse(testExpr, assignValue, assignNull);

          var lambdaType = typeof(ReadValueDelegate<>).MakeGenericType(valueType);
          var lambda = Expression.Lambda(lambdaType, lambdaBody, readerParam, valueParam);

          readDelegate = lambda.Compile();
          return true;
        }
      }

      readDelegate = null;
      return false;
    }

    private static bool TryGenerateReadValueForIDictionary(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out IDictionary<string, Type> value) =>
      // {
      //   Dictionary<string, T> dictionary = new Dictionary<string, T>();
      //   string propertyName;
      //   while (reader.GetNextArrayProperty(out propertyName))
      //   {
      //     Type temp;
      //     reader.ReadValue(out temp);
      //     dictionary.Add(propertyName, temp);
      //   }
      //   value = dictionary;
      // }

      var iDictionaryQuery =
        from intfType in valueType.GetInterfaces().Concat(new[] { valueType })
        let intfTypeInfo = intfType.GetTypeInfo()
        where intfTypeInfo.IsInterface
        && intfTypeInfo.IsGenericType
        && intfTypeInfo.GetGenericTypeDefinition() == typeof(IDictionary<,>)
        && intfTypeInfo.GenericTypeArguments[0] == typeof(string)
        select intfTypeInfo.GenericTypeArguments[1];

      Type typeArg = iDictionaryQuery.FirstOrDefault();
      if (typeArg != null)
      {
        Delegate typeArgReadValue = GetReadValueDelegate(typeArg);
        if (typeArgReadValue != null)
        {
          // Lambda parameters (IJSValueReader reader, out Type value)
          var readerParam = Expression.Parameter(typeof(IJSValueReader), "reader");
          var valueParam = Expression.Parameter(valueType.MakeByRefType(), "value");

          var typeArgReadValueType = typeof(ReadValueDelegate<>).MakeGenericType(typeArg);
          var typeArgReadValueDelegate = Expression.Convert(Expression.Constant(typeArgReadValue), typeArgReadValueType);

          // Dictionary<string, T> list = new Dictionary<string, T>();
          var dictionaryType = typeof(Dictionary<,>).MakeGenericType(typeof(string), typeArg);
          var varDictionary = Expression.Variable(dictionaryType, "dictionary");
          var createDictionary = Expression.Assign(varDictionary, Expression.New(dictionaryType));

          // string propertyName;
          var varPropertyName = Expression.Variable(typeof(string), "propertyName");

          // Type temp;
          var varTemp = Expression.Variable(typeArg, "temp");
          var addMetodInfo = dictionaryType.GetMethod("Add");
          var whileBody = Expression.Block(
            new ParameterExpression[] { varTemp },
            // reader.ReadValue(out temp);
            Expression.Invoke(typeArgReadValueDelegate, readerParam, varTemp),
            // value.Add(temp);
            Expression.Call(varDictionary, addMetodInfo, varPropertyName, varTemp));

          // Creating a label to jump to from a loop.
          LabelTarget label = Expression.Label(typeof(void));

          var whileLoop =
            Expression.Loop(
              Expression.IfThenElse(
                Expression.Call(readerParam, typeof(IJSValueReader).GetMethod(nameof(IJSValueReader.GetNextObjectProperty)), varPropertyName),
                whileBody,
                Expression.Break(label)), label);

          // value = dictionary;
          var valueAssign = Expression.Assign(valueParam, varDictionary);

          var lambdaBody = Expression.Block(
            new ParameterExpression[] { varDictionary, varPropertyName },
            createDictionary, whileLoop, valueAssign);

          var lambdaType = typeof(ReadValueDelegate<>).MakeGenericType(valueType);
          var lambda = Expression.Lambda(lambdaType, lambdaBody, readerParam, valueParam);

          readDelegate = lambda.Compile();
          return true;
        }
      }

      readDelegate = null;
      return false;
    }

    private static bool TryGenerateReadValueForIList(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out IList<Type> value) =>
      // {
      //   List<T> list = new List<T>();
      //   while (reader.GetNextArrayItem())
      //   {
      //     Type temp;
      //     reader.ReadValue(out temp);
      //     list.Add(temp);
      //   }
      //   value = list;
      // }

      var iListQuery =
        from intfType in valueType.GetInterfaces().Concat(new[] { valueType })
        let intfTypeInfo = intfType.GetTypeInfo()
        where intfTypeInfo.IsInterface
        && intfTypeInfo.IsGenericType
        && intfTypeInfo.GetGenericTypeDefinition() == typeof(IList<>)
        select intfTypeInfo.GenericTypeArguments[0];

      Type typeArg = iListQuery.FirstOrDefault();
      if (typeArg != null)
      {
        Delegate typeArgReadValue = GetReadValueDelegate(typeArg);
        if (typeArgReadValue != null)
        {
          // Lambda parameters (IJSValueReader reader, out Type value)
          var readerParam = Expression.Parameter(typeof(IJSValueReader), "reader");
          var valueParam = Expression.Parameter(valueType.MakeByRefType(), "value");

          var typeArgReadValueType = typeof(ReadValueDelegate<>).MakeGenericType(typeArg);
          var typeArgReadValueDelegate = Expression.Convert(Expression.Constant(typeArgReadValue), typeArgReadValueType);

          // List<T> list = new List<T>();
          var listType = typeof(List<>).MakeGenericType(typeArg);
          var varList = Expression.Variable(listType, "list");
          var createList = Expression.Assign(varList, Expression.New(listType));

          // Type temp;
          var varTemp = Expression.Variable(typeArg, "temp");
          var addMetodInfo = listType.GetMethod("Add");
          var whileBody = Expression.Block(
            new ParameterExpression[] { varTemp },
            // reader.ReadValue(out temp);
            Expression.Invoke(typeArgReadValueDelegate, readerParam, varTemp),
            // value.Add(temp);
            Expression.Call(varList, addMetodInfo, varTemp));

          // Creating a label to jump to from a loop.
          LabelTarget label = Expression.Label(typeof(void));

          var whileLoop =
            Expression.Loop(
              Expression.IfThenElse(
                Expression.Call(readerParam, typeof(IJSValueReader).GetMethod(nameof(IJSValueReader.GetNextArrayItem))),
                whileBody,
                Expression.Break(label)), label);

          // value = list;
          var valueAssign = Expression.Assign(valueParam, varList);

          var lambdaBody = Expression.Block(
            new ParameterExpression[] { varList },
            createList, whileLoop, valueAssign);

          var lambdaType = typeof(ReadValueDelegate<>).MakeGenericType(valueType);
          var lambda = Expression.Lambda(lambdaType, lambdaBody, readerParam, valueParam);

          readDelegate = lambda.Compile();
          return true;
        }
      }

      readDelegate = null;
      return false;
    }

    private static bool TryGenerateReadValueForTuple(Type valueType, out Delegate readDelegate)
    {
      readDelegate = null;
      return false;
    }

    private static bool TryGenerateReadValueForClass(Type valueType, out Delegate readDelegate)
    {
      readDelegate = null;
      return false;
    }

    private static Dictionary<Type, ReflectionMethodInfo> GetReadValueExtensionMethods()
    {
      var extensionMethods =
        from type in typeof(JSValueReader).GetTypeInfo().Assembly.GetTypes()
        let typeInfo = type.GetTypeInfo()
        where typeInfo.IsSealed && !typeInfo.IsGenericType && !typeInfo.IsNested
        from method in type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic)
        where method.IsDefined(typeof(ExtensionAttribute), inherit: false) && method.Name == "ReadValue"
        let parameters = method.GetParameters()
        where parameters.Length == 2
        && parameters[0].ParameterType == typeof(IJSValueReader)
        && parameters[1].IsOut
        select new { valueType = parameters[1].ParameterType.GetElementType(), method };
      return extensionMethods.ToDictionary(i => i.valueType, i => i.method);
    }
  }

  // This class provides constant time access to the ReadValue delegate.
  static class JSValueReader<T>
  {
    public static ReadValueDelegate<T> ReadValue = JSValueReader.GetReadValueDelegate<T>();
  }
}

using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;
using static System.Linq.Expressions.Expression;

namespace Microsoft.ReactNative.Managed
{
  static class JSValueReaderGenerator
  {
    private static Lazy<IReadOnlyDictionary<Type, MethodInfo>> s_readValueExtensionMethods =
      new Lazy<IReadOnlyDictionary<Type, MethodInfo>>(
        () => GetReadValueExtensionMethods(typeof(IJSValueReader)),
        LazyThreadSafetyMode.PublicationOnly);

    private static Lazy<IReadOnlyDictionary<Type, MethodInfo>> s_jsValueReadValueExtensionMethods =
      new Lazy<IReadOnlyDictionary<Type, MethodInfo>>(
        () => GetReadValueExtensionMethods(typeof(JSValue)),
        LazyThreadSafetyMode.PublicationOnly);

    public class VariableWrapper
    {
      public static VariableWrapper CreateVariable(Type type, Expression init)
      {
        return new VariableWrapper
        {
          Type = type,
          AsExpression = Expression.Variable(type),
          Init = init,
          IsParameter = false
        };
      }

      public static VariableWrapper CreateParameter(Type type)
      {
        return new VariableWrapper
        {
          Type = type,
          AsExpression = Expression.Parameter(type),
          IsParameter = true
        };
      }

      public bool IsParameter { get; private set; }

      public Expression Init { get; private set; }

      public Type Type { get; private set; }

      public ParameterExpression AsExpression { get; private set; }

      public static implicit operator ParameterExpression(VariableWrapper v) => v.AsExpression;

      public Expression Assign(Expression value)
      {
        return Expression.Assign(AsExpression, value);
      }

      public Expression Call(MethodInfo method, params Expression[] args)
      {
        return Expression.Call(AsExpression, method, args);
      }

      public Expression Call(string methodName, params Expression[] args)
      {
        return Call(Type.GetMethod(methodName), args);
      }

      public Expression CallExt(MethodInfo method)
      {
        return Expression.Call(method, AsExpression);
      }

      public Expression CallExt(MethodInfo method, Expression arg0)
      {
        return Expression.Call(method, AsExpression, arg0);
      }

      public Expression Property(string propertyName)
      {
        return PropertyOrField(AsExpression, propertyName);
      }

      public Expression AssignProperty(string propertyName, Expression value)
      {
        return Expression.Assign(Property(propertyName), value);
      }

      public Expression AssignPropertyVoid(string propertyName, Expression value)
      {
        return Expression.Block(AssignProperty(propertyName, value), Default(typeof(void)));
      }
    }

    public class TypeWrapper
    {
      public TypeWrapper(Type type)
      {
        Type = type;
      }

      public Type Type { get; private set; }

      public Delegate CompileLambda(params object[] expressions)
      {
        var parameters = new List<VariableWrapper>();
        var variables = new List<VariableWrapper>();
        var body = new List<Expression>();

        foreach (var item in expressions)
        {
          switch (item)
          {
            case VariableWrapper parameter when parameter.IsParameter: parameters.Add(parameter); break;
            case VariableWrapper variable when !variable.IsParameter:
              variables.Add(variable);
              if (variable.Init != null)
              {
                body.Add(variable.Assign(variable.Init));
              }
              break;
            case Expression expression: body.Add(expression); break;
          }
        }

        var block = Expression.Block(variables.Select(v => v.AsExpression), body);
        var lambda = Lambda(Type, block, parameters.Select(p => p.AsExpression));
        return lambda.Compile();
      }
    }

    public static VariableWrapper Variable(Type type, out VariableWrapper variable, Expression init = null)
    {
      return variable = VariableWrapper.CreateVariable(type, init);
    }

    public static VariableWrapper Parameter(Type type, out VariableWrapper parameter)
    {
      return parameter = VariableWrapper.CreateParameter(type);
    }

    public static BlockExpression Block(params object[] expressions)
    {
      var variables = new List<VariableWrapper>();
      var body = new List<Expression>();

      foreach (var item in expressions)
      {
        switch (item)
        {
          case VariableWrapper variable:
            variables.Add(variable);
            if (variable.Init != null)
            {
              body.Add(variable.Assign(variable.Init));
            }
            break;
          case Expression expression: body.Add(expression); break;
        }
      }

      return Expression.Block(variables.Select(v => v.AsExpression), body);
    }

    public static Expression Convert(Type type, Expression expression)
    {
      return Expression.Convert(expression, type);
    }

    public static Expression While(Expression condition, Expression body)
    {
      // A label to jump to from a loop.
      LabelTarget breakLabel = Label(typeof(void));

      // Execute loop while condition is true.
      return Loop(IfThenElse(condition, body, Break(breakLabel)), breakLabel);
    }

    static MethodInfo GetNextObjectProperty =>
      typeof(IJSValueReader).GetMethod(nameof(IJSValueReader.GetNextObjectProperty));

    private static MethodInfo MethodOf(string methodName, params Type[] typeArgs) =>
      typeof(JSValueReaderGenerator).GetMethod(methodName).MakeGenericMethod(typeArgs);

    static string ValueType => nameof(IJSValueReader.ValueType);

    static TypeWrapper ReadValueDelegateOf(Type typeArg) =>
      new TypeWrapper(typeof(ReadValueDelegate<>).MakeGenericType(typeArg));

    static MethodInfo ReadValueOf(Type typeArg)
    {
      var readValueGenericMethod = (
          from method in typeof(JSValueReader).GetMethods(BindingFlags.Static | BindingFlags.Public)
          where method.IsGenericMethod && method.IsDefined(typeof(ExtensionAttribute), inherit: false)
          && method.Name == nameof(JSValueReader.ReadValue)
          let parameters = method.GetParameters()
          where parameters.Length == 1
          && parameters[0].ParameterType == typeof(IJSValueReader)
          select method
        ).First();
      return readValueGenericMethod.MakeGenericMethod(typeArg);
    }

    static MethodInfo JSValueReadFrom => typeof(JSValue).GetMethod(nameof(JSValue.ReadFrom));

    public static bool TryGetTypeArgs(Type type, Type genericType, out Type typeArg)
    {
      var typeInfo = type.GetTypeInfo();
      typeArg = (typeInfo.IsGenericType && typeInfo.GetGenericTypeDefinition() == genericType)
        ? typeInfo.GenericTypeArguments[0]
        : null;
      return typeArg != null;
    }

    public static bool TryGetTypeArgs(Type type, Type genericType, out Type[] typeArgs)
    {
      var typeInfo = type.GetTypeInfo();
      typeArgs = (typeInfo.IsGenericType && typeInfo.GetGenericTypeDefinition() == genericType)
        ? typeInfo.GenericTypeArguments
        : null;
      return typeArgs != null;
    }

    public static bool TryGetTypeArgsWithString(Type type, Type genericType, out Type typeArg)
    {
      typeArg = TryGetTypeArgs(type, genericType, out Type[] typeArgs) && typeArgs[0] == typeof(string)
        ? typeArgs[1]
        : null;
      return typeArg != null;
    }

    public static bool TryGetArrayElementType(Type type, out Type elementType)
    {
      var typeInfo = type.GetTypeInfo();
      elementType = typeInfo.IsArray ? typeInfo.GetElementType() : null;
      return elementType != null;
    }

    public static Delegate GenerateReadValueDelegate(Type valueType)
    {
      if (TryGenerateReadValueFromExtension(valueType, out Delegate readDelegate)
        || TryGenerateReadValueFromJSValueExtension(valueType, out readDelegate)
        || TryGenerateReadValueForEnum(valueType, out readDelegate)
        || TryGenerateReadValueForNullable(valueType, out readDelegate)
        || TryGenerateReadValueForDictionary(valueType, out readDelegate)
        || TryGenerateReadValueForList(valueType, out readDelegate)
        || TryGenerateReadValueForTuple(valueType, out readDelegate)
        || TryGenerateReadValueForClass(valueType, out readDelegate))
      {
        return readDelegate;
      }

      throw new Exception($"Cannot generate ReadValue delegate for type {valueType}");
    }

    private static Dictionary<Type, MethodInfo> GetReadValueExtensionMethods(Type extendedType)
    {
      var extensionMethods =
        from type in typeof(JSValueReader).GetTypeInfo().Assembly.GetTypes()
        let typeInfo = type.GetTypeInfo()
        where typeInfo.IsSealed && !typeInfo.IsGenericType && !typeInfo.IsNested
        from method in type.GetMethods(BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic)
        where method.IsDefined(typeof(ExtensionAttribute), inherit: false) && method.Name == nameof(JSValueReader.ReadValue)
        let parameters = method.GetParameters()
        where parameters.Length == 2
        && parameters[0].ParameterType == extendedType
        && parameters[1].IsOut
        select new { valueType = parameters[1].ParameterType.GetElementType(), method };
      return extensionMethods.ToDictionary(i => i.valueType, i => i.method);
    }

    private static bool TryGenerateReadValueFromExtension(Type valueType, out Delegate readDelegate)
    {
      // Creates a delegate from the ReadValue extension method
      readDelegate = s_readValueExtensionMethods.Value.TryGetValue(valueType, out MethodInfo methodInfo)
        ? methodInfo.CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType))
        : null;

      return readDelegate != null;
    }

    private static bool TryGenerateReadValueFromJSValueExtension(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out Type value) =>
      // {
      //   var jsValue = JSValue.ReadFrom(reader);
      //   jsValue.ReadValue(out value);
      // }

      readDelegate = s_jsValueReadValueExtensionMethods.Value.TryGetValue(valueType, out MethodInfo jsValueReadValue)
        ? ReadValueDelegateOf(valueType).CompileLambda(
            Parameter(typeof(IJSValueReader), out var reader),
            Parameter(valueType.MakeByRefType(), out var value),
            Variable(typeof(JSValue), out var jsValue, Call(JSValueReadFrom, reader)),
            jsValue.CallExt(jsValueReadValue, value))
        : null;

      return readDelegate != null;
    }

    public static void ReadEnum<TEnum>(IJSValueReader reader, out TEnum value) where TEnum : Enum
    {
      value = (TEnum)(object)reader.ReadValue<int>();
    }

    private static bool TryGenerateReadValueForEnum(Type valueType, out Delegate readDelegate)
    {
      // Creates a delegate from the ReadEnum method
      readDelegate = valueType.GetTypeInfo().IsEnum
        ? MethodOf(nameof(ReadEnum), valueType).CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType))
        : null;

      return readDelegate != null;
    }

    public static void ReadNullable<T>(IJSValueReader reader, out T? value) where T : struct
    {
      if (reader.ValueType != JSValueType.Null)
      {
        value = reader.ReadValue<T>();
      }
      else
      {
        value = null;
      }
    }

    private static bool TryGenerateReadValueForNullable(Type valueType, out Delegate readDelegate)
    {
      // Creates a delegate from the ReadNullable method
      readDelegate = TryGetTypeArgs(valueType, typeof(Nullable<>), out Type typeArg)
        ? MethodOf(nameof(ReadNullable), typeArg).CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType))
        : null;

      return readDelegate != null;
    }

    public static void ReadDictionary<T>(IJSValueReader reader, out Dictionary<string, T> value)
    {
      value = new Dictionary<string, T>();
      if (reader.ValueType == JSValueType.Object)
      {
        while (reader.GetNextObjectProperty(out string propertyName))
        {
          value.Add(propertyName, reader.ReadValue<T>());
        }
      }
    }

    public static void ReadIDictionary<T>(IJSValueReader reader, out IDictionary<string, T> value)
    {
      ReadDictionary(reader, out Dictionary<string, T> dictionary);
      value = dictionary;
    }

    public static void ReadICollectionOfKeyValuePair<T>(IJSValueReader reader, out ICollection<KeyValuePair<string, T>> value)
    {
      ReadDictionary(reader, out Dictionary<string, T> dictionary);
      value = dictionary;
    }

    public static void ReadIEnumerableOfKeyValuePair<T>(IJSValueReader reader, out IEnumerable<KeyValuePair<string, T>> value)
    {
      ReadDictionary(reader, out Dictionary<string, T> dictionary);
      value = dictionary;
    }

    public static void ReadReadOnlyDictionary<T>(IJSValueReader reader, out ReadOnlyDictionary<string, T> value)
    {
      ReadDictionary(reader, out Dictionary<string, T> dictionary);
      value = new ReadOnlyDictionary<string, T>(dictionary);
    }

    public static void ReadIReadOnlyDictionary<T>(IJSValueReader reader, out IReadOnlyDictionary<string, T> value)
    {
      ReadReadOnlyDictionary(reader, out ReadOnlyDictionary<string, T> dictionary);
      value = dictionary;
    }

    public static void ReadIReadOnlyCollectionOfKeyValuePair<T>(IJSValueReader reader, out IReadOnlyCollection<KeyValuePair<string, T>> value)
    {
      ReadReadOnlyDictionary(reader, out ReadOnlyDictionary<string, T> dictionary);
      value = dictionary;
    }

    private static bool TryGenerateReadValueForDictionary(Type valueType, out Delegate readDelegate)
    {
      MethodInfo methodInfo = null;
      if (TryGetTypeArgsWithString(valueType, typeof(Dictionary<,>), out Type typeArg))
      {
        methodInfo = MethodOf(nameof(ReadDictionary), typeArg);
      }
      else if (TryGetTypeArgsWithString(valueType, typeof(IDictionary<,>), out typeArg))
      {
        methodInfo = MethodOf(nameof(ReadIDictionary), typeArg);
      }
      else if (TryGetTypeArgs(valueType, typeof(ICollection<>), out Type pairType)
        && TryGetTypeArgsWithString(pairType, typeof(KeyValuePair<,>), out typeArg))
      {
        methodInfo = MethodOf(nameof(ReadICollectionOfKeyValuePair), typeArg);
      }
      else if (TryGetTypeArgs(valueType, typeof(IEnumerable<>), out pairType)
        && TryGetTypeArgsWithString(pairType, typeof(KeyValuePair<,>), out typeArg))
      {
        methodInfo = MethodOf(nameof(ReadIEnumerableOfKeyValuePair), typeArg);
      }
      else if (TryGetTypeArgsWithString(valueType, typeof(ReadOnlyDictionary<,>), out typeArg))
      {
        methodInfo = MethodOf(nameof(ReadReadOnlyDictionary), typeArg);
      }
      else if (TryGetTypeArgsWithString(valueType, typeof(IReadOnlyDictionary<,>), out typeArg))
      {
        methodInfo = MethodOf(nameof(ReadIReadOnlyDictionary), typeArg);
      }
      else if (TryGetTypeArgs(valueType, typeof(IReadOnlyCollection<>), out pairType)
        && TryGetTypeArgsWithString(pairType, typeof(KeyValuePair<,>), out typeArg))
      {
        methodInfo = MethodOf(nameof(ReadIReadOnlyCollectionOfKeyValuePair), typeArg);
      }

      readDelegate = methodInfo?.CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType));
      return readDelegate != null;
    }

    public static void ReadList<T>(IJSValueReader reader, out List<T> value)
    {
      value = new List<T>();
      if (reader.ValueType == JSValueType.Array)
      {
        while (reader.GetNextArrayItem())
        {
          value.Add(reader.ReadValue<T>());
        }
      }
    }

    public static void ReadIList<T>(IJSValueReader reader, out IList<T> value)
    {
      ReadList(reader, out List<T> list);
      value = list;
    }

    public static void ReadICollection<T>(IJSValueReader reader, out ICollection<T> value)
    {
      ReadList(reader, out List<T> list);
      value = list;
    }

    public static void ReadIEnumerable<T>(IJSValueReader reader, out IEnumerable<T> value)
    {
      ReadList(reader, out List<T> list);
      value = list;
    }

    public static void ReadReadOnlyCollection<T>(IJSValueReader reader, out ReadOnlyCollection<T> value)
    {
      ReadList(reader, out List<T> list);
      value = new ReadOnlyCollection<T>(list);
    }

    public static void ReadIReadOnlyList<T>(IJSValueReader reader, out IReadOnlyList<T> value)
    {
      ReadReadOnlyCollection(reader, out ReadOnlyCollection<T> collection);
      value = collection;
    }

    public static void ReadIReadOnlyCollection<T>(IJSValueReader reader, out IReadOnlyCollection<T> value)
    {
      ReadReadOnlyCollection(reader, out ReadOnlyCollection<T> collection);
      value = collection;
    }

    public static void ReadArray<T>(IJSValueReader reader, out T[] value)
    {
      ReadList(reader, out List<T> list);
      value = list.ToArray();
    }

    private static bool TryGenerateReadValueForList(Type valueType, out Delegate readDelegate)
    {
      MethodInfo methodInfo =
        valueType.MatchMethodInfo(typeof(List<>), nameof(ReadList))
        ?? valueType.MatchMethodInfo(typeof(IList<>), nameof(ReadIList))
        ?? valueType.MatchMethodInfo(typeof(ICollection<>), nameof(ReadICollection))
        ?? valueType.MatchMethodInfo(typeof(IEnumerable<>), nameof(ReadIEnumerable))
        ?? valueType.MatchMethodInfo(typeof(ReadOnlyCollection<>), nameof(ReadReadOnlyCollection))
        ?? valueType.MatchMethodInfo(typeof(IReadOnlyList<>), nameof(ReadIReadOnlyList))
        ?? valueType.MatchMethodInfo(typeof(IReadOnlyCollection<>), nameof(ReadIReadOnlyCollection))
        ?? valueType.MatchMethodInfo(typeof(IReadOnlyCollection<>), nameof(ReadArray))
        ?? (TryGetArrayElementType(valueType, out var typeArg) ? MethodOf(nameof(ReadArray), typeArg) : null);

      readDelegate = methodInfo?.CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType));
      return readDelegate != null;
    }

    private static void SkipArrayToEnd(IJSValueReader reader)
    {
      while (reader.GetNextArrayItem())
      {
        reader.ReadValue<JSValue>(); // Read and ignore the value
      }
    }

    public static void ReadTuple1<T1>(IJSValueReader reader, out Tuple<T1> value)
    {
      value = default;
      if (reader.ValueType != JSValueType.Array) return;
      if (!reader.GetNextArrayItem()) return;
      T1 t1 = reader.ReadValue<T1>();
      SkipArrayToEnd(reader);
      value = new Tuple<T1>(t1);
    }

    public static void ReadTuple2<T1, T2>(IJSValueReader reader, out Tuple<T1, T2> value)
    {
      value = default;
      if (reader.ValueType != JSValueType.Array) return;
      if (!reader.GetNextArrayItem()) return;
      T1 t1 = reader.ReadValue<T1>();
      if (!reader.GetNextArrayItem()) return;
      T2 t2 = reader.ReadValue<T2>();
      SkipArrayToEnd(reader);
      value = new Tuple<T1, T2>(t1, t2);
    }

    public static void ReadTuple3<T1, T2, T3>(IJSValueReader reader, out Tuple<T1, T2, T3> value)
    {
      value = default;
      if (reader.ValueType != JSValueType.Array) return;
      if (!reader.GetNextArrayItem()) return;
      T1 t1 = reader.ReadValue<T1>();
      if (!reader.GetNextArrayItem()) return;
      T2 t2 = reader.ReadValue<T2>();
      if (!reader.GetNextArrayItem()) return;
      T3 t3 = reader.ReadValue<T3>();
      SkipArrayToEnd(reader);
      value = new Tuple<T1, T2, T3>(t1, t2, t3);
    }

    public static void ReadTuple4<T1, T2, T3, T4>(IJSValueReader reader, out Tuple<T1, T2, T3, T4> value)
    {
      value = default;
      if (reader.ValueType != JSValueType.Array) return;
      if (!reader.GetNextArrayItem()) return;
      T1 t1 = reader.ReadValue<T1>();
      if (!reader.GetNextArrayItem()) return;
      T2 t2 = reader.ReadValue<T2>();
      if (!reader.GetNextArrayItem()) return;
      T3 t3 = reader.ReadValue<T3>();
      if (!reader.GetNextArrayItem()) return;
      T4 t4 = reader.ReadValue<T4>();
      SkipArrayToEnd(reader);
      value = new Tuple<T1, T2, T3, T4>(t1, t2, t3, t4);
    }

    public static void ReadTuple5<T1, T2, T3, T4, T5>(
      IJSValueReader reader, out Tuple<T1, T2, T3, T4, T5> value)
    {
      value = default;
      if (reader.ValueType != JSValueType.Array) return;
      if (!reader.GetNextArrayItem()) return;
      T1 t1 = reader.ReadValue<T1>();
      if (!reader.GetNextArrayItem()) return;
      T2 t2 = reader.ReadValue<T2>();
      if (!reader.GetNextArrayItem()) return;
      T3 t3 = reader.ReadValue<T3>();
      if (!reader.GetNextArrayItem()) return;
      T4 t4 = reader.ReadValue<T4>();
      if (!reader.GetNextArrayItem()) return;
      T5 t5 = reader.ReadValue<T5>();
      SkipArrayToEnd(reader);
      value = new Tuple<T1, T2, T3, T4, T5>(t1, t2, t3, t4, t5);
    }

    public static void ReadTuple6<T1, T2, T3, T4, T5, T6>(
      IJSValueReader reader, out Tuple<T1, T2, T3, T4, T5, T6> value)
    {
      value = default;
      if (reader.ValueType != JSValueType.Array) return;
      if (!reader.GetNextArrayItem()) return;
      T1 t1 = reader.ReadValue<T1>();
      if (!reader.GetNextArrayItem()) return;
      T2 t2 = reader.ReadValue<T2>();
      if (!reader.GetNextArrayItem()) return;
      T3 t3 = reader.ReadValue<T3>();
      if (!reader.GetNextArrayItem()) return;
      T4 t4 = reader.ReadValue<T4>();
      if (!reader.GetNextArrayItem()) return;
      T5 t5 = reader.ReadValue<T5>();
      if (!reader.GetNextArrayItem()) return;
      T6 t6 = reader.ReadValue<T6>();
      SkipArrayToEnd(reader);
      value = new Tuple<T1, T2, T3, T4, T5, T6>(t1, t2, t3, t4, t5, t6);
    }

    public static void ReadTuple7<T1, T2, T3, T4, T5, T6, T7>(
      IJSValueReader reader, out Tuple<T1, T2, T3, T4, T5, T6, T7> value)
    {
      value = default;
      if (reader.ValueType != JSValueType.Array) return;
      if (!reader.GetNextArrayItem()) return;
      T1 t1 = reader.ReadValue<T1>();
      if (!reader.GetNextArrayItem()) return;
      T2 t2 = reader.ReadValue<T2>();
      if (!reader.GetNextArrayItem()) return;
      T3 t3 = reader.ReadValue<T3>();
      if (!reader.GetNextArrayItem()) return;
      T4 t4 = reader.ReadValue<T4>();
      if (!reader.GetNextArrayItem()) return;
      T5 t5 = reader.ReadValue<T5>();
      if (!reader.GetNextArrayItem()) return;
      T6 t6 = reader.ReadValue<T6>();
      if (!reader.GetNextArrayItem()) return;
      T7 t7 = reader.ReadValue<T7>();
      SkipArrayToEnd(reader);
      value = new Tuple<T1, T2, T3, T4, T5, T6, T7>(t1, t2, t3, t4, t5, t6, t7);
    }

    private static MethodInfo MatchMethodInfo(this Type valueType, Type genericType, string methodName)
    {
      return TryGetTypeArgs(valueType, genericType, out Type[] typeArgs)
        ? MethodOf(methodName, typeArgs)
        : null;
    }

    private static bool TryGenerateReadValueForTuple(Type valueType, out Delegate readDelegate)
    {
      MethodInfo methodInfo =
        valueType.MatchMethodInfo(typeof(Tuple<>), nameof(ReadTuple1))
        ?? valueType.MatchMethodInfo(typeof(Tuple<,>), nameof(ReadTuple2))
        ?? valueType.MatchMethodInfo(typeof(Tuple<,,>), nameof(ReadTuple3))
        ?? valueType.MatchMethodInfo(typeof(Tuple<,,,>), nameof(ReadTuple4))
        ?? valueType.MatchMethodInfo(typeof(Tuple<,,,,>), nameof(ReadTuple5))
        ?? valueType.MatchMethodInfo(typeof(Tuple<,,,,,>), nameof(ReadTuple6))
        ?? valueType.MatchMethodInfo(typeof(Tuple<,,,,,,>), nameof(ReadTuple7));
      readDelegate = methodInfo?.CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType));
      return readDelegate != null;
    }

    private static bool TryGenerateReadValueForClass(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out Type value) =>
      // {
      //   value = new Type();
      //   if (reader.ValueType == JSValueType.Object)
      //   {
      //     while (reader.GetNextObjectProperty(out string propertyName))
      //     {
      //       // For each object field or property
      //       switch(propertyName)
      //       {
      //         case "Field1": value.Field1 = reader.ReadValue<Field1Type>(); break;
      //         case "Field2": value.Field2 = reader.ReadValue<Field2Type>(); break;
      //         case "Prop1": value.Prop1 = reader.ReadValue<Prop1Type>(); break;
      //         case "Prop2": value.Prop2 = reader.ReadValue<Prop2Type>(); break;
      //       }
      //     }
      //   }
      // }

      var valueTypeInfo = valueType.GetTypeInfo();
      bool isStruct = valueTypeInfo.IsValueType && !valueTypeInfo.IsEnum;
      bool isClass = valueTypeInfo.IsClass;
      if (isStruct || (isClass && valueType.GetConstructor(Type.EmptyTypes) != null))
      {
        var fields =
          from field in valueType.GetFields(BindingFlags.Public | BindingFlags.Instance)
          where !field.IsInitOnly
          select new { field.Name, Type = field.FieldType };
        var properties =
          from property in valueType.GetProperties(BindingFlags.Public | BindingFlags.Instance)
          let propertySetter = property.SetMethod
          where propertySetter.IsPublic
          select new { property.Name, Type = property.PropertyType };
        var members = fields.Concat(properties).ToArray();

        readDelegate = ReadValueDelegateOf(valueType).CompileLambda(
          Parameter(typeof(IJSValueReader), out var reader),
          Parameter(valueType.MakeByRefType(), out var value),
          value.Assign(New(valueType)),
          IfThen(Equal(reader.Property(ValueType), Constant(JSValueType.Object)),
            ifTrue: Block(
              Variable(typeof(string), out var propertyName),
              While(reader.Call(GetNextObjectProperty, propertyName),
                (members.Length != 0)
                ? Switch(propertyName,
                    members.Select(member => SwitchCase(
                      value.AssignPropertyVoid(member.Name, reader.CallExt(ReadValueOf(member.Type))),
                      Constant(member.Name))).ToArray()) as Expression
                : Default(typeof(void))))));

        return true;
      }

      readDelegate = null;
      return false;
    }
  }
}

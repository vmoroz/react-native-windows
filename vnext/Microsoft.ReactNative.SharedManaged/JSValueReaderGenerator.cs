using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;
using static System.Linq.Expressions.Expression;

namespace Microsoft.ReactNative.Managed
{
  class JSValueReaderGenerator
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
        return Expression.PropertyOrField(AsExpression, propertyName);
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
        var lambda = Expression.Lambda(Type, block, parameters.Select(p => p.AsExpression));
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
      LabelTarget breakLabel = Expression.Label(typeof(void));

      // Execute loop while condition is true.
      return Expression.Loop(
          Expression.IfThenElse(
            condition,
            body,
            Expression.Break(breakLabel)), breakLabel);
    }

    static MethodInfo GetNextObjectProperty =>
      typeof(IJSValueReader).GetMethod(nameof(IJSValueReader.GetNextObjectProperty));

    static MethodInfo GetNextArrayItem =>
      typeof(IJSValueReader).GetMethod(nameof(IJSValueReader.GetNextArrayItem));

    static string ValueType => nameof(IJSValueReader.ValueType);

    static Type DictionaryOf(Type keyType, Type valueType) => typeof(Dictionary<,>).MakeGenericType(keyType, valueType);

    static Type ListOf(Type itemType) => typeof(List<>).MakeGenericType(itemType);

    static Type IEnumerableOf(Type itemType) => typeof(IEnumerable<>).MakeGenericType(itemType);

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

    public static bool TryGetInterfaceTypeArg(Type type, Type interfaceType, out Type typeArg)
    {
      var typeArgQuery =
        from intfType in type.GetInterfaces().Concat(new[] { type })
        let intfTypeInfo = intfType.GetTypeInfo()
        where intfTypeInfo.IsInterface
        && intfTypeInfo.IsGenericType
        && intfTypeInfo.GetGenericTypeDefinition() == interfaceType
        select intfTypeInfo.GenericTypeArguments[0];
      typeArg = typeArgQuery.FirstOrDefault();
      return typeArg != null;
    }

    public static bool TryGetInterfaceTypeArg(Type type, Type interfaceType, out Type typeArg0, out Type typeArg1)
    {
      var typeArgQuery =
        from intfType in type.GetInterfaces().Concat(new[] { type })
        let intfTypeInfo = intfType.GetTypeInfo()
        where intfTypeInfo.IsInterface
        && intfTypeInfo.IsGenericType
        && intfTypeInfo.GetGenericTypeDefinition() == interfaceType
        select intfTypeInfo;
      var typeInfo = typeArgQuery.FirstOrDefault();
      typeArg0 = typeInfo?.GenericTypeArguments[0];
      typeArg1 = typeInfo?.GenericTypeArguments[1];
      return typeInfo != null;
    }

    public static bool TryGetClassTypeArg(Type type, Type genericType, out Type typeArg)
    {
      var typeInfo = type.GetTypeInfo();
      typeArg = (typeInfo.IsGenericType && typeInfo.GetGenericTypeDefinition() == genericType) ?
        typeInfo.GenericTypeArguments[0] : null;
      return typeArg != null;
    }

    public static Delegate GenerateReadValueDelegate(Type valueType)
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
      // (IJSValueReader reader, out EnumType value) =>
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

    private static bool TryGenerateReadValueForEnum(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out EnumType value) =>
      // {
      //   value = (EnumType)reader.ReadValue<int>();
      // }

      readDelegate = valueType.GetTypeInfo().IsEnum
        ? ReadValueDelegateOf(valueType).CompileLambda(
            Parameter(typeof(IJSValueReader), out var reader),
            Parameter(valueType.MakeByRefType(), out var value),
            value.Assign(Convert(valueType, reader.CallExt(ReadValueOf(typeof(int))))))
        : null;

      return readDelegate != null;
    }

    private static bool TryGenerateReadValueForNullable(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out T? value) =>
      // {
      //   if (reader.ValueType != JSValueType.Null)
      //   {
      //     value = reader.ReadValue<T>();
      //   }
      //   else
      //   {
      //     value = null;
      //   }
      // }

      readDelegate = TryGetClassTypeArg(valueType, typeof(Nullable<>), out Type typeArg)
        ? ReadValueDelegateOf(valueType).CompileLambda(
            Parameter(typeof(IJSValueReader), out var reader),
            Parameter(valueType.MakeByRefType(), out var value),
            IfThenElse(NotEqual(reader.Property(ValueType), Constant(JSValueType.Null)),
              ifTrue: value.Assign(Convert(valueType, reader.CallExt(ReadValueOf(typeArg)))),
              ifFalse: value.Assign(New(valueType))))
        : null;

      return readDelegate != null;
    }

    private static bool TryGenerateReadValueForIDictionary(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out IDictionary<string, T> value) =>
      // {
      //   var dictionary = new Dictionary<string, T>();
      //   if (reader.ValueType == JSValueType.Object)
      //   {
      //     while (reader.GetNextObjectProperty(out string propertyName))
      //     {
      //       dictionary.Add(propertyName, reader.ReadValue<T>());
      //     }
      //   }
      //   value = dictionary;
      // }

      readDelegate =
        TryGetInterfaceTypeArg(valueType, typeof(IDictionary<,>), out Type keyType, out Type typeArg)
        && keyType == typeof(string)
        ? ReadValueDelegateOf(valueType).CompileLambda(
            Parameter(typeof(IJSValueReader), out var reader),
            Parameter(valueType.MakeByRefType(), out var value),
            Variable(DictionaryOf(keyType, typeArg), out var dictionary, New(DictionaryOf(keyType, typeArg))),
            Variable(typeof(string), out var propertyName),
            IfThen(Equal(reader.Property(ValueType), Constant(JSValueType.Array)),
              ifTrue: While(reader.Call(GetNextObjectProperty, propertyName),
                dictionary.Call("Add", propertyName, reader.CallExt(ReadValueOf(typeArg))))),
            value.Assign(dictionary))
        : null;

      return readDelegate != null;
    }

    private static bool TryGenerateReadValueForIList(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out IList<T> value) =>
      // {
      //   List<T> list = new List<T>();
      //   if (reader.ValueType == JSValueType.Array)
      //   {
      //     while (reader.GetNextArrayItem())
      //     {
      //       list.Add(reader.ReadValue<T>());
      //     }
      //   }
      //   value = list;
      // }

      readDelegate = TryGetInterfaceTypeArg(valueType, typeof(IList<>), out Type typeArg)
        ? ReadValueDelegateOf(valueType).CompileLambda(
            Parameter(typeof(IJSValueReader), out var reader),
            Parameter(valueType.MakeByRefType(), out var value),
            Variable(ListOf(typeArg), out var list, New(ListOf(typeArg))),
            IfThen(Equal(reader.Property(ValueType), Constant(JSValueType.Array)),
              ifTrue: While(reader.Call(GetNextArrayItem),
                list.Call("Add", reader.CallExt(ReadValueOf(typeArg))))),
            (valueType == ListOf(typeArg)) ? value.Assign(list)
            : valueType.IsArray ? value.Assign(list.Call("ToArray"))
            : valueType.GetTypeInfo().IsInterface ? value.Assign(Convert(valueType, list))
            : value.Assign(New(valueType.GetConstructor(new[] { IEnumerableOf(typeArg) }), list)))
        : null;

      return readDelegate != null;
    }

    private static bool TryGenerateReadValueForTuple(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks as the following:
      // (It may be up to seven template arguments: T1, T2, T3, T4, T5, T6, T7.)
      // (ValueTuple is not supported because it is in a different package.)
      //
      // (IJSValueReader reader, out Tuple<T1, T2> value) =>
      // {
      //   if (reader.ValueType == JSValueType.Array)
      //   {
      //     if (reader.GetNextArrayItem())
      //     {
      //       T1 t1 = reader.ReadValue<T1>();
      //       if (reader.GetNextArrayItem())
      //       {
      //         T2 t2 = reader.ReadValue<T2>();
      //         while (reader.GetNextArrayItem())
      //         {
      //            // Skip to the end of the array
      //            reader.ReadValue<JSValue>();
      //         }
      //         value = new Tuple<T1, T2>(t1, t2);
      //         return;
      //       }
      //     }
      //   }
      //   value = default;
      // }

      var valueTypeInfo = valueType.GetTypeInfo();
      if (valueTypeInfo.IsGenericType)
      {
        var genericValueType = valueTypeInfo.GetGenericTypeDefinition();
        if (genericValueType == typeof(Tuple<>)
          || genericValueType == typeof(Tuple<,>)
          || genericValueType == typeof(Tuple<,,>)
          || genericValueType == typeof(Tuple<,,,>)
          || genericValueType == typeof(Tuple<,,,,>)
          || genericValueType == typeof(Tuple<,,,,,>)
          || genericValueType == typeof(Tuple<,,,,,,>))
        {
          var tupleArgTypes = valueTypeInfo.GenericTypeArguments;
          var tupleArgs = new VariableWrapper[tupleArgTypes.Length];
          Parameter(typeof(IJSValueReader), out var reader);
          Parameter(valueType.MakeByRefType(), out var value);
          LabelTarget returnTarget = Label();

          Expression GenerateForNextItem(int index)
          {
            return (index < tupleArgTypes.Length)
              ? IfThen(reader.Call(GetNextArrayItem),
                  ifTrue: Block(
                    Variable(tupleArgTypes[index], out tupleArgs[index], reader.CallExt(ReadValueOf(tupleArgTypes[index]))),
                    GenerateForNextItem(index + 1))) as Expression
              : Block(
                  While(reader.Call(GetNextArrayItem),
                    reader.CallExt(ReadValueOf(typeof(JSValue)))),
                  value.Assign(New(valueType.GetConstructor(tupleArgTypes), tupleArgs.Select(a => a.AsExpression))),
                  Return(returnTarget));
          };

          readDelegate = ReadValueDelegateOf(valueType).CompileLambda(reader, value,
            IfThen(Equal(reader.Property(ValueType), Constant(JSValueType.Array)),
              ifTrue: GenerateForNextItem(0)),
            value.Assign(Default(valueType)),
            Label(returnTarget));
          return true;
        }
      }

      readDelegate = null;
      return false;
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

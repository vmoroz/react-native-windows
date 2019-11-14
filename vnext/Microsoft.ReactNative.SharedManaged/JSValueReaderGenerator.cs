using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.ServiceModel.Channels;

namespace Microsoft.ReactNative.Managed
{
  class JSValueReaderGenerator
  {
    public class VariableWraper
    {
      public static VariableWraper CreateVariable(Type type, Expression init)
      {
        return new VariableWraper
        {
          Type = type,
          AsExpression = Expression.Variable(type),
          Init = init,
          IsParameter = false
        };
      }

      public static VariableWraper CreateParameter(Type type)
      {
        return new VariableWraper
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

      public static implicit operator ParameterExpression(VariableWraper v) => v.AsExpression;

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
        var parameters = new List<VariableWraper>();
        var variables = new List<VariableWraper>();
        var body = new List<Expression>();

        foreach (var item in expressions)
        {
          switch (item)
          {
            case VariableWraper parameter when parameter.IsParameter: parameters.Add(parameter); break;
            case VariableWraper variable when !variable.IsParameter:
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

    public static VariableWraper Variable(Type type, out VariableWraper variable, Expression init = null)
    {
      return variable = VariableWraper.CreateVariable(type, init);
    }

    public static VariableWraper Parameter(Type type, out VariableWraper parameter)
    {
      return parameter = VariableWraper.CreateParameter(type);
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

    public static Expression New(Type type)
    {
      return Expression.New(type);
    }

    static MethodInfo GetNextArrayItem =>
      typeof(IJSValueReader).GetMethod(nameof(IJSValueReader.GetNextArrayItem));

    static Type ListOf(Type typeArg)
    {
      return typeof(List<>).MakeGenericType(typeArg);
    }

    static TypeWrapper ReadValueDelegateOf(Type typeArg)
    {
      return new TypeWrapper(typeof(ReadValueDelegate<>).MakeGenericType(typeArg));
    }

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

    public static bool TryGenerateReadValueForEnum(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out EnumType value) =>
      // {
      //   value = (EnumType)reader.ReadValue<EnumType>();
      // }

      readDelegate = valueType.GetTypeInfo().IsEnum ?
        ReadValueDelegateOf(valueType).CompileLambda(
          Parameter(typeof(IJSValueReader), out var reader),
          Parameter(valueType.MakeByRefType(), out var value),
          value.Assign(Convert(valueType, reader.CallExt(ReadValueOf(typeof(int)))))) : null;

      return readDelegate != null;
    }

    public static bool TryGenerateReadValueForIList(Type valueType, out Delegate readDelegate)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out IList<Type> value) =>
      // {
      //   List<T> list = new List<T>();
      //   while (reader.GetNextArrayItem())
      //   {
      //     list.Add(reader.ReadValue<T>());
      //   }
      //   value = list;
      // }

      readDelegate = TryGetInterfaceTypeArg(valueType, typeof(IList<>), out Type typeArg) ?
        ReadValueDelegateOf(valueType).CompileLambda(
          Parameter(typeof(IJSValueReader), out var reader),
          Parameter(valueType.MakeByRefType(), out var value),
          Variable(ListOf(typeArg), out var list, New(ListOf(typeArg))),
          While(reader.Call(GetNextArrayItem),
            list.Call("Add", reader.CallExt(ReadValueOf(typeArg)))),
          value.Assign(list)) : null;

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
      //         while (!reader.GetNextArrayItem())
      //         {
      //            reader.ReadValue<JSValue>();
      //         }
      //         value = new Tuple<T1, T2>(t1, t2);
      //         return;
      //       }
      //     }
      //   }
      //   value = default;
      // }
      //var valueTypeInfo = valueType.GetTypeInfo();
      //var genericValueType = valueTypeInfo.GetGenericTypeDefinition();
      //var genericValueTypeArgs = valueTypeInfo.GenericTypeArguments;
      //int typeArgCount = 0;
      //if (genericValueType == typeof(Tuple<>))
      //{
      //  typeArgCount = 1;
      //}
      //else if (genericValueType == typeof(Tuple<,>))
      //{
      //  typeArgCount = 2;
      //}
      //else if (genericValueType == typeof(Tuple<,,>))
      //{
      //  typeArgCount = 3;
      //}
      //else if (genericValueType == typeof(Tuple<,,,>))
      //{
      //  typeArgCount = 4;
      //}
      //else if (genericValueType == typeof(Tuple<,,,,>))
      //{
      //  typeArgCount = 5;
      //}
      //else if (genericValueType == typeof(Tuple<,,,,,>))
      //{
      //  typeArgCount = 6;
      //}
      //else if (genericValueType == typeof(Tuple<,,,,,,>))
      //{
      //  typeArgCount = 7;
      //}

      //if (typeArgCount > 0)
      //{
      //  // Lambda parameters (IJSValueReader reader, out Type value)
      //  var readerParam = Expression.Parameter(typeof(IJSValueReader), "reader");
      //  var valueParam = Expression.Parameter(valueType.MakeByRefType(), "value");

      //  // We start with the innermost block and build up on top of it.

      //  // T2 t2 = reader.ReadValue<T2>();
      //  var typeArg = genericValueTypeArgs[typeArgCount - 1];

      //  var readValueGenericMethod = (
      //    from method in typeof(JSValueReader).GetMethods(BindingFlags.Static | BindingFlags.Public)
      //    where method.IsGenericMethod && method.IsDefined(typeof(ExtensionAttribute), inherit: false) && method.Name == nameof(ReadValue)
      //    let parameters = method.GetParameters()
      //    where parameters.Length == 1
      //    && parameters[0].ParameterType == typeof(IJSValueReader)
      //    select method
      //  ).FirstOrDefault();
      //  if (readValueGenericMethod != null)
      //  {
      //    var readValueMethod = readValueGenericMethod.MakeGenericMethod(typeArg);
      //    var tVar = Expression.Variable(typeArg);
      //    Expression.Assign(tVar, Expression.Call(readValueMethod, readerParam));


      //    var readValueOfJSValue = readValueGenericMethod.MakeGenericMethod(typeof(JSValue));
      //    var whileBody = Expression.Call(readValueOfJSValue, readerParam);

      //    // Creating a label to jump to from a loop.
      //    LabelTarget label = Expression.Label(typeof(void));

      //    var whileLoop =
      //      Expression.Loop(
      //        Expression.IfThenElse(
      //          Expression.Call(readerParam, typeof(IJSValueReader).GetMethod(nameof(IJSValueReader.GetNextArrayItem))),
      //          whileBody,
      //          Expression.Break(label)), label);

      //    // value = new Tuple<T1, T2>(t1, t2);
      //    Expression.New(type)

      //  }

      //}

      //     if (reader.GetNextArrayItem())
      //     {
      //       T1 t1 = reader.ReadValue<T1>();

      //IfThen(reader.Call(GetNextArrayItem),
      //  Block(b => b.Add(
      //    b.Var(T1, "t1").Assign(reader.CallExt(ReadValueOf(T1))),

      //));


      //Block(b => b.Add(
      //  b.AddVar(T2, "t2").Assign(reader.CallExt(ReadValueOf(T2)))),
      //  While(reader.Call(GetNextArrayItem),
      //    reader.CallExt(ReadValueOf(typeof(JSValue)))),
      //  value.Assign(New(TupleOf(typeArgs), tupleArgs)),
      //  Return())
      //  );
      //         T2 t2 = reader.ReadValue<T2>();
      //         while (reader.GetNextArrayItem())
      //         {
      //            reader.ReadValue<JSValue>();
      //         }
      //         value = new Tuple<T1, T2>(t1, t2);
      //         return;

      readDelegate = null;
      return false;
    }

  }
}

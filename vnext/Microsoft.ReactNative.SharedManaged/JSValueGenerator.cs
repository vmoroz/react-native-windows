// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using static System.Linq.Expressions.Expression;

namespace Microsoft.ReactNative.Managed
{
  static class JSValueGenerator
  {
    // Compare two types by putting more specific types before more generic.
    // While we use it to compare types with the same generic type base,
    // we do more thorough comparison because the same method is called
    // recursively for the generic type arguments.
    public class GenericTypeComparer : IComparer<Type>
    {
      public static readonly GenericTypeComparer Default = new GenericTypeComparer();

      public int Compare(Type x, Type y)
      {
        var xTypeInfo = x.GetTypeInfo();
        var yTypeInfo = y.GetTypeInfo();

        // Generic parameters are less specific and must appear after other types. E.g. string before T.
        int result = Comparer<bool>.Default.Compare(x.IsGenericParameter, y.IsGenericParameter);
        if (result != 0) return result;

        // Compare generic parameters. E.g. T vs U. We use default type order.
        if (x.IsGenericParameter) return Comparer<Type>.Default.Compare(x, y);

        // We consider arrays to be more specific than non-arrays.
        // Note the minus '-' sign to reverse order.
        result = -Comparer<bool>.Default.Compare(x.IsArray, y.IsArray);
        if (result != 0) return result;

        // Compare arrays by their element types.
        if (x.IsArray) return Compare(x.GetElementType(), y.GetElementType());

        // Generic types are more specific and must appear before non-generic types.
        // E.g. IDictionary<T, U> before IDictionary. Note the minus '-' sign to reverse order.
        result = -Comparer<bool>.Default.Compare(xTypeInfo.IsGenericType, yTypeInfo.IsGenericType);
        if (result != 0) return result;

        // Compare non-generic types. E.g string vs int. We use default type order.
        if (!xTypeInfo.IsGenericType) return Comparer<Type>.Default.Compare(x, y);

        // We consider types with more generic parameters to be more specific than types with less generic parameters.
        // E.g. we want to match IDictionary<string, T> before IList<KeyValuePair<string, T>>.
        var xArgs = x.GetGenericArguments();
        var yArgs = y.GetGenericArguments();
        // Note minus sign '-' to order integers in reverse order. E.g. 7 before 5.
        result = -Comparer<int>.Default.Compare(xArgs.Length, yArgs.Length);
        if (result != 0) return result;

        // If number of generic arguments is the same, then we use the order generic type definitions.
        // E.g. List<> vs IList<>.
        result = Comparer<Type>.Default.Compare(xTypeInfo.GetGenericTypeDefinition(), yTypeInfo.GetGenericTypeDefinition());
        if (result != 0) return result;

        // We have the same generic type definitions. Recursively compare their generic arguments.
        for (int i = 0; i < xArgs.Length; ++i)
        {
          result = Compare(xArgs[i], yArgs[i]);
          if (result != 0) return result;
        }

        return 0;
      }
    }

    // Try to match type to pattern with patternArgs.
    // If successful return matchedArgs where each generic parameter T from patternArgs has a real type.
    public static bool TryMatchGenericType(Type type, Type pattern, Type[] patternArgs, out Type[] matchedArgs)
    {
      matchedArgs = null;
      var genericBindings = new Dictionary<Type, Type>(patternArgs.Length);

      // This local function is going to be called recursively for generic type arguments.
      bool MatchType(Type testType, Type patternType)
      {
        if (testType == patternType) return true;

        // Match array types
        if (testType.IsArray != patternType.IsArray) return false;
        if (testType.IsArray && patternType.IsArray)
        {
          return MatchType(testType.GetElementType(), patternType.GetElementType());
        }

        // Match testType to generic parameter type such as T.
        if (patternType.IsGenericParameter)
        {
          if (genericBindings.TryGetValue(patternType, out var existingBinding))
          {
            return testType == existingBinding;
          }
          else
          {
            genericBindings.Add(patternType, testType);
            return true;
          }
        }

        // Match generic types
        var testTypeInfo = testType.GetTypeInfo();
        var patternTypeInfo = patternType.GetTypeInfo();
        if (testTypeInfo.IsGenericType && patternTypeInfo.IsGenericType)
        {
          Type[] testGenericArgs = testType.GetGenericArguments();
          Type[] patternGenericArgs = pattern.GetGenericArguments();
          if (testGenericArgs.Length == patternGenericArgs.Length)
          {
            for (int i = 0; i < testGenericArgs.Length; ++i)
            {
              if (!MatchType(testGenericArgs[i], patternGenericArgs[i]))
              {
                return false;
              }
            }

            return true;
          }
        }

        return false;
      }

      if (!MatchType(type, pattern)) return false;

      if (patternArgs.Length != genericBindings.Count) return false;

      // Check generic constraints
      foreach (var genericArg in patternArgs)
      {
        // base class and interface constraints
        var baseTypeConstraints = genericArg.GetTypeInfo().GetGenericParameterConstraints();
        if (baseTypeConstraints.Length > 0)
        {
          var boundType = genericBindings[genericArg];
          foreach (var baseType in baseTypeConstraints)
          {
            // TODO: what if baseType is based on a generic parameter? E.g. 'where T : U'
            if (!boundType.GetTypeInfo().IsSubclassOf(baseType))
            {
              return false;
            }
          }
        }

        // TODO: Consider to add checks for generic parameter attributes: t.GenericParameterAttributes
      }

      matchedArgs = new Type[patternArgs.Length];
      for (int i = 0; i < matchedArgs.Length; ++i)
      {
        matchedArgs[i] = genericBindings[patternArgs[i]];
      }

      return true;
    }

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

      public Expression CallExt(MethodInfo method, Expression arg0, Expression arg1)
      {
        return Expression.Call(method, AsExpression, arg0, arg1);
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

    public static BlockExpression AutoBlock(params object[] expressions)
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

      return Block(variables.Select(v => v.AsExpression), body);
    }

    public static Expression While(Expression condition, Expression body)
    {
      // A label to jump to from a loop.
      LabelTarget breakLabel = Label(typeof(void));

      // Execute loop while condition is true.
      return Loop(IfThenElse(condition, body, Break(breakLabel)), breakLabel);
    }
  }
}

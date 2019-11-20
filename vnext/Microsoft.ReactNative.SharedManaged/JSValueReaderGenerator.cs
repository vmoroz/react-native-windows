using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Threading;
using Windows.UI.Composition;
using static System.Linq.Expressions.Expression;

namespace Microsoft.ReactNative.Managed
{
  static class JSValueReaderGenerator
  {
    private static readonly Lazy<KeyValuePair<Type, MethodInfo>[]> s_allMethods;
    private static readonly Lazy<IReadOnlyDictionary<Type, MethodInfo>> s_nonGenericMethods;
    private static readonly Lazy<IReadOnlyDictionary<Type, SortedList<Type, MethodInfo>>> s_genericMethods;

    // Compare two types by putting more specific types before more generic.
    // While we use it to compare types with the same generic type base,
    // we do more thorough comparison because the same method is called
    // recursively for the generic type arguments.
    class GenericTypeComparer : IComparer<Type>
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

    static JSValueReaderGenerator()
    {
      // Get all extension ReadValue methods for IJSValueReader and JSValue.
      // The first parameter must be IJSValueReader or JSValue.
      // The second parameter must be an 'out' parameter and must not be a generic parameter T.
      // This is to avoid endless recursion because ReadValue with generic parameter T
      // is the one who calls the JSValueReaderGenerator.
      s_allMethods = new Lazy<KeyValuePair<Type, MethodInfo>[]>(() =>
      {
        var extensionMethods = 
          from type in typeof(JSValueReader).GetTypeInfo().Assembly.GetTypes()
          let typeInfo = type.GetTypeInfo()
          where typeInfo.IsSealed && !typeInfo.IsGenericType && !typeInfo.IsNested
          from member in type.GetMember(nameof(JSValueReader.ReadValue), BindingFlags.Static | BindingFlags.Public | BindingFlags.NonPublic)
          let method = member as MethodInfo
          where (method != null) && method.IsDefined(typeof(ExtensionAttribute), inherit: false)
          let parameters = method.GetParameters()
          where parameters.Length == 2
            && (parameters[0].ParameterType == typeof(IJSValueReader)
             || parameters[0].ParameterType == typeof(JSValue))
            && parameters[1].IsOut
          let dataType = parameters[1].ParameterType.GetElementType()
          where !dataType.IsGenericParameter
          select new KeyValuePair<Type, MethodInfo>(dataType, method);
        return extensionMethods.ToArray();
      });

      // Get all non-generic ReadValue extension methods.
      // They are easy to match and we can put them in a dictionary.
      s_nonGenericMethods = new Lazy<IReadOnlyDictionary<Type, MethodInfo>>(() =>
        s_allMethods.Value.Where(p => !p.Value.IsGenericMethod).ToDictionary(p => p.Key, p => p.Value));

      // Get all generic ReadValue extension methods.
      // Group them by generic type definitions and sort them by having more specific methods getting first.
      // The matching for generic types is more complicated: we first match the generic type definition,
      // and then walk the sorted list and try to match types one by one.
      // Note that the parameter could be an array of generic type. The array is not a generic type. 
      s_genericMethods = new Lazy<IReadOnlyDictionary<Type, SortedList<Type, MethodInfo>>>(() =>
      {
        var genericMethods =
          from pair in s_allMethods.Value
          where pair.Value.IsGenericMethod
          let type = pair.Key
          let keyType = type.GetTypeInfo().IsGenericType ? type.GetGenericTypeDefinition()
            : type.IsArray ? typeof(Array)
            : throw new InvalidOperationException($"Unsupported argument type {type}")
          group pair by keyType into g
          select new KeyValuePair<Type, SortedList<Type, MethodInfo>>(
            g.Key, new SortedList<Type, MethodInfo>(
              g.ToDictionary(p => p.Key, p => p.Value), GenericTypeComparer.Default));
        return genericMethods.ToDictionary(p => p.Key, p => p.Value);
      });
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

    // Try to match type to pattern with patternArgs.
    // If successful return matchedArgs where each generic parameter T from patternArgs has a real type.
    private static bool TryMatchGenericType(Type type, Type pattern, Type[] patternArgs, out Type[] matchedArgs)
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

    public static Delegate GenerateReadValueDelegate(Type valueType)
    {
      return GenerateFromExtension(valueType)
        ?? GenerateFromGenericExtension(valueType)
        ?? GenerateReadValueForEnum(valueType)
        ?? GenerateReadValueForClass(valueType)
        ?? throw new Exception($"Cannot generate ReadValue delegate for type {valueType}"); 
    }

    // Creates a delegate from the ReadValue non-generic extension method
    private static Delegate GenerateFromExtension(Type valueType)
    {
      if (s_nonGenericMethods.Value.TryGetValue(valueType, out MethodInfo methodInfo))
      {
        var extendedType = methodInfo.GetParameters()[0].ParameterType;
        if (extendedType == typeof(IJSValueReader))
        {
          return methodInfo.CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType));
        }
        else if (extendedType == typeof(JSValue))
        {
          return GenerateFromJSValueExtension(valueType, methodInfo);
        }
      }

      return null;
    }

    private static Delegate GenerateFromGenericExtension(Type valueType)
    {
      var keyType = valueType.GetTypeInfo().IsGenericType ? valueType.GetGenericTypeDefinition()
        : valueType.IsArray ? typeof(Array)
        : null;

      if (keyType != null
        && s_genericMethods.Value.TryGetValue(keyType, out SortedList<Type, MethodInfo> candidateMethods))
      {
        foreach (var candidateMethod in candidateMethods)
        {
          var genericType = candidateMethod.Key;
          var methodInfo = candidateMethod.Value;
          var genericArgs = methodInfo.GetGenericArguments();
          if (TryMatchGenericType(valueType, genericType, genericArgs, out Type[] typeArgs))
          {
            var genericMethod = methodInfo.MakeGenericMethod(typeArgs);
            var extendedType = methodInfo.GetParameters()[0].ParameterType;
            if (extendedType == typeof(IJSValueReader))
            {
              return genericMethod.CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType));
            }
            else if (extendedType == typeof(JSValue))
            {
              return GenerateFromJSValueExtension(valueType, genericMethod);
            }
          }
        }
      }

      return null;
    }

    // Creates a delegate from the JSValue ReadValue non-generic extension method
    private static Delegate GenerateFromJSValueExtension(Type valueType, MethodInfo methodInfo)
    {
      // Generate code that looks like:
      //
      // (IJSValueReader reader, out Type value) =>
      // {
      //   var jsValue = JSValue.ReadFrom(reader);
      //   jsValue.ReadValue(out value);
      // }

      return ReadValueDelegateOf(valueType).CompileLambda(
        Parameter(typeof(IJSValueReader), out var reader),
        Parameter(valueType.MakeByRefType(), out var value),
        Variable(typeof(JSValue), out var jsValue, Call(JSValueReadFrom, reader)),
        jsValue.CallExt(methodInfo, value));
    }

    // It cannot be an extension method because it would conflict with the generic
    // extension method that uses T value type.
    public static void ReadEnum<TEnum>(IJSValueReader reader, out TEnum value) where TEnum : Enum
    {
      value = (TEnum)(object)reader.ReadValue<int>();
    }

    private static Delegate GenerateReadValueForEnum(Type valueType)
    {
      // Creates a delegate from the ReadEnum method
      return valueType.GetTypeInfo().IsEnum
        ? MethodOf(nameof(ReadEnum), valueType).CreateDelegate(typeof(ReadValueDelegate<>).MakeGenericType(valueType))
        : null;
    }

    private static Delegate GenerateReadValueForClass(Type valueType)
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

        return ReadValueDelegateOf(valueType).CompileLambda(
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
      }

      return null;
    }
  }
}

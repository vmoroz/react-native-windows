// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.ReactNative.Bridge;
using System;
using System.Reflection;
using System.Threading;
using static Microsoft.ReactNative.Managed.JSValueGenerator;
using static Microsoft.ReactNative.Managed.JSValueReaderGenerator;
using static Microsoft.ReactNative.Managed.JSValueWriterGenerator;
using static System.Linq.Expressions.Expression;

namespace Microsoft.ReactNative.Managed
{
  class ReactMethodInfo
  {
    public ReactMethodInfo(MethodInfo methodInfo, ReactMethodAttribute methodAttribute)
    {
      MethodName = methodAttribute.MethodName ?? methodInfo.Name;

      Type returnType = methodInfo.ReturnType;
      ParameterInfo[] parameters = methodInfo.GetParameters();
      if (returnType != typeof(void))
      {
        MethodReturnType = MethodReturnType.Callback;
        MethodImpl = new Lazy<ReactMethodImpl>(() => MakeReturningMethod(methodInfo, returnType, parameters), LazyThreadSafetyMode.PublicationOnly);
      }
      else
      {
        if (parameters.Length > 0 && typeof(Delegate).IsAssignableFrom(parameters[parameters.Length - 1].ParameterType))
        {
          if (parameters.Length > 1 && typeof(Delegate).IsAssignableFrom(parameters[parameters.Length - 2].ParameterType))
          {
            MethodReturnType = MethodReturnType.Promise;
            MethodImpl = new Lazy<ReactMethodImpl>(() => MakePromiseMethod(methodInfo, parameters), LazyThreadSafetyMode.PublicationOnly);
          }
          else
          {
            MethodReturnType = MethodReturnType.Callback;
            MethodImpl = new Lazy<ReactMethodImpl>(() => MakeCallbackMethod(methodInfo, parameters), LazyThreadSafetyMode.PublicationOnly);
          }
        }
        else
        {
          MethodReturnType = MethodReturnType.Void;
          MethodImpl = new Lazy<ReactMethodImpl>(() => MakeFireAndForgetMethod(methodInfo, parameters), LazyThreadSafetyMode.PublicationOnly);
        }
      }
    }

    static VariableWrapper[] MethodParameters(
      out VariableWrapper module,
      out VariableWrapper inputReader,
      out VariableWrapper outputWriter,
      out VariableWrapper resolve,
      out VariableWrapper reject)
    {
      var result = new VariableWrapper[5];
      result[0] = Parameter(typeof(object), out module);
      result[1] = Parameter(typeof(IJSValueReader), out inputReader);
      result[2] = Parameter(typeof(IJSValueWriter), out outputWriter);
      result[3] = Parameter(typeof(MethodResultCallback), out resolve);
      result[4] = Parameter(typeof(MethodResultCallback), out reject);
      return result;
    }

    private ReactMethodImpl MakeFireAndForgetMethod(MethodInfo methodInfo, ParameterInfo[] parameters)
    {
      // Generate code that looks like:
      //
      // (object module, IJSValueReader inputReader, IJSValueWriter outputWriter,
      //    MethodResultCallback resolve, MethodResultCallback reject) =>
      // {
      //   inputReader.ReadArgs(out ArgType0 arg0, out ArgType1 arg1);
      //   (module as ModuleType).Method(arg0, arg1);
      // }

      return CompileLambda<ReactMethodImpl>(
        MethodParameters(out var module, out var inputReader, out _, out _, out _),
        MethodArgs(parameters, callbackCount: 0, out var argTypes, out var args, out _, out _),
        inputReader.CallExt(ReadArgsOf(argTypes), args),
        module.CastTo(methodInfo.DeclaringType).Call(methodInfo, args));
    }

    private ReactMethodImpl MakeCallbackMethod(MethodInfo methodInfo, ParameterInfo[] parameters)
    {
      // Generate code that looks like:
      //
      // (object module, IJSValueReader inputReader, IJSValueWriter outputWriter,
      //    MethodResultCallback resolve, MethodResultCallback reject) =>
      // {
      //   inputReader.ReadArgs(out ArgType0 arg0, out ArgType1 arg1);
      //   (module as ModuleType).Method(arg0, arg1,
      //     result => resolve(outputWriter.WriteArgs(result)));
      // }

      // The last parameter in parameters is a 'resolve' delegate
      return CompileLambda<ReactMethodImpl>(
        MethodParameters(out var module, out var inputReader, out var outputWriter, out var resolve, out _),
        MethodArgs(parameters, callbackCount: 1, out var argTypes, out var args, out var resolveArgType, out _),
        inputReader.CallExt(ReadArgsOf(argTypes), args),
        module.CastTo(methodInfo.DeclaringType).Call(methodInfo, args,
          AutoLambda(ResultCallbackOf(resolveArgType), Parameter(resolveArgType, out var result),
            resolve.Invoke(outputWriter.CallExt(WriteArgsOf(resolveArgType), result)))));
    }

    private ReactMethodImpl MakePromiseMethod(MethodInfo methodInfo, ParameterInfo[] parameters)
    {
      // Generate code that looks like:
      //
      // (object module, IJSValueReader inputReader, IJSValueWriter outputWriter,
      //    MethodResultCallback resolve, MethodResultCallback reject) =>
      // {
      //   inputReader.ReadArgs(out ArgType0 arg0, out ArgType1 arg1);
      //     (module as ModuleType).Method(arg0, arg1,
      //       result => resolve(outputWriter.WriteArgs(result)),
      //       error => reject(outputWriter.WriteError(error)));
      // }

      // The last two parameters in parameters are resolve and reject delegates
      return CompileLambda<ReactMethodImpl>(
        MethodParameters(out var module, out var inputReader, out var outputWriter, out var resolve, out var reject),
        MethodArgs(parameters, callbackCount: 2, out var argTypes, out var args, out var resolveArgType, out var rejectArgType),
        inputReader.CallExt(ReadArgsOf(argTypes), args),
        module.CastTo(methodInfo.DeclaringType).Call(methodInfo, args,
          AutoLambda(ResultCallbackOf(resolveArgType), Parameter(resolveArgType, out var result),
            resolve.Invoke(outputWriter.CallExt(WriteArgsOf(resolveArgType), result))),
          AutoLambda(ResultCallbackOf(resolveArgType), Parameter(rejectArgType, out var error),
            reject.Invoke(outputWriter.CallExt(WriteErrorOf(rejectArgType), error)))));
    }

    private ReactMethodImpl MakeReturningMethod(MethodInfo methodInfo, Type returnType, ParameterInfo[] parameters)
    {
      // Generate code that looks like:
      //
      // (object module, IJSValueReader inputReader, IJSValueWriter outputWriter,
      //    MethodResultCallback resolve, MethodResultCallback reject) =>
      // {
      //   inputReader.ReadArgs(out ArgType0 arg0, out ArgType1 arg1);
      //   resolve(outputWriter.WriteArgs((module as ModuleType).Method(arg0, arg1)));
      // }

      // It is like the MakeCallbackMethod but callback is not passed as a parameter.
      return CompileLambda<ReactMethodImpl>(
        MethodParameters(out var module, out var inputReader, out var outputWriter, out var resolve, out _),
        MethodArgs(parameters, callbackCount: 1, out var argTypes, out var args, out var resolveArgType, out _),
        inputReader.CallExt(ReadArgsOf(argTypes), args),
        resolve.Invoke(outputWriter.CallExt(WriteArgsOf(resolveArgType),
          module.CastTo(methodInfo.DeclaringType).Call(methodInfo, args))));
    }

    public delegate void ReactMethodImpl(
        object module,
        IJSValueReader inputReader,
        IJSValueWriter outputWriter,
        MethodResultCallback resolve,
        MethodResultCallback reject);

    private delegate void ResultCallback<T>(T result);

    private Type ResultCallbackOf(Type type) => typeof(ResultCallback<>).MakeGenericType(type);

    public string MethodName { get; }

    public MethodReturnType MethodReturnType { get;  }

    public Lazy<ReactMethodImpl> MethodImpl { get; }

    public void AddToModuleBuilder(IReactModuleBuilder moduleBuilder, object module)
    {
      moduleBuilder.AddMethod(MethodName, MethodReturnType, (
          IJSValueReader inputReader,
          IJSValueWriter outputWriter,
          MethodResultCallback resolve,
          MethodResultCallback reject) =>
      {
        MethodImpl.Value(module, inputReader, outputWriter, resolve, reject);
      });
    }
  }
}

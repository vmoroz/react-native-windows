// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.ReactNative.Bridge;
using System;
using System.Linq.Expressions;
using System.Reflection;
using System.Threading;
using static Microsoft.ReactNative.Managed.JSValueGenerator;
using static Microsoft.ReactNative.Managed.JSValueWriterGenerator;
using static System.Linq.Expressions.Expression;

namespace Microsoft.ReactNative.Managed
{
  class ReactEventInfo
  {
    public ReactEventInfo(PropertyInfo propertyInfo, ReactEventAttribute eventAttribute)
    {
      EventName = eventAttribute.EventName ?? propertyInfo.Name;

      Type propertyType = propertyInfo.PropertyType;
      if (!typeof(Delegate).IsAssignableFrom(propertyType))
      {
        throw new ArgumentException("React event must be a delegate", propertyInfo.Name);
      }
      MethodInfo eventDelegateMethod = propertyType.GetMethod("Invoke");
      ParameterInfo[] parameters = eventDelegateMethod.GetParameters();
      if (parameters.Length != 1)
      {
        throw new ArgumentException($"React event delegate must have one parameter. Module: {propertyInfo.DeclaringType.FullName}, Event: {propertyInfo.Name}");
      }
      EventImpl = new Lazy<ReactEventImpl>(() => MakeEvent(propertyInfo, parameters[0]), LazyThreadSafetyMode.PublicationOnly);
    }

    private ReactEventImpl MakeEvent(PropertyInfo propertyInfo, ParameterInfo parameter)
    {
      // Generate code that looks like:
      //
      // (object module, ReactEventHandler raiseEvent) =>
      // {
      //   (module as MyModule).eventProperty = (ArgType arg) =>
      //     raiseEvent((IJSValueWriter argWriter) => argWriter.WriteArgs(arg));
      // });

      Type ArgType = parameter.ParameterType;
      return CompileLambda<ReactEventImpl>(
        Parameter(typeof(object), out var module),
        Parameter(typeof(ReactEventHandler), out var raiseEvent),
        module.SetProperty(propertyInfo, AutoLambda(ActionOf(ArgType),
          Parameter(ArgType, out var arg),
          raiseEvent.Invoke(AutoLambda(ActionOf<IJSValueWriter>(),
            Parameter(typeof(IJSValueWriter), out var argWriter),
            argWriter.CallExt(WriteArgsOf(ArgType), argWriter))))));
    }

    public delegate void ReactEventImpl(object module, ReactEventHandler eventHandler);

    public string EventName { get; private set; }

    public Lazy<ReactEventImpl> EventImpl { get; private set; }

    public void AddToModuleBuilder(IReactModuleBuilder moduleBuilder, object module)
    {
      moduleBuilder.AddEventHandlerSetter(EventName, (ReactEventHandler eventHandler) =>
      {
        EventImpl.Value(module, eventHandler);
      });
    }
  }
}

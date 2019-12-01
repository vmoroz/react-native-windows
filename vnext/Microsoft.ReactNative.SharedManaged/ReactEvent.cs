// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

using Microsoft.ReactNative.Bridge;

namespace Microsoft.ReactNative.Managed
{
  delegate void ReactEvent<T>(T value);

  static class ReactEvent
  {
    public static ReactArgWriter ArgWriter<T>(T arg) => (IJSValueWriter writer) => writer.WriteValue(arg);
  }
}

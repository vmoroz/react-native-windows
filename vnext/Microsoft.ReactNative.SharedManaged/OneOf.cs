// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

namespace Microsoft.ReactNative.Managed
{
  struct OneOf<T1>
  {
    public OneOf(T1 value)
    {
      Value = value;
    }

    public object Value { get; }

    public bool TryGet<T>(out T value)
    {
      bool result = (Value != null && typeof(T) == Value.GetType());
      value = result ? (T)Value : default;
      return result;
    }
  }

  struct OneOf<T1, T2>
  {
    public OneOf(T1 value)
    {
      Value = value;
    }

    public OneOf(T2 value)
    {
      Value = value;
    }

    public static implicit operator OneOf<T1, T2>(T1 value) => new OneOf<T1, T2>(value);
    public static implicit operator OneOf<T1, T2>(T2 value) => new OneOf<T1, T2>(value);

    public object Value { get; }

    public bool TryGet<T>(out T value)
    {
      bool result = (Value != null && typeof(T) == Value.GetType());
      value = result ? (T)Value : default;
      return result;
    }
  }

  struct OneOf<T1, T2, T3>
  {
    public OneOf(T1 value)
    {
      Value = value;
    }

    public OneOf(T2 value)
    {
      Value = value;
    }

    public OneOf(T3 value)
    {
      Value = value;
    }

    public object Value { get; }

    public bool TryGet<T>(out T value)
    {
      bool result = (Value != null && typeof(T) == Value.GetType());
      value = result ? (T)Value : default;
      return result;
    }
  }

  struct OneOf<T1, T2, T3, T4>
  {
    public OneOf(T1 value)
    {
      Value = value;
    }

    public OneOf(T2 value)
    {
      Value = value;
    }

    public OneOf(T3 value)
    {
      Value = value;
    }

    public OneOf(T4 value)
    {
      Value = value;
    }

    public object Value { get; }

    public bool TryGet<T>(out T value)
    {
      bool result = (Value != null && typeof(T) == Value.GetType());
      value = result ? (T)Value : default;
      return result;
    }
  }

  struct OneOf<T1, T2, T3, T4, T5>
  {
    public OneOf(T1 value)
    {
      Value = value;
    }

    public OneOf(T2 value)
    {
      Value = value;
    }

    public OneOf(T3 value)
    {
      Value = value;
    }

    public OneOf(T4 value)
    {
      Value = value;
    }

    public OneOf(T5 value)
    {
      Value = value;
    }

    public object Value { get; }

    public bool TryGet<T>(out T value)
    {
      bool result = (Value != null && typeof(T) == Value.GetType());
      value = result ? (T)Value : default;
      return result;
    }
  }

  struct OneOf<T1, T2, T3, T4, T5, T6>
  {
    public OneOf(T1 value)
    {
      Value = value;
    }

    public OneOf(T2 value)
    {
      Value = value;
    }

    public OneOf(T3 value)
    {
      Value = value;
    }

    public OneOf(T4 value)
    {
      Value = value;
    }

    public OneOf(T5 value)
    {
      Value = value;
    }

    public OneOf(T6 value)
    {
      Value = value;
    }

    public object Value { get; }

    public bool TryGet<T>(out T value)
    {
      bool result = (Value != null && typeof(T) == Value.GetType());
      value = result ? (T)Value : default;
      return result;
    }
  }

  struct OneOf<T1, T2, T3, T4, T5, T6, T7>
  {
    public OneOf(T1 value)
    {
      Value = value;
    }

    public OneOf(T2 value)
    {
      Value = value;
    }

    public OneOf(T3 value)
    {
      Value = value;
    }

    public OneOf(T4 value)
    {
      Value = value;
    }

    public OneOf(T5 value)
    {
      Value = value;
    }

    public OneOf(T6 value)
    {
      Value = value;
    }

    public OneOf(T7 value)
    {
      Value = value;
    }

    public object Value { get; }

    public bool TryGet<T>(out T value)
    {
      bool result = (Value != null && typeof(T) == Value.GetType());
      value = result ? (T)Value : default;
      return result;
    }
  }
}

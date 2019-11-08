using Microsoft.ReactNative.Bridge;
using System;
using System.Collections.Generic;
using System.Text;

namespace Microsoft.ReactNative.Managed
{
  class JSValueTreeReader : IJSValueReader, IJSValueTreeReader
  {
    // Special initial internal state that we never return.
    private static readonly JSValueReaderState StartState = (JSValueReaderState)(-1);

    private JSValue m_current;
    private JSValueReaderState m_state = StartState;
    private readonly Stack<StackEntry> m_stack = new Stack<StackEntry>();

    public JSValueTreeReader(JSValue root)
    {
      Root = root;
    }

    public JSValueReaderState ReadNext()
    {
      if (m_state == StartState)
      {
        return ReadValue(Root);
      }

      switch (m_state)
      {
        case JSValueReaderState.ObjectBegin: return ReadObject();
        case JSValueReaderState.ArrayBegin: return ReadArray();
        case JSValueReaderState.NullValue:
        case JSValueReaderState.ObjectEnd:
        case JSValueReaderState.ArrayEnd:
        case JSValueReaderState.StringValue:
        case JSValueReaderState.BooleanValue:
        case JSValueReaderState.Int64Value:
        case JSValueReaderState.DoubleValue: return ReadNextValue();
        default: return m_state = JSValueReaderState.Error;
      }
    }

    private JSValueReaderState ReadValue(JSValue value)
    {
      if (value == null)
      {
        return m_state = JSValueReaderState.Error;
      }

      m_current = value;
      switch (value.Type)
      {
        case JSValueType.Null:
          return m_state = JSValueReaderState.NullValue;
        case JSValueType.Object:
          return m_state = JSValueReaderState.ObjectBegin;
        case JSValueType.Array:
          return m_state = JSValueReaderState.ArrayBegin;
        case JSValueType.String:
          return m_state = JSValueReaderState.StringValue;
        case JSValueType.Boolean:
          return m_state = JSValueReaderState.BooleanValue;
        case JSValueType.Int64:
          return m_state = JSValueReaderState.Int64Value;
        case JSValueType.Double:
          return m_state = JSValueReaderState.DoubleValue;
        default:
          return m_state = JSValueReaderState.Error;
      }
    }

    private JSValueReaderState ReadObject()
    {
      var properties = m_current.Object.GetEnumerator();
      if (properties.MoveNext())
      {
        m_stack.Push(StackEntry.ObjectProperty(m_current, properties));
        return ReadValue(properties.Current.Value);
      }
      else
      {
        return m_state = JSValueReaderState.ObjectEnd;
      }
    }

    private JSValueReaderState ReadArray()
    {
      var items = m_current.Array.GetEnumerator();
      if (items.MoveNext())
      {
        m_stack.Push(StackEntry.ArrayItem(m_current, items));
        return ReadValue(items.Current);
      }
      else
      {
        return m_state = JSValueReaderState.ObjectEnd;
      }
    }

    private JSValueReaderState ReadNextValue()
    {
      if (m_stack.Count == 0)
      {
        return m_state = JSValueReaderState.Error;
      }

      switch (m_stack.Peek().Value.Type)
      {
        case JSValueType.Object:
          return ReadNextObjectProperty();
        case JSValueType.Array:
          return ReadNextArrayItem();
        default:
          return m_state = JSValueReaderState.Error;
      }
    }

    private JSValueReaderState ReadNextObjectProperty()
    {
      StackEntry entry = m_stack.Peek();
      if (entry.Property.MoveNext())
      {
        return ReadValue(entry.Property.Current.Value);
      }
      else
      {
        m_current = entry.Value;
        m_stack.Pop();
        return m_state = JSValueReaderState.ObjectEnd;
      }
    }

    private JSValueReaderState ReadNextArrayItem()
    {
      var entry = m_stack.Peek();
      if (entry.Item.MoveNext())
      {
        return ReadValue(entry.Item.Current);
      }
      else
      {
        m_current = entry.Value;
        m_stack.Pop();
        return m_state = JSValueReaderState.ArrayEnd;
      }
    }

    public string GetPropertyName()
    {
      if (m_stack.Count != 0)
      {
        var entry = m_stack.Peek();
        if (entry.Value.Type == JSValueType.Object)
        {
          return entry.Property.Current.Key;
        }
      }

      return "";
    }

    public string GetString()
    {
      return (m_state == JSValueReaderState.StringValue) ? m_stack.Peek().Value.String : "";
    }

    public bool GetBoolean()
    {
      return (m_state == JSValueReaderState.BooleanValue) ? m_stack.Peek().Value.Boolean : false;
    }

    public long GetInt64()
    {
      return (m_state == JSValueReaderState.Int64Value) ? m_stack.Peek().Value.Int64 : 0;
    }

    public double GetDouble()
    {
      return (m_state == JSValueReaderState.DoubleValue) ? m_stack.Peek().Value.Double : 0;
    }

    public JSValue Current => m_stack.Peek().Value;

    public JSValue Root { get; private set; }

    private struct StackEntry
    {
      public static StackEntry ObjectProperty(JSValue value, IEnumerator<KeyValuePair<string, JSValue>> property)
      {
        return new StackEntry
        {
          Value = value,
          Item = null,
          Property = property
        };
      }

      public static StackEntry ArrayItem(JSValue value, IEnumerator<JSValue> item)
      {
        return new StackEntry
        {
          Value = value,
          Item = item,
          Property = null
        };
      }

      public JSValue Value;
      public IEnumerator<JSValue> Item;
      public IEnumerator<KeyValuePair<string, JSValue>> Property;
    }
  }
}

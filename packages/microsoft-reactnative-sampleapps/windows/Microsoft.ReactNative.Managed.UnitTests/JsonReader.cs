using Microsoft.ReactNative.Bridge;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.ReactNative.Managed.UnitTests
{
    class JTokenReader : IJSValueReader
    {
        // Special initial internal state that we never return.
        private static readonly JSValueReaderState StartState = (JSValueReaderState)(-1);

        private readonly JToken m_root;
        private JToken m_current;
        private JSValueReaderState m_state = StartState;
        private readonly Stack<StackEntry> m_stack = new Stack<StackEntry>();

        public JTokenReader(JToken root)
        {
            m_root = root;
        }

        public JSValueReaderState ReadNext()
        {
            if (m_state == StartState)
            {
                return ReadValue(m_root);
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

        private JSValueReaderState ReadValue(JToken value)
        {
            if (value == null)
            {
                return m_state = JSValueReaderState.Error;
            }

            m_current = value;
            switch (value.Type)
            {
                case JTokenType.Null:
                    return m_state = JSValueReaderState.NullValue;
                case JTokenType.Object:
                    return m_state = JSValueReaderState.ObjectBegin;
                case JTokenType.Array:
                    return m_state = JSValueReaderState.ArrayBegin;
                case JTokenType.String:
                    return m_state = JSValueReaderState.StringValue;
                case JTokenType.Boolean:
                    return m_state = JSValueReaderState.BooleanValue;
                case JTokenType.Integer:
                    return m_state = JSValueReaderState.Int64Value;
                case JTokenType.Float:
                    return m_state = JSValueReaderState.DoubleValue;
                default:
                    return m_state = JSValueReaderState.Error;
            }
        }

        private JSValueReaderState ReadObject()
        {
            var properties = ((JObject)m_current).Properties().GetEnumerator();
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
            var items = ((JArray)m_current).GetEnumerator();
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
                case JTokenType.Object:
                    return ReadNextObjectProperty();
                case JTokenType.Array:
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
                if (entry.Value.Type == JTokenType.Object)
                {
                    return entry.Property.Current.Name;
                }
            }

            return "";
        }

        public string GetString()
        {
            return (m_state == JSValueReaderState.StringValue) ? (string)(JValue)m_stack.Peek().Value : "";
        }

        public bool GetBoolean()
        {
            return (m_state == JSValueReaderState.BooleanValue) ? (bool)(JValue)m_stack.Peek().Value : false;
        }

        public long GetInt64()
        {
            return (m_state == JSValueReaderState.Int64Value) ? (long)(JValue)m_stack.Peek().Value : 0;
        }

        public double GetDouble()
        {
            return (m_state == JSValueReaderState.DoubleValue) ? (double)(JValue)m_stack.Peek().Value : 0;
        }

        private struct StackEntry
        {
            public static StackEntry ObjectProperty(JToken value, IEnumerator<JProperty> property)
            {
                return new StackEntry
                {
                    Value = value,
                    Item = null,
                    Property = property
                };
            }

            public static StackEntry ArrayItem(JToken value, IEnumerator<JToken> item)
            {
                return new StackEntry
                {
                    Value = value,
                    Item = item,
                    Property = null
                };
            }

            public JToken Value;
            public IEnumerator<JToken> Item;
            public IEnumerator<JProperty> Property;
        }
    }
}

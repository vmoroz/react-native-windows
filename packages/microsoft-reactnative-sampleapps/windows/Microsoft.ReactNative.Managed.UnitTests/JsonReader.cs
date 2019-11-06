using Microsoft.ReactNative.Bridge;
using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.ReactNative.Managed.UnitTests
{
    class JsonReader : IJSValueReader
    {
        public JsonReader(JToken root)
        {
            m_root = root;
        }

        public JSValueReaderState ReadNext()
        {
            switch (m_state)
            {
                case JSValueReaderState.Error:
                    {
                        // The Error state is the initial state if m_root is not null.
                        if (m_stack.Count != 0 && m_root == null)
                        {
                            return JSValueReaderState.Error;
                        }

                        JTokenType type = m_root.Type;
                        if (type == JTokenType.Object)
                        {
                            m_stack.Add(new StackEntry(m_root));
                            return m_state = JSValueReaderState.ObjectBegin;
                        }
                        else if (type == JTokenType.Array)
                        {
                            m_stack.Add(new StackEntry(m_root));
                            return m_state = JSValueReaderState.ArrayBegin;
                        }
                        else
                        {
                            return JSValueReaderState.Error;
                        }
                    }
                case JSValueReaderState.ObjectBegin:
                    {
                        var entry = m_stack.Last();
                        entry.Property = ((JObject)entry.Value).Properties().GetEnumerator();
                        if (entry.Property.MoveNext())
                        {
                            return m_state = JSValueReaderState.PropertyName;
                        }
                        else
                        {
                            return m_state = JSValueReaderState.ObjectEnd;
                        }
                    }
                case JSValueReaderState.ArrayBegin:
                    {
                        var entry = m_stack.Last();
                        entry.Item = ((JArray)entry.Value).GetEnumerator();
                        if (entry.Item.MoveNext())
                        {
                            return ReadValue(entry.Item.Current);
                        }
                        else
                        {
                            return m_state = JSValueReaderState.ArrayEnd;
                        }
                    }
                case JSValueReaderState.PropertyName:
                    {
                        var entry = m_stack.Last();
                        try
                        {
                            return ReadValue(entry.Property.Current.Value);
                        }
                        catch (InvalidOperationException)
                        {
                            return m_state = JSValueReaderState.Error;
                        }
                    }
                case JSValueReaderState.ObjectEnd:
                case JSValueReaderState.ArrayEnd:
                case JSValueReaderState.NullValue:
                case JSValueReaderState.BooleanValue:
                case JSValueReaderState.Int64Value:
                case JSValueReaderState.DoubleValue:
                case JSValueReaderState.StringValue:
                    {
                        m_stack.RemoveAt(m_stack.Count - 1);
                        if (!m_stack.Any())
                        {
                            return m_state = JSValueReaderState.Error;
                        }

                        var entry = m_stack.Last();
                        JTokenType type = entry.Value.Type;
                        if (type == JTokenType.Object)
                        {
                            if (entry.Property.MoveNext())
                            {
                                return m_state = JSValueReaderState.PropertyName;
                            }
                            else
                            {
                                return m_state = JSValueReaderState.ObjectEnd;
                            }
                        }
                        else if (type == JTokenType.Array)
                        {
                            if (entry.Item.MoveNext())
                            {
                                return ReadValue(entry.Item.Current);
                            }
                            else
                            {
                                return m_state = JSValueReaderState.ArrayEnd;
                            }
                        }

                        return m_state = JSValueReaderState.Error;
                    }
                default:
                    return m_state = JSValueReaderState.Error;
            }
        }

        public bool TryGetBoolen(out bool value)
        {
            if (m_state == JSValueReaderState.BooleanValue)
            {
                value = (bool)(JValue)m_stack.Last().Value;
                return true;
            }

            value = false;
            return false;

        }

        public bool TryGetInt64(out long value)
        {
            if (m_state == JSValueReaderState.Int64Value)
            {
                value = (long)(JValue)m_stack.Last().Value;
                return true;
            }
            else if (m_state == JSValueReaderState.DoubleValue)
            {
                value = (long)(double)(JValue)m_stack.Last().Value;
                return true;
            }

            value = 0;
            return false;
        }

        public bool TryGetDouble(out double value)
        {
            if (m_state == JSValueReaderState.DoubleValue)
            {
                value = (double)(JValue)m_stack.Last().Value;
                return true;
            }
            else if (m_state == JSValueReaderState.Int64Value)
            {
                value = (double)(long)(JValue)m_stack.Last().Value;
                return true;
            }

            value = 0;
            return false;
        }

        public bool TryGetString(out string value)
        {
            if (m_state == JSValueReaderState.StringValue)
            {
                value = (string)(JValue)m_stack.Last().Value;
                return true;
            }
            else if (m_state == JSValueReaderState.PropertyName)
            {
                value = m_stack.Last().Property.Current.Name;
                return true;
            }

            value = "";
            return false;
        }

        private class StackEntry
        {
            public StackEntry(JToken value)
            {
                Value = value;
            }

            public JToken Value = null;
            public IEnumerator<JToken> Item = null;
            public IEnumerator<JProperty> Property = null;
        }

        private JSValueReaderState ReadValue(JToken value)
        {
            m_stack.Add(new StackEntry(value));
            switch (value.Type)
            {
                case JTokenType.Null:
                    return m_state = JSValueReaderState.NullValue;
                case JTokenType.Boolean:
                    return m_state = JSValueReaderState.BooleanValue;
                case JTokenType.Integer:
                    return m_state = JSValueReaderState.Int64Value;
                case JTokenType.Float:
                    return m_state = JSValueReaderState.DoubleValue;
                case JTokenType.String:
                    return m_state = JSValueReaderState.StringValue;
                case JTokenType.Object:
                    return m_state = JSValueReaderState.ObjectBegin;
                case JTokenType.Array:
                    return m_state = JSValueReaderState.ArrayBegin;
                default:
                    return m_state = JSValueReaderState.Error;
            }
        }

        private JToken m_root;
        private JSValueReaderState m_state = JSValueReaderState.Error;
        private List<StackEntry> m_stack = new List<StackEntry>();
    }
}


using Microsoft.ReactNative.Bridge;
using Newtonsoft.Json.Linq;
using System.Collections.Generic;
using System.Linq;

namespace Microsoft.ReactNative.Managed.UnitTests
{
    class JsonWriter : IJSValueWriter
    {
        public JToken TakeValue()
        {
            return m_result;
        }

        public bool WriteNull()
        {
            return WriteValue(new JValue((object)null));
        }

        public bool WriteBoolean(bool value)
        {
            return WriteValue(new JValue(value));
        }

        public bool WriteInt64(long value)
        {
            return WriteValue(new JValue(value));
        }

        public bool WriteDouble(double value)
        {
            return WriteValue(new JValue(value));
        }

        public bool WriteString(string value)
        {
            return WriteValue(new JValue(value));
        }

        public bool WriteObjectBegin()
        {
            if (m_state == State.PropertyValue)
            {
                m_stack.Add(new StackEntry(m_state, (JObject)m_dynamic, m_propertyName));
            }
            else if (m_state == State.Array)
            {
                m_stack.Add(new StackEntry(m_state, (JArray)m_dynamic));
            }
            else if (m_state != State.Start)
            {
                return false;
            }

            m_dynamic = new JObject();
            m_state = State.PropertyName;

            return true;

        }

        public bool WritePropertyName(string name)
        {
            if (m_state == State.PropertyName)
            {
                m_propertyName = name;
                m_state = State.PropertyValue;
                return true;
            }

            return false;
        }

        public bool WriteObjectEnd()
        {
            if (m_state == State.PropertyName)
            {
                if (m_stack.Count == 0)
                {
                    m_result = m_dynamic;
                    m_state = State.Finish;
                }
                else
                {
                    StackEntry entry = m_stack.Last();
                    if (entry.State == State.PropertyValue)
                    {
                        ((JObject)entry.Dynamic)[entry.PropertyName] = m_dynamic;
                        m_dynamic = entry.Dynamic;
                        m_state = State.PropertyName;
                        m_stack.RemoveAt(m_stack.Count - 1);
                        return true;
                    }
                    else if (entry.State == State.Array)
                    {
                        entry.Dynamic.Add(m_dynamic);
                        m_dynamic = entry.Dynamic;
                        m_state = State.Array;
                        m_stack.RemoveAt(m_stack.Count - 1);
                        return true;
                    }
                }
            }

            return false;
        }

        public bool WriteArrayBegin()
        {
            if (m_state == State.PropertyValue)
            {
                m_stack.Add(new StackEntry(m_state, (JObject)m_dynamic, m_propertyName));
            }
            else if (m_state == State.Array)
            {
                m_stack.Add(new StackEntry(m_state, (JArray)m_dynamic));
            }
            else if (m_state != State.Start)
            {
                return false;
            }

            m_dynamic = new JArray();
            m_state = State.Array;
            return true;
        }

        public bool WriteArrayEnd()
        {
            if (m_state == State.Array)
            {
                if (m_stack.Count == 0)
                {
                    m_result = m_dynamic;
                    m_state = State.Finish;
                }
                else
                {
                    StackEntry entry = m_stack.Last();
                    if (entry.State == State.PropertyValue)
                    {
                        ((JObject)entry.Dynamic)[entry.PropertyName] = m_dynamic;
                        m_dynamic = entry.Dynamic;
                        m_state = State.PropertyName;
                        m_stack.RemoveAt(m_stack.Count - 1);
                        return true;
                    }
                    else if (entry.State == State.Array)
                    {
                        entry.Dynamic.Add(m_dynamic);
                        m_dynamic = entry.Dynamic;
                        m_state = State.Array;
                        m_stack.RemoveAt(m_stack.Count - 1);
                        return true;
                    }
                }
            }

            return false;
        }

        private enum State { Start, PropertyName, PropertyValue, Array, Finish };

        class StackEntry
        {
            public StackEntry(State state, JObject obj, string propertyName)
            {
                State = state;
                Dynamic = obj;
                PropertyName = propertyName;
            }

            public StackEntry(State state, JArray arr)
            {
                State = state;
                Dynamic = arr;
                PropertyName = null;
            }

            public State State;
            public JContainer Dynamic;
            public string PropertyName;
        }

        private bool WriteValue(JToken value)
        {
            if (m_state == State.PropertyValue)
            {
                m_dynamic[m_propertyName] = value;
                m_state = State.PropertyName;
                return true;
            }
            else if (m_state == State.Array)
            {
                ((JArray)m_dynamic).Add(value);
                return true;
            }

            return false;

        }

        private State m_state = State.Start;
        private List<StackEntry> m_stack = new List<StackEntry>();
        private JToken m_dynamic;
        private string m_propertyName;
        private JToken m_result;
    }
}

// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "JSValue.h"
#include "JSValueTreeReader.h"

namespace winrt::Microsoft::ReactNative::Bridge {

//===========================================================================
// JSValue implementation
//===========================================================================

/*static*/ const JSValue JSValue::Null;
/*static*/ const JSValueObject JSValue::EmptyObject;
/*static*/ const JSValueArray JSValue::EmptyArray;
/*static*/ const std::string JSValue::EmptyString;

JSValue::JSValue(JSValue &&other) noexcept : m_type{other.m_type} {
  switch (m_type) {
    case JSValueType::Object:
      new (&m_object) JSValueObject(std::move(other.m_object));
      break;
    case JSValueType::Array:
      new (&m_array) JSValueArray(std::move(other.m_array));
      break;
    case JSValueType::String:
      new (&m_string) std::string(std::move(other.m_string));
      break;
    case JSValueType::Boolean:
      m_bool = other.m_bool;
      break;
    case JSValueType::Int64:
      m_int64 = other.m_int64;
      break;
    case JSValueType::Double:
      m_double = other.m_double;
      break;
  }

  other.m_type = JSValueType::Null;
  other.m_int64 = 0;
}

JSValue &JSValue::operator=(JSValue &&other) noexcept {
  if (this != &other) {
    this->~JSValue();
    new (this) JSValue(std::move(other));
  }

  return *this;
}

JSValue::~JSValue() noexcept {
  switch (m_type) {
    case JSValueType::Object:
      m_object.~map();
      break;
    case JSValueType::Array:
      m_array.~vector();
      break;
    case JSValueType::String:
      m_string.~basic_string();
      break;
  }

  m_int64 = 0;
  m_type = JSValueType::Null;
}

JSValue JSValue::Copy() const noexcept {
  switch (m_type) {
    case JSValueType::Object:
      return JSValue{CopyObject(m_object)};
    case JSValueType::Array:
      return JSValue{CopyArray(m_array)};
    case JSValueType::String:
      return JSValue{std::string(m_string)};
    case JSValueType::Boolean:
      return JSValue{m_bool};
    case JSValueType::Int64:
      return JSValue{m_int64};
    case JSValueType::Double:
      return JSValue{m_double};
    default:
      return JSValue{};
  }
}

/*static*/ JSValueObject JSValue::CopyObject(const JSValueObject &other) noexcept {
  JSValueObject result;
  for (const auto &property : other) {
    result.emplace(property.first, property.second.Copy());
  }

  return result;
}

/*static*/ JSValueArray JSValue::CopyArray(const JSValueArray &other) noexcept {
  JSValueArray result(other.size());
  for (const auto &item : other) {
    result.push_back(item.Copy());
  }

  return result;
}

const JSValue &JSValue::GetObjectProperty(std::string_view propertyName) const noexcept {
  if (m_type == JSValueType::Object) {
    auto it = m_object.find(propertyName);
    if (it != m_object.end()) {
      return it->second;
    }
  }

  return Null;
}

const JSValue &JSValue::GetArrayItem(JSValueArray::size_type index) const noexcept {
  if (m_type == JSValueType::Array && index < m_array.size()) {
    return m_array[index];
  }

  return Null;
}

bool JSValue::Equals(const JSValue &other) const noexcept {
  if (m_type == other.m_type) {
    switch (m_type) {
      case JSValueType::Null:
        return true;
      case JSValueType::Object:
        return ObjectEquals(other.m_object);
      case JSValueType::Array:
        return ArrayEquals(other.m_array);
      case JSValueType::String:
        return m_string == other.m_string;
      case JSValueType::Boolean:
        return m_bool == other.m_bool;
      case JSValueType::Int64:
        return m_int64 == other.m_int64;
      case JSValueType::Double:
        return m_double == other.m_double;
      default:
        return false;
    }
  }

  return false;
}

/*static*/ JSValue JSValue::ReadFrom(IJSValueReader &reader) noexcept {
  if (auto treeReader = reader.try_as<IJSValueTreeReader>()) {
    return treeReader->Current().Copy();
  }

  return ReadValue(reader);
}

// TODO: implement
// static Dictionary<string, JSValue> ReadObjectPropertiesFrom(IJSValueReader reader) {
//   if (reader.ValueType == JSValueType.Object) {
//     var treeReader = reader as IJSValueTreeReader;
//     if (treeReader != null) {
//       return new Dictionary<string, JSValue>(treeReader.Current.Object as IDictionary<string, JSValue>);
//     }

//     return ReadObjectProperties(reader);
//   }

//   return new Dictionary<string, JSValue>();
// }

// TODO: implement
// static List<JSValue> ReadArrayItemsFrom(IJSValueReader reader) {
//   if (reader.ValueType == JSValueType.Array) {
//     var treeReader = reader as IJSValueTreeReader;
//     if (treeReader != null) {
//       return new List<JSValue>(treeReader.Current.Array as IList<JSValue>);
//     }

//     return ReadArrayItems(reader);
//   }

//   return new List<JSValue>();
// }

void JSValue::WriteTo(IJSValueWriter &writer) const noexcept {
  switch (m_type) {
    case JSValueType::Null:
      writer.WriteNull();
      break;
    case JSValueType::Object:
      WriteObject(writer, m_object);
      break;
    case JSValueType::Array:
      WriteArray(writer, m_array);
      break;
    case JSValueType::String:
      writer.WriteString(to_hstring(m_string));
      break;
    case JSValueType::Boolean:
      writer.WriteBoolean(m_bool);
      break;
    case JSValueType::Int64:
      writer.WriteInt64(m_int64);
      break;
    case JSValueType::Double:
      writer.WriteDouble(m_double);
      break;
  }
}

/*static*/ void JSValue::WriteObject(IJSValueWriter &writer, JSValueObjectView object) noexcept {
  writer.WriteObjectBegin();
  for (const auto &property : object) {
    writer.WritePropertyName(to_hstring(property.first));
    property.second.WriteTo(writer);
  }
  writer.WriteObjectEnd();
}

/*static*/ void JSValue::WriteArray(IJSValueWriter &writer, JSValueArrayView value) noexcept {
  writer.WriteArrayBegin();
  for (const JSValue &item : value) {
    item.WriteTo(writer);
  }
  writer.WriteArrayEnd();
}

bool JSValue::ObjectEquals(const JSValueObject &other) const noexcept {
  if (m_object.size() != other.size()) {
    return false;
  }

  for (const auto &entry : m_object) {
    auto it = other.find(entry.first);
    if (it == other.end()) {
      return false;
    }

    if (!entry.second.Equals(it->second)) {
      return false;
    }
  }

  return true;
}

bool JSValue::ArrayEquals(const JSValueArray &other) const noexcept {
  if (m_array.size() != other.size()) {
    return false;
  }

  auto otherIt = other.begin();
  for (const JSValue &item : m_array) {
    if (!item.Equals(*(otherIt++))) {
      return false;
    }
  }

  return true;
}

// Read value from IJSValueReader without checking for IJSValueTreeReader.
/*static*/ JSValue JSValue::ReadValue(IJSValueReader &reader) noexcept {
  switch (reader.ValueType()) {
    case JSValueType::Null:
      return JSValue();
    case JSValueType::Object:
      return JSValue(ReadObjectProperties(reader));
    case JSValueType::Array:
      return JSValue(ReadArrayItems(reader));
    case JSValueType::String:
      return JSValue(to_string(reader.GetString()));
    case JSValueType::Boolean:
      return JSValue(reader.GetBoolean());
    case JSValueType::Int64:
      return JSValue(reader.GetInt64());
    case JSValueType::Double:
      return JSValue(reader.GetDouble());
    default:
      VerifyElseCrashSz(false, "Unexpected JSValueType");
  }
}

// Read object properties without checking value type.
/*static*/ JSValueObject JSValue::ReadObjectProperties(IJSValueReader &reader) noexcept {
  JSValueObject properties;
  hstring propertyName;
  while (reader.GetNextObjectProperty(/*ref*/ propertyName)) {
    properties.emplace(to_string(propertyName), ReadValue(reader));
  }

  return properties;
}

// Read array items without checking value type.
/*static*/ JSValueArray JSValue::ReadArrayItems(IJSValueReader &reader) noexcept {
  JSValueArray items;
  while (reader.GetNextArrayItem()) {
    items.push_back(ReadValue(reader));
  }

  return items;
}

//===========================================================================
// Standalone inline functions implementation
//===========================================================================

void swap(JSValue &left, JSValue &right) noexcept {
  JSValue temp{std::move(right)};
  right = std::move(left);
  left = std::move(temp);
}

} // namespace winrt::Microsoft::ReactNative::Bridge

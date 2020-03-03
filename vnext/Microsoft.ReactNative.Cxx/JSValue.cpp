// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "JSValue.h"
#include <iomanip>
#include <sstream>
#include <string_view>

namespace winrt::Microsoft::ReactNative {

//===========================================================================
// JSValueObject implementation
//===========================================================================

JSValueObject::JSValueObject(std::initializer_list<JSValueObjectKeyValue> initObject) noexcept {
  for (auto const &item : initObject) {
    this->try_emplace(std::string(item.Key), std::move(*const_cast<JSValue *>(&item.Value)));
  }
}

JSValueObject::JSValueObject(std::map<std::string, JSValue, std::less<>> &&other) noexcept : map{std::move(other)} {}

JSValueObject JSValueObject::Copy() const noexcept {
  JSValueObject object;
  for (auto const &property : *this) {
    object.try_emplace(property.first, property.second.Copy());
  }

  return object;
}

bool JSValueObject::Equals(JSValueObject const &other) const noexcept {
  if (size() != other.size()) {
    return false;
  }

  // std::map keeps key-values in an ordered sequence.
  // Make sure that pairs are matching at the same position.
  auto otherIt = other.begin();
  for (auto const &property : *this) {
    auto it = otherIt++;
    if (property.first != it->first || !property.second.Equals(it->second)) {
      return false;
    }
  }

  return true;
}

bool JSValueObject::EqualsAfterConversion(JSValueObject const &other) const noexcept {
  if (size() != other.size()) {
    return false;
  }

  // std::map keeps key-values in an ordered sequence.
  // Make sure that pairs are matching at the same position.
  auto otherIt = other.begin();
  for (auto const &property : *this) {
    auto it = otherIt++;
    if (property.first != it->first || !property.second.EqualsAfterConversion(it->second)) {
      return false;
    }
  }

  return true;
}

/*static*/ JSValueObject JSValueObject::ReadFrom(IJSValueReader const &reader) noexcept {
  JSValueObject object;
  if (reader.ValueType() == JSValueType::Object) {
    hstring propertyName;
    while (reader.GetNextObjectProperty(/*ref*/ propertyName)) {
      object.try_emplace(to_string(propertyName), JSValue::ReadFrom(reader));
    }
  }

  return object;
}

void JSValueObject::WriteTo(IJSValueWriter const &writer) const noexcept {
  writer.WriteObjectBegin();
  for (auto const &property : *this) {
    writer.WritePropertyName(to_hstring(property.first));
    property.second.WriteTo(writer);
  }
  writer.WriteObjectEnd();
}

//===========================================================================
// JSValueArray implementation
//===========================================================================

JSValueArray::JSValueArray(std::initializer_list<JSValueArrayItem> initArray) noexcept {
  for (auto const &item : initArray) {
    this->push_back(std::move(*const_cast<JSValue *>(&item.Item)));
  }
}

JSValueArray::JSValueArray(std::vector<JSValue> &&other) noexcept : vector{std::move(other)} {}

JSValueArray JSValueArray::Copy() const noexcept {
  JSValueArray array;
  array.reserve(size());
  for (auto const &item : *this) {
    array.push_back(item.Copy());
  }

  return array;
}

bool JSValueArray::Equals(JSValueArray const &other) const noexcept {
  if (size() != other.size()) {
    return false;
  }

  auto otherIt = other.begin();
  for (auto const &item : *this) {
    if (!item.Equals(*otherIt++)) {
      return false;
    }
  }

  return true;
}

bool JSValueArray::EqualsAfterConversion(JSValueArray const &other) const noexcept {
  if (size() != other.size()) {
    return false;
  }

  auto otherIt = other.begin();
  for (auto const &item : *this) {
    if (!item.EqualsAfterConversion(*otherIt++)) {
      return false;
    }
  }

  return true;
}

/*static*/ JSValueArray JSValueArray::ReadFrom(IJSValueReader const &reader) noexcept {
  JSValueArray array;
  if (reader.ValueType() == JSValueType::Array) {
    while (reader.GetNextArrayItem()) {
      array.push_back(ReadFrom(reader));
    }
  }

  return array;
}

void JSValueArray::WriteTo(IJSValueWriter const &writer) const noexcept {
  writer.WriteArrayBegin();
  for (const JSValue &item : *this) {
    item.WriteTo(writer);
  }
  writer.WriteArrayEnd();
}

//===========================================================================
// JSValue implementation
//===========================================================================

/*static*/ const JSValue JSValue::Null;
/*static*/ const JSValueObject JSValue::EmptyObject;
/*static*/ const JSValueArray JSValue::EmptyArray;
/*static*/ const std::string JSValue::EmptyString;

#pragma warning(push)
#pragma warning(disable : 26495) // False positive for union member not initialized
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
#pragma warning(pop)

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
      m_object.~JSValueObject();
      break;
    case JSValueType::Array:
      m_array.~JSValueArray();
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
      return JSValue{m_object.Copy()};
    case JSValueType::Array:
      return JSValue{m_array.Copy()};
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

namespace {

struct AsStringFormatter {
  AsStringFormatter(JSValue const &jsValue) noexcept : m_jsValue{jsValue} {}
  friend std::ostream &operator<<(std::ostream &stream, AsStringFormatter const &formatter) noexcept {
    auto const &jsValue = formatter.m_jsValue;

    if (jsValue.IsNull()) {
      return stream << "null";
    } else if (jsValue.GetIfObject()) {
      return stream << "[object Object]";
    } else if (auto arr = jsValue.GetIfArray()) {
      bool start = true;
      for (auto const &item : *arr) {
        stream << (start ? (start = false, "") : ",") << AsStringFormatter{item};
      }
      return stream;
    } else if (auto str = jsValue.GetIfString()) {
      return stream << *str;
    } else if (auto b = jsValue.GetIfBoolean()) {
      return stream << (*b ? "true" : "false");
    } else if (auto i = jsValue.GetIfInt64()) {
      return stream << *i;
    } else if (auto d = jsValue.GetIfDouble()) {
      return stream << *d;
    } else {
      return stream << "<Unexpected>";
    }
  }

 private:
  JSValue const &m_jsValue;
};

} // namespace

std::string JSValue::AsString() const noexcept {
  switch (m_type) {
    case JSValueType::Null:
      return "null";
    case JSValueType::Object:
      return "[object Object]";
    case JSValueType::Array: {
      std::stringstream ss;
      ss << AsStringFormatter(*this);
      return ss.str();
    }
    case JSValueType::String:
      return m_string;
    case JSValueType::Boolean:
      return m_bool ? "true" : "false";
    case JSValueType::Int64:
      return std::to_string(m_int64);
    case JSValueType::Double:
      return std::to_string(m_double);
    default:
      return "<unexpected>";
  }
}

bool JSValue::AsBoolean() const noexcept {
  switch (m_type) {
    case JSValueType::Object:
      return true;
    case JSValueType::Array:
      return true;
    case JSValueType::String:
      return !m_string.empty();
    case JSValueType::Boolean:
      return m_bool;
    case JSValueType::Int64:
      return m_int64 != 0;
    case JSValueType::Double:
      return m_double != 0;
    default:
      return false;
  }
}

int8_t JSValue::AsInt8() const noexcept {
  return static_cast<int8_t>(AsInt64());
}

int16_t JSValue::AsInt16() const noexcept {
  return static_cast<int16_t>(AsInt64());
}

int32_t JSValue::AsInt32() const noexcept {
  return static_cast<int32_t>(AsInt64());
}

int64_t JSValue::AsInt64() const noexcept {
  switch (m_type) {
    case JSValueType::Object:
    case JSValueType::Array:
    case JSValueType::String:
      return static_cast<int64_t>(AsDouble());
    case JSValueType::Boolean:
      return m_bool ? 1 : 0;
    case JSValueType::Int64:
      return m_int64;
    case JSValueType::Double:
      return static_cast<int64_t>(m_double);
    default:
      return 0;
  }
}

uint8_t JSValue::AsUInt8() const noexcept {
  return static_cast<uint8_t>(AsInt64());
}

uint16_t JSValue::AsUInt16() const noexcept {
  return static_cast<uint16_t>(AsInt64());
}

uint32_t JSValue::AsUInt32() const noexcept {
  return static_cast<uint32_t>(AsInt64());
}

uint64_t JSValue::AsUInt64() const noexcept {
  return static_cast<uint64_t>(AsInt64());
}

namespace {

std::string_view TrimString(std::string const &value) noexcept {
  constexpr char const *WhiteSpace = " \n\r\t\f\v";

  size_t start = value.find_first_not_of(WhiteSpace);
  if (start == std::string::npos) {
    return "";
  }
  size_t end = value.find_last_not_of(WhiteSpace);
  if (end == std::string::npos) {
    return "";
  }
  return {value.data() + start, end - start + 1};
}

} // namespace

double JSValue::AsDouble() const noexcept {
  switch (m_type) {
    case JSValueType::Object:
      return NAN;
    case JSValueType::Array: {
      switch (m_array.size()) {
        case 0:
          return 0;
        case 1:
          return m_array[0].AsDouble();
        default:
          return NAN;
      }
    }
    case JSValueType::String: {
      auto trimmedStr = TrimString(m_string);
      if (trimmedStr.empty()) {
        return 0;
      }
      char *end;
      double result = strtod(trimmedStr.data(), &end);
      return (end == trimmedStr.data() + trimmedStr.size()) ? result : NAN;
    }
    case JSValueType::Boolean:
      return m_bool ? 1 : 0;
    case JSValueType::Int64:
      return static_cast<double>(m_int64);
    case JSValueType::Double:
      return m_double;
    default:
      return 0;
  }
}

float JSValue::AsFloat() const noexcept {
  return static_cast<float>(AsDouble());
}

JSValueObject JSValue::MoveObject() noexcept {
  JSValueObject result;
  if (m_type == JSValueType::Object) {
    result = std::move(m_object);
    m_int64 = 0;
    m_type = JSValueType::Null;
  }
  return result;
}

JSValueArray JSValue::MoveArray() noexcept {
  JSValueArray result;
  if (m_type == JSValueType::Array) {
    result = std::move(m_array);
    m_int64 = 0;
    m_type = JSValueType::Null;
  }
  return result;
}

namespace {

struct JsonStringFormatter {
  JsonStringFormatter(std::string_view stringView) noexcept : m_stringView{stringView} {}
  friend std::ostream &operator<<(std::ostream &stream, JsonStringFormatter const &formatter) noexcept {
    auto writeChar = [](std::ostream &stream, char ch) noexcept -> std::ostream & {
      switch (ch) {
        case '"':
          return stream << "\\\"";
        case '\\':
          return stream << "\\\\";
        case '\b':
          return stream << "\\b";
        case '\f':
          return stream << "\\f";
        case '\n':
          return stream << "\\n";
        case '\r':
          return stream << "\\r";
        case '\t':
          return stream << "\\t";
        default:
          if ('\x00' <= ch && ch <= '\x1f') {
            return stream << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)ch;
          } else {
            return stream << ch;
          }
      }
    };

    stream << '"';
    for (auto ch : formatter.m_stringView) {
      writeChar(stream, ch);
    }
    return stream << '"';
  }

 private:
  std::string_view m_stringView;
};

struct JsonJSValueFormatter {
  JsonJSValueFormatter(JSValue const &jsValue) noexcept : m_jsValue{jsValue} {}
  friend std::ostream &operator<<(std::ostream &stream, JsonJSValueFormatter const &formatter) noexcept {
    auto const &jsValue = formatter.m_jsValue;

    if (jsValue.IsNull()) {
      return stream << "null";
    } else if (auto obj = jsValue.GetIfObject()) {
      stream << "{";
      bool start = true;
      for (auto const &prop : *obj) {
        stream << start ? (start = false, "") : ", ";
        stream << JsonStringFormatter{prop.first} << ": " << JsonJSValueFormatter{prop.second};
      }
      return stream << "}";
    } else if (auto arr = jsValue.GetIfArray()) {
      stream << "[";
      bool start = true;
      for (auto const &item : *arr) {
        stream << start ? (start = false, "") : ", ";
        stream << JsonJSValueFormatter{item};
      }
      return stream << "]";
    } else if (auto str = jsValue.GetIfString()) {
      return stream << JsonStringFormatter{*str};
    } else if (auto b = jsValue.GetIfBoolean()) {
      return stream << (*b ? "true" : "false");
    } else if (auto i = jsValue.GetIfInt64()) {
      return stream << *i;
    } else if (auto d = jsValue.GetIfDouble()) {
      return stream << *d;
    } else {
      return stream << "<Unexpected>";
    }
  }

 private:
  JSValue const &m_jsValue;
};

} // namespace

std::string JSValue::ToString() const noexcept {
  std::ostringstream stream;
  stream << JsonJSValueFormatter{*this};
  return stream.str();
}

size_t JSValue::PropertyCount() const noexcept {
  return (m_type == JSValueType::Object) ? m_object.size() : 0;
}

JSValue const &JSValue::GetObjectProperty(std::string_view propertyName) const noexcept {
  if (m_type == JSValueType::Object) {
    auto it = m_object.find(propertyName);
    if (it != m_object.end()) {
      return it->second;
    }
  }

  return Null;
}

size_t JSValue::ItemCount() const noexcept {
  return (m_type == JSValueType::Array) ? m_array.size() : 0;
}

JSValue const &JSValue::GetArrayItem(JSValueArray::size_type index) const noexcept {
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
        return m_object.Equals(other.m_object);
      case JSValueType::Array:
        return m_array.Equals(other.m_array);
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

bool JSValue::EqualsAfterConversion(const JSValue &other) const noexcept {
  if (m_type == other.m_type) {
    return Equals(other);
  } else {
    switch (m_type) {
      case JSValueType::Null:
        return false;
      case JSValueType::Object:
        return other.m_type == JSValueType::Boolean && other.m_bool;
      case JSValueType::Array:
        return other.m_type == JSValueType::Boolean && other.m_bool;
      case JSValueType::String:
        switch (other.m_type) {
          case JSValueType::Boolean:
            return !m_string.empty() && other.m_bool;
          case JSValueType::Int64:
            return AsInt64() == other.m_int64;
          case JSValueType::Double:
            return AsDouble() == other.m_double;
          default:
            return other.EqualsAfterConversion(*this);
        }
      case JSValueType::Boolean:
        switch (other.m_type) {
          case JSValueType::Int64:
            return (m_bool ? 1 : 0) == other.m_int64;
          case JSValueType::Double:
            return (m_bool ? 1 : 0) == other.m_double;
          default:
            return other.EqualsAfterConversion(*this);
        }
      case JSValueType::Int64:
        if (other.m_type == JSValueType::Double) {
          return m_int64 == other.m_double;
        } else {
          return other.EqualsAfterConversion(*this);
        }
      default:
        return false;
    }
  }

  return false;
}

/*static*/ JSValue JSValue::ReadFrom(IJSValueReader const &reader) noexcept {
  switch (reader.ValueType()) {
    case JSValueType::Null:
      return JSValue();
    case JSValueType::Object:
      return JSValue(JSValueObject::ReadFrom(reader));
    case JSValueType::Array:
      return JSValue(JSValueArray::ReadFrom(reader));
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

void JSValue::WriteTo(IJSValueWriter const &writer) const noexcept {
  switch (m_type) {
    case JSValueType::Null:
      writer.WriteNull();
      break;
    case JSValueType::Object:
      m_object.WriteTo(writer);
      break;
    case JSValueType::Array:
      m_array.WriteTo(writer);
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

} // namespace winrt::Microsoft::ReactNative

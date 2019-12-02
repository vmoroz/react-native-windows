// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "Crash.h"
#include "winrt/Microsoft.ReactNative.Bridge.h"

namespace winrt::Microsoft::ReactNative {

// Forward declarations
struct JSValue;

using JSValueObject = std::map<std::string, JSValue, std::less<>>;
using JSValueArray = std::vector<JSValue>;

struct JSValueObjectView {
  JSValueObjectView(const JSValueObject &object) noexcept : m_object{object} {}

  auto begin() const noexcept {
    return m_object.begin();
  }

  auto end() const noexcept {
    return m_object.end();
  }

 private:
  const JSValueObject &m_object;
};

struct JSValueArrayView {
  JSValueArrayView(const JSValueArray &array) noexcept : m_array{array} {}

  auto begin() const noexcept {
    return m_array.begin();
  }

  auto end() const noexcept {
    return m_array.end();
  }

 private:
  const JSValueArray &m_array;
};

// JSValue represents an immutable JavaScript value passed as a parameter.
// It is created to simplify working with IJSValueReader.
// The JSValue is an immutable and is safe to be used from multiple threads.
struct JSValue {
  static const JSValue Null;

  JSValue() noexcept : m_type{Bridge::JSValueType::Null}, m_int64{0} {}
  JSValue(std::nullptr_t) noexcept : m_type{Bridge::JSValueType::Null}, m_int64{0} {}
  JSValue(JSValueObject &&value) noexcept : m_type{Bridge::JSValueType::Object}, m_object{std::move(value)} {}
  JSValue(JSValueArray &&value) noexcept : m_type{Bridge::JSValueType::Array}, m_array(std::move(value)) {}
  JSValue(std::string &&value) noexcept : m_type{Bridge::JSValueType::String}, m_string{std::move(value)} {}
  JSValue(bool value) noexcept : m_type{Bridge::JSValueType::Boolean}, m_bool{value} {}
  JSValue(int64_t value) noexcept : m_type{Bridge::JSValueType::Int64}, m_int64{value} {}
  JSValue(double value) noexcept : m_type{Bridge::JSValueType::Double}, m_double{value} {}

  JSValue(JSValue &&other) noexcept : m_type{other.m_type} {
    switch (m_type) {
      case Bridge::JSValueType::Object:
        m_object = std::move(other.m_object);
        break;
      case Bridge::JSValueType::Array:
        m_array = std::move(other.m_array);
        break;
      case Bridge::JSValueType::String:
        m_string = std::move(other.m_string);
        break;
      case Bridge::JSValueType::Boolean:
        m_bool = other.m_bool;
        break;
      case Bridge::JSValueType::Int64:
        m_int64 = other.m_int64;
        break;
      case Bridge::JSValueType::Double:
        m_double = other.m_double;
        break;
    }

    other.m_type = Bridge::JSValueType::Null;
    other.m_int64 = 0;
  }

  JSValue &operator=(JSValue &&other) noexcept {
    if (this != &other) {
      this->~JSValue();
      new (this) JSValue(std::move(other));
    }

    return *this;
  }

  JSValue(const JSValue &other) = delete;
  JSValue &operator=(const JSValue &other) = delete;

  ~JSValue() noexcept {
    switch (m_type) {
      case Bridge::JSValueType::Object:
        m_object.~map();
        break;
      case Bridge::JSValueType::Array:
        m_array.~vector();
        break;
      case Bridge::JSValueType::String:
        m_string.~basic_string();
        break;
    }

    m_int64 = 0;
    m_type = Bridge::JSValueType::Null;
  }

  Bridge::JSValueType Type() const noexcept {
    return m_type;
  }

  bool IsNull() const noexcept {
    return m_type == Bridge::JSValueType::Null;
  }

  const JSValueObject &Object() const noexcept {
    VerifyElseCrash(m_type == Bridge::JSValueType::Object);
    return m_object;
  }

  const JSValueArray &Array() const noexcept {
    VerifyElseCrash(m_type == Bridge::JSValueType::Array);
    return m_array;
  }

  const std::string &String() const noexcept {
    VerifyElseCrash(m_type == Bridge::JSValueType::String);
    return m_string;
  }

  bool Boolean() const noexcept {
    VerifyElseCrash(m_type == Bridge::JSValueType::Boolean);
    return m_bool;
  }

  int64_t Int64() const noexcept {
    VerifyElseCrash(m_type == Bridge::JSValueType::Int64);
    return m_int64;
  }

  double Double() const noexcept {
    VerifyElseCrash(m_type == Bridge::JSValueType::Double);
    return m_double;
  }

  // T To<T>() = > (new JSValueTreeReader(this)).ReadValue<T>();

  // JSValue From<T>(T value) {
  //   var writer = new JSValueTreeWriter();
  //   writer.WriteValue(value);
  //   return writer.TakeValue();
  // }

  const JSValue *GetObjectProperty(std::string_view propertyName) {
    if (m_type == Bridge::JSValueType::Object) {
      auto it = m_object.find(propertyName);
      if (it != m_object.end()) {
        return &(it->second);
      }
    }

    return nullptr;
  }

  bool Equals(const JSValue &other) const noexcept {
    if (m_type == other.m_type) {
      switch (m_type) {
        case Bridge::JSValueType::Null:
          return true;
        case Bridge::JSValueType::Object:
          return ObjectEquals(m_object);
        case Bridge::JSValueType::Array:
          return ArrayEquals(m_array);
        case Bridge::JSValueType::String:
          return m_string == other.m_string;
        case Bridge::JSValueType::Boolean:
          return m_bool == other.m_bool;
        case Bridge::JSValueType::Int64:
          return m_int64 == other.m_int64;
        case Bridge::JSValueType::Double:
          return m_double == m_double;
        default:
          return false;
      }
    }

    return false;
  }

   //static JSValue ReadFrom(IJSValueReader& reader) {
   //  auto treeReader = reader.try_ as IJSValueTreeReader;
   //  if (treeReader != null) {
   //    return treeReader.Current;
   //  }

   //  return ReadValue(reader);
   //}

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

 private:
  static JSValue ReadValue(Bridge::IJSValueReader &reader) noexcept {
    switch (reader.ValueType()) {
      case Bridge::JSValueType::Null:
        return JSValue();
      case Bridge::JSValueType::Object:
        return ReadObject(reader);
      case Bridge::JSValueType::Array:
        return ReadArray(reader);
      case Bridge::JSValueType::String:
        return new JSValue(to_string(reader.GetString()));
      case Bridge::JSValueType::Boolean:
        return new JSValue(reader.GetBoolean());
      case Bridge::JSValueType::Int64:
        return new JSValue(reader.GetInt64());
      case Bridge::JSValueType::Double:
        return new JSValue(reader.GetDouble());
      default:
        VerifyElseCrashSz(false, "Unexpected JSValueType");
    }
  }

  static JSValue ReadObject(Bridge::IJSValueReader &reader) noexcept {
    return new JSValue(ReadObjectProperties(reader));
  }

  static JSValue ReadArray(Bridge::IJSValueReader &reader) noexcept {
    return new JSValue(ReadArrayItems(reader));
  }

  static JSValueObject ReadObjectProperties(Bridge::IJSValueReader &reader) noexcept {
    JSValueObject properties;
    hstring propertyName;
    while (reader.GetNextObjectProperty(/*ref*/ propertyName)) {
      properties[to_string(propertyName)] = ReadValue(reader);
    }

    return properties;
  }

  static JSValueArray ReadArrayItems(Bridge::IJSValueReader &reader) noexcept {
    JSValueArray items;
    while (reader.GetNextArrayItem()) {
      items.push_back(ReadValue(reader));
    }

    return items;
  }

 public:
  void WriteTo(Bridge::IJSValueWriter &writer) const noexcept {
    switch (m_type) {
      case Bridge::JSValueType::Null:
        writer.WriteNull();
        break;
      case Bridge::JSValueType::Object:
        WriteObject(writer, Object());
        break;
      case Bridge::JSValueType::Array:
        WriteArray(writer, Array());
        break;
      case Bridge::JSValueType::String:
        writer.WriteString(to_hstring(String()));
        break;
      case Bridge::JSValueType::Boolean:
        writer.WriteBoolean(Boolean());
        break;
      case Bridge::JSValueType::Int64:
        writer.WriteInt64(Int64());
        break;
      case Bridge::JSValueType::Double:
        writer.WriteDouble(Double());
        break;
    }
  }

  static void WriteObject(Bridge::IJSValueWriter &writer, JSValueObjectView value) noexcept {
    writer.WriteObjectBegin();
    for (const auto &keyValue : value) {
      writer.WritePropertyName(to_hstring(keyValue.first));
      keyValue.second.WriteTo(writer);
    }
    writer.WriteObjectEnd();
  }

  static void WriteArray(Bridge::IJSValueWriter &writer, JSValueArrayView value) noexcept {
    writer.WriteArrayBegin();
    for (const JSValue &item : value) {
      item.WriteTo(writer);
    }
    writer.WriteArrayEnd();
  }

 private:
  bool ObjectEquals(const JSValueObject &other) const noexcept {
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

  bool ArrayEquals(const JSValueArray &other) const noexcept {
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

 private:
  Bridge::JSValueType m_type;
  union {
    JSValueObject m_object;
    JSValueArray m_array;
    std::string m_string;
    bool m_bool;
    int64_t m_int64;
    double m_double;
  };
};

bool operator==(const JSValue &left, const JSValue &right) noexcept {
  return left.Equals(right);
}

bool operator!=(const JSValue &left, const JSValue &right) noexcept {
  return !left.Equals(right);
}

void swap(JSValue &left, JSValue &right) noexcept {
  JSValue temp{std::move(right)};
  right = std::move(left);
  left = std::move(temp);
}

} // namespace winrt::Microsoft::ReactNative

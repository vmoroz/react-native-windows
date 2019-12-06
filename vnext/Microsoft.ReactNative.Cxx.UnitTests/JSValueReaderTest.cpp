// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "pch.h"
#include "JSValueReader.h"
#include <variant>
#include "JSValueWriter.h"
#include "JsonJSValueReader.h"
#include "catch.hpp"

namespace winrt::Microsoft::ReactNative::Bridge {

enum struct RobotModel {
  T2,
  R2D2,
  C3PO,
};

// We are generating ReadValue for it.
enum struct RobotShape {
  Humanoid,
  Trashcan,
  Beercan,
  Quadrocopter,
};

struct RobotTool {
  std::string Name;
  int Weight;
  bool IsEnabled;
};

// This is how we can provide "reflection" information about type outside of struct.
FieldMap GetStructInfo(RobotTool *) {
  return {{L"Name", &RobotTool::Name}, {L"Weight", &RobotTool::Weight}, {L"IsEnabled", &RobotTool::IsEnabled}};
}

struct RobotPoint {
  int X;
  int Y;
};

FieldMap GetStructInfo(RobotPoint *) {
  return {{L"X", &RobotPoint::X}, {L"Y", &RobotPoint::Y}};
}

// We can provide "reflection" information about struct using REACT_STRUCT and REACT_FIELD macros.
REACT_STRUCT(T2Extra)
struct T2Extra {
  REACT_FIELD(ActorName)
  std::string ActorName;

  REACT_FIELD(MovieYear)
  int MovieYear;
};

REACT_STRUCT(R2D2Extra)
struct R2D2Extra {
  REACT_FIELD(MovieSeries)
  std::string MovieSeries;
};

struct RobotInfo {
  RobotModel Model;
  std::string Name;
  int Age;
  RobotShape Shape;
  std::optional<RobotShape> Shape2;
  std::optional<RobotShape> Shape3;
  std::vector<int> Steps;
  std::map<std::string, int> Dimensions;
  std::tuple<int, std::string, bool> Badges;
  std::vector<RobotTool> Tools;
  std::vector<RobotPoint> Path;
  std::variant<T2Extra, R2D2Extra> Extra;
};

// Reading RobotModel enum value. We could use template-based version instead.
void ReadValue(IJSValueReader &reader, RobotModel &value) noexcept {
  value = static_cast<RobotModel>(ReadValue<int>(reader));
}

// Writing RobotModel enum value. We could use template-based version instead.
void WriteValue(IJSValueWriter const &writer, RobotModel value) noexcept {
  WriteValue(writer, static_cast<int>(value));
}

// Reading discriminating union requires using JSValue.
void ReadValue(const JSValue &jsValue, std::variant<T2Extra, R2D2Extra> &value) noexcept {
  switch (ReadValue<RobotModel>(jsValue["Kind"])) {
    case RobotModel::T2:
      value = ReadValue<T2Extra>(jsValue);
      break;
    case RobotModel::R2D2:
      value = ReadValue<R2D2Extra>(jsValue);
      break;
  }
}

// Writing discriminating union.
void WriteValue(IJSValueWriter const &writer, std::variant<T2Extra, R2D2Extra> const &value) noexcept {
  writer.WriteObjectBegin();
  if (const T2Extra *t2 = std::get_if<T2Extra>(&value)) {
    WriteProperty(writer, L"Kind", RobotModel::T2);
    WriteProperties(writer, *t2);
  } else if (const R2D2Extra *r2d2 = std::get_if<R2D2Extra>(&value)) {
    WriteProperty(writer, L"Kind", RobotModel::R2D2);
    WriteProperties(writer, *r2d2);
  }
  writer.WriteObjectEnd();
}

// Reading RobotInfo value. It could be generated instead.
void ReadValue(IJSValueReader &reader, RobotInfo &value) noexcept {
  value = RobotInfo();
  if (reader.ValueType() == JSValueType::Object) {
    hstring propertyName;
    while (reader.GetNextObjectProperty(/*out*/ propertyName)) {
      if (propertyName == L"Model")
        ReadValue(reader, value.Model);
      else if (propertyName == L"Name")
        ReadValue(reader, value.Name);
      else if (propertyName == L"Age")
        ReadValue(reader, value.Age);
      else if (propertyName == L"Shape")
        ReadValue(reader, value.Shape);
      else if (propertyName == L"Shape2")
        ReadValue(reader, value.Shape2);
      else if (propertyName == L"Shape3")
        ReadValue(reader, value.Shape3);
      else if (propertyName == L"Steps")
        ReadValue(reader, value.Steps);
      else if (propertyName == L"Dimensions")
        ReadValue(reader, value.Dimensions);
      else if (propertyName == L"Badges")
        ReadValue(reader, value.Badges);
      else if (propertyName == L"Tools")
        ReadValue(reader, value.Tools);
      else if (propertyName == L"Path")
        ReadValue(reader, value.Path);
      else if (propertyName == L"Extra")
        ReadValue(reader, value.Extra);
      else
        ReadValue<JSValue>(reader);
    }
  }
}

// Writing RobotInfo value. It could be generated instead.
void WriteValue(IJSValueWriter const &writer, RobotInfo const & /*value*/) noexcept {
  writer.WriteObjectBegin();
  // WriteValue(writer, L"Model", value.Model);
  // WriteValue(writer, L"Name", value.Name);
  // WriteValue(writer, L"Age", value.Age);
  // WriteValue(writer, L"Shape", value.Shape);
  // WriteValue(writer, L"Shape2", value.Shape2);
  // WriteValue(writer, L"Shape3", value.Shape3);
  // WriteValue(writer, L"Steps", value.Steps);
  // WriteValue(writer, L"Dimensions", value.Dimensions);
  // WriteValue(writer, L"Badges", value.Badges);
  // WriteValue(writer, L"Tools", value.Tools);
  // WriteValue(writer, L"Path", value.Path);
  // WriteValue(writer, L"Extra", value.Extra);
  writer.WriteObjectEnd();
}

TEST_CASE("TestReadCustomType", "JSValueReaderTest") {
  const wchar_t *json =
      LR"JSON({
        "Model": 1,
        "Name": "Bob",
        "Age": 42,
        "Shape": 1,
        "Shape2": 2,
        "Shape3": null,
        "Steps": [1, 2, 3],
        "Dimensions": {"Width": 24, "Height": 78},
        "Badges": [2, "Maverick", true],
        "Tools": [
          {"Name": "Screwdriver", "Weight": 2, "IsEnabled": true},
          {"Name": "Electro-shocker", "Weight": 3, "IsEnabled": false}],
        "Path": [{"X": 5, "Y": 6}, {"X": 45, "Y": 90}, {"X": 15, "Y": 16}],
        "Extra": {"Kind": 1, "MovieSeries" : "Episode 2"}
    })JSON";

  IJSValueReader reader = make<JsonJSValueReader>(json);

  RobotInfo robot = ReadValue<RobotInfo>(reader);
  REQUIRE(robot.Model == RobotModel::R2D2);
  REQUIRE(robot.Name == "Bob");
  REQUIRE(robot.Age == 42);
  REQUIRE(robot.Shape == RobotShape::Trashcan);
  REQUIRE(*robot.Shape2 == RobotShape::Beercan);
  REQUIRE(!robot.Shape3.has_value());
  REQUIRE(robot.Steps.size() == 3);
  REQUIRE(robot.Steps[0] == 1);
  REQUIRE(robot.Steps[1] == 2);
  REQUIRE(robot.Steps[2] == 3);
  REQUIRE(robot.Dimensions.size() == 2);
  REQUIRE(robot.Dimensions["Width"] == 24);
  REQUIRE(robot.Dimensions["Height"] == 78);
  REQUIRE(std::get<0>(robot.Badges) == 2);
  REQUIRE(std::get<1>(robot.Badges) == "Maverick");
  REQUIRE(std::get<2>(robot.Badges) == true);
  REQUIRE(robot.Tools.size() == 2);
  REQUIRE(robot.Tools[0].Name == "Screwdriver");
  REQUIRE(robot.Tools[0].Weight == 2);
  REQUIRE(robot.Tools[0].IsEnabled == true);
  REQUIRE(robot.Tools[1].Name == "Electro-shocker");
  REQUIRE(robot.Tools[1].Weight == 3);
  REQUIRE(robot.Tools[1].IsEnabled == false);
  REQUIRE(robot.Path.size() == 3);
  REQUIRE(robot.Path[0].X == 5);
  REQUIRE(robot.Path[0].Y == 6);
  REQUIRE(robot.Path[1].X == 45);
  REQUIRE(robot.Path[1].Y == 90);
  REQUIRE(robot.Path[2].X == 15);
  REQUIRE(robot.Path[2].Y == 16);
  const R2D2Extra *r2d2Extra = std::get_if<R2D2Extra>(&robot.Extra);
  REQUIRE(r2d2Extra != nullptr);
  REQUIRE(r2d2Extra->MovieSeries == "Episode 2");
}

//
//      [TestMethod] public void
//      TestWriteCustomType() {
//    var robot =
//        new RobotInfo{Model = RobotModel.R2D2,
//                      Name = "Bob",
//                      Age = 42,
//                      Shape = RobotShape.Trashcan,
//                      Shape2 = RobotShape.Beercan,
//                      Shape3 = null,
//                      Steps = new List<int>{1, 2, 3},
//                      Dimensions = new Dictionary<string, int>{["Width"] = 24, ["Height"] = 78},
//                      Badges = Tuple.Create(2, "Maverick", true),
//                      Tools = new RobotTool[]{new RobotTool{Name = "Screwdriver", Weight = 2, IsEnabled = true},
//                                              new RobotTool{Name = "Electro-shocker", Weight = 3, IsEnabled = false}},
//                      Path = new RobotPoint[]{new RobotPoint{X = 5, Y = 6},
//                                              new RobotPoint{X = 45, Y = 90},
//                                              new RobotPoint{X = 15, Y = 16}},
//                      Extra = new R2D2Extra{MovieSeries = "Episode 2"}};
//
//    var writer = new JTokenJSValueWriter();
//    writer.WriteValue(robot);
//    JToken jValue = writer.TakeValue();
//
//    Assert.AreEqual((int)RobotModel.R2D2, jValue["Model"]);
//    Assert.AreEqual("Bob", jValue["Name"]);
//    Assert.AreEqual(42, jValue["Age"]);
//    Assert.AreEqual((int)RobotShape.Trashcan, jValue["Shape"]);
//    Assert.AreEqual((int)RobotShape.Beercan, jValue["Shape2"]);
//    Assert.AreEqual(JTokenType.Null, jValue["Shape3"].Type);
//    Assert.AreEqual(3, (jValue["Steps"] as JArray).Count);
//    Assert.AreEqual(1, jValue["Steps"][0]);
//    Assert.AreEqual(2, jValue["Steps"][1]);
//    Assert.AreEqual(3, jValue["Steps"][2]);
//    Assert.AreEqual(2, (jValue["Dimensions"] as JObject).Count);
//    Assert.AreEqual(24, jValue["Dimensions"]["Width"]);
//    Assert.AreEqual(78, jValue["Dimensions"]["Height"]);
//    Assert.AreEqual(2, jValue["Badges"][0]);
//    Assert.AreEqual("Maverick", jValue["Badges"][1]);
//    Assert.AreEqual(true, jValue["Badges"][2]);
//    Assert.AreEqual(2, (jValue["Tools"] as JArray).Count);
//    Assert.AreEqual("Screwdriver", jValue["Tools"][0]["Name"]);
//    Assert.AreEqual(2, jValue["Tools"][0]["Weight"]);
//    Assert.AreEqual(true, jValue["Tools"][0]["IsEnabled"]);
//    Assert.AreEqual("Electro-shocker", jValue["Tools"][1]["Name"]);
//    Assert.AreEqual(3, jValue["Tools"][1]["Weight"]);
//    Assert.AreEqual(false, jValue["Tools"][1]["IsEnabled"]);
//    Assert.AreEqual(3, (jValue["Path"] as JArray).Count);
//    Assert.AreEqual(5, jValue["Path"][0]["X"]);
//    Assert.AreEqual(6, jValue["Path"][0]["Y"]);
//    Assert.AreEqual(45, jValue["Path"][1]["X"]);
//    Assert.AreEqual(90, jValue["Path"][1]["Y"]);
//    Assert.AreEqual(15, jValue["Path"][2]["X"]);
//    Assert.AreEqual(16, jValue["Path"][2]["Y"]);
//    Assert.AreEqual("Episode 2", jValue["Extra"]["MovieSeries"]);
//  }

TEST_CASE("TestReadValueDefaultExtensions", "JSValueReaderTest") {
  const wchar_t *json =
      LR"JSON({
      "StringValue1": "",
      "StringValue2": "5",
      "StringValue3": "Hello",
      "BoolValue1": false,
      "BoolValue2": true,
      "IntValue1": 0,
      "IntValue2": 42,
      "FloatValue": 3.14,
      "NullValue": null
    })JSON";

  IJSValueReader reader = make<JsonJSValueReader>(json);

  REQUIRE(reader.ValueType() == JSValueType::Object);
  int properyCount = 0;
  hstring propertyName;
  while (reader.GetNextObjectProperty(/*out*/ propertyName)) {
    if (propertyName == L"StringValue1") {
      REQUIRE(ReadValue<std::string>(reader) == "");
      REQUIRE(ReadValue<std::wstring>(reader) == L"");
      REQUIRE(ReadValue<bool>(reader) == false);
      REQUIRE(ReadValue<int8_t>(reader) == 0);
      REQUIRE(ReadValue<int16_t>(reader) == 0);
      REQUIRE(ReadValue<int32_t>(reader) == 0);
      REQUIRE(ReadValue<int64_t>(reader) == 0);
      REQUIRE(ReadValue<uint8_t>(reader) == 0);
      REQUIRE(ReadValue<uint16_t>(reader) == 0);
      REQUIRE(ReadValue<uint32_t>(reader) == 0);
      REQUIRE(ReadValue<uint64_t>(reader) == 0);
      REQUIRE(ReadValue<float>(reader) == 0);
      REQUIRE(ReadValue<double>(reader) == 0);
      ++properyCount;
    } else if (propertyName == L"StringValue2") {
      REQUIRE(ReadValue<std::string>(reader) == "5");
      REQUIRE(ReadValue<std::wstring>(reader) == L"5");
      REQUIRE(ReadValue<bool>(reader) == true);
      REQUIRE(ReadValue<int8_t>(reader) == 5);
      REQUIRE(ReadValue<int16_t>(reader) == 5);
      REQUIRE(ReadValue<int32_t>(reader) == 5);
      REQUIRE(ReadValue<int64_t>(reader) == 5);
      REQUIRE(ReadValue<uint8_t>(reader) == 5);
      REQUIRE(ReadValue<uint16_t>(reader) == 5);
      REQUIRE(ReadValue<uint32_t>(reader) == 5);
      REQUIRE(ReadValue<uint64_t>(reader) == 5);
      REQUIRE(ReadValue<float>(reader) == 5);
      REQUIRE(ReadValue<double>(reader) == 5);
      ++properyCount;
    } else if (propertyName == L"StringValue3") {
      REQUIRE(ReadValue<std::string>(reader) == "Hello");
      REQUIRE(ReadValue<std::wstring>(reader) == L"Hello");
      REQUIRE(ReadValue<bool>(reader) == true);
      REQUIRE(ReadValue<int8_t>(reader) == 0);
      REQUIRE(ReadValue<int16_t>(reader) == 0);
      REQUIRE(ReadValue<int32_t>(reader) == 0);
      REQUIRE(ReadValue<int64_t>(reader) == 0);
      REQUIRE(ReadValue<uint8_t>(reader) == 0);
      REQUIRE(ReadValue<uint16_t>(reader) == 0);
      REQUIRE(ReadValue<uint32_t>(reader) == 0);
      REQUIRE(ReadValue<uint64_t>(reader) == 0);
      REQUIRE(ReadValue<float>(reader) == 0);
      REQUIRE(ReadValue<double>(reader) == 0);
      ++properyCount;
    } else if (propertyName == L"BoolValue1") {
      REQUIRE(ReadValue<std::string>(reader) == "false");
      REQUIRE(ReadValue<std::wstring>(reader) == L"false");
      REQUIRE(ReadValue<bool>(reader) == false);
      REQUIRE(ReadValue<int8_t>(reader) == 0);
      REQUIRE(ReadValue<int16_t>(reader) == 0);
      REQUIRE(ReadValue<int32_t>(reader) == 0);
      REQUIRE(ReadValue<int64_t>(reader) == 0);
      REQUIRE(ReadValue<uint8_t>(reader) == 0);
      REQUIRE(ReadValue<uint16_t>(reader) == 0);
      REQUIRE(ReadValue<uint32_t>(reader) == 0);
      REQUIRE(ReadValue<uint64_t>(reader) == 0);
      REQUIRE(ReadValue<float>(reader) == 0);
      REQUIRE(ReadValue<double>(reader) == 0);
      ++properyCount;
    } else if (propertyName == L"BoolValue2") {
      REQUIRE(ReadValue<std::string>(reader) == "true");
      REQUIRE(ReadValue<std::wstring>(reader) == L"true");
      REQUIRE(ReadValue<bool>(reader) == true);
      REQUIRE(ReadValue<int8_t>(reader) == 1);
      REQUIRE(ReadValue<int16_t>(reader) == 1);
      REQUIRE(ReadValue<int32_t>(reader) == 1);
      REQUIRE(ReadValue<int64_t>(reader) == 1);
      REQUIRE(ReadValue<uint8_t>(reader) == 1);
      REQUIRE(ReadValue<uint16_t>(reader) == 1);
      REQUIRE(ReadValue<uint32_t>(reader) == 1);
      REQUIRE(ReadValue<uint64_t>(reader) == 1);
      REQUIRE(ReadValue<float>(reader) == 1);
      REQUIRE(ReadValue<double>(reader) == 1);
      ++properyCount;
    } else if (propertyName == L"IntValue1") {
      REQUIRE(ReadValue<std::string>(reader) == "0");
      REQUIRE(ReadValue<std::wstring>(reader) == L"0");
      REQUIRE(ReadValue<bool>(reader) == false);
      REQUIRE(ReadValue<int8_t>(reader) == 0);
      REQUIRE(ReadValue<int16_t>(reader) == 0);
      REQUIRE(ReadValue<int32_t>(reader) == 0);
      REQUIRE(ReadValue<int64_t>(reader) == 0);
      REQUIRE(ReadValue<uint8_t>(reader) == 0);
      REQUIRE(ReadValue<uint16_t>(reader) == 0);
      REQUIRE(ReadValue<uint32_t>(reader) == 0);
      REQUIRE(ReadValue<uint64_t>(reader) == 0);
      REQUIRE(ReadValue<float>(reader) == 0);
      REQUIRE(ReadValue<double>(reader) == 0);
      ++properyCount;
    } else if (propertyName == L"IntValue2") {
      REQUIRE(ReadValue<std::string>(reader) == "42");
      REQUIRE(ReadValue<std::wstring>(reader) == L"42");
      REQUIRE(ReadValue<bool>(reader) == true);
      REQUIRE(ReadValue<int8_t>(reader) == 42);
      REQUIRE(ReadValue<int16_t>(reader) == 42);
      REQUIRE(ReadValue<int32_t>(reader) == 42);
      REQUIRE(ReadValue<int64_t>(reader) == 42);
      REQUIRE(ReadValue<uint8_t>(reader) == 42);
      REQUIRE(ReadValue<uint16_t>(reader) == 42);
      REQUIRE(ReadValue<uint32_t>(reader) == 42);
      REQUIRE(ReadValue<uint64_t>(reader) == 42);
      REQUIRE(ReadValue<float>(reader) == 42);
      REQUIRE(ReadValue<double>(reader) == 42);
      ++properyCount;
    } else if (propertyName == L"FloatValue") {
      REQUIRE(ReadValue<std::string>(reader) == "3.14");
      REQUIRE(ReadValue<std::wstring>(reader) == L"3.14");
      REQUIRE(ReadValue<bool>(reader) == true);
      REQUIRE(ReadValue<int8_t>(reader) == 3);
      REQUIRE(ReadValue<int16_t>(reader) == 3);
      REQUIRE(ReadValue<int32_t>(reader) == 3);
      REQUIRE(ReadValue<int64_t>(reader) == 3);
      REQUIRE(ReadValue<uint8_t>(reader) == 3);
      REQUIRE(ReadValue<uint16_t>(reader) == 3);
      REQUIRE(ReadValue<uint32_t>(reader) == 3);
      REQUIRE(ReadValue<uint64_t>(reader) == 3);
      REQUIRE(ReadValue<float>(reader) == 3.14f);
      REQUIRE(ReadValue<double>(reader) == 3.14);
      ++properyCount;
    } else if (propertyName == L"NullValue") {
      REQUIRE(ReadValue<std::string>(reader) == "");
      REQUIRE(ReadValue<std::wstring>(reader) == L"");
      REQUIRE(ReadValue<bool>(reader) == false);
      REQUIRE(ReadValue<int8_t>(reader) == 0);
      REQUIRE(ReadValue<int16_t>(reader) == 0);
      REQUIRE(ReadValue<int32_t>(reader) == 0);
      REQUIRE(ReadValue<int64_t>(reader) == 0);
      REQUIRE(ReadValue<uint8_t>(reader) == 0);
      REQUIRE(ReadValue<uint16_t>(reader) == 0);
      REQUIRE(ReadValue<uint32_t>(reader) == 0);
      REQUIRE(ReadValue<uint64_t>(reader) == 0);
      REQUIRE(ReadValue<float>(reader) == 0);
      REQUIRE(ReadValue<double>(reader) == 0);

      ++properyCount;
    }
  }
  REQUIRE(properyCount == 9);
}

TEST_CASE("TestWriteValueDefaultExtensions", "JSValueReaderTest") {
  JSValue jsValue;
  auto writer = MakeJSValueTreeWriter(jsValue);
  writer.WriteObjectBegin();
  WriteProperty(writer, L"StringValue1", "");
  WriteProperty(writer, L"StringValue2", "5");
  WriteProperty(writer, L"StringValue3", "Hello");
  WriteProperty(writer, L"BoolValue1", false);
  WriteProperty(writer, L"BoolValue2", true);
  WriteProperty(writer, L"IntValue1", 0);
  WriteProperty(writer, L"IntValue2", 42);
  WriteProperty(writer, L"FloatValue", 3.14);
  WriteProperty(writer, L"NullValue", nullptr);
  writer.WriteObjectEnd();
  // JSValue jsValue = writer.TakeValue();

  // REQUIRE("", jsValue["StringValue1"].String());
  // REQUIRE("5", jsValue["StringValue2"]);
  // REQUIRE("Hello", jsValue["StringValue3"]);
  // REQUIRE(false, jsValue["BoolValue1"]);
  // REQUIRE(true, jsValue["BoolValue2"]);
  // REQUIRE(0, jsValue["IntValue1"]);
  // REQUIRE(42, jsValue["IntValue2"]);
  // REQUIRE(3.14, jsValue["FloatValue"]);
  // REQUIRE(JTokenType.Null, jsValue["NullValue"].Type);
}

} // namespace winrt::Microsoft::ReactNative::Bridge

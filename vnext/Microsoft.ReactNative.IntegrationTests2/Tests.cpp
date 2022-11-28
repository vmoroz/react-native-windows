// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include <ReactObject.h>

namespace ReactNativeIntegrationTests {

TEST_CLASS(MyTests2) {
  TEST_METHOD(SimpleTest2) {
    react::ReactObject obj;
    TestCheck((bool) obj);
  }
};

} // namespace ReactNativeIntegrationTests

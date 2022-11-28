// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include <ReactObject.h>

namespace ReactNativeIntegrationTests {

TEST_CLASS(MyTests) {
  TEST_METHOD(SimpleTest) {
    react::ReactObject obj;
    TestCheck((bool) obj);
  }
};

} // namespace ReactNativeIntegrationTests

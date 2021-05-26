// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

import { NativeModules } from 'react-native';

// The test class which methods are called from C++ code.
class TestDriver {
  testDefaultDispatchedModule() {
    const { DefaultDispatchedModule } = NativeModules;
    const myConst = DefaultDispatchedModule.myConst;
    DefaultDispatchedModule.testAsyncMethod(myConst);
    DefaultDispatchedModule.testSyncMethod(myConst);
  }

  testUIDispatchedModule() {
    const { UIDispatchedModule } = NativeModules;
    const myConst = UIDispatchedModule.myConst;
    UIDispatchedModule.testAsyncMethod(myConst);
    UIDispatchedModule.testSyncMethod(myConst);
  }

  testJSDispatchedModule() {
    const { JSDispatchedModule } = NativeModules;
    const myConst = JSDispatchedModule.myConst;
    JSDispatchedModule.testAsyncMethod(myConst);
    JSDispatchedModule.testSyncMethod(myConst);
  }

  testCustomDispatchedModule() {
    const { CustomDispatchedModule } = NativeModules;
    const myConst = CustomDispatchedModule.myConst;
    CustomDispatchedModule.testAsyncMethod(myConst);
    CustomDispatchedModule.testSyncMethod(myConst);
  }
}

global.__fbBatchedBridge.registerLazyCallableModule('TestDriver', () => new TestDriver());

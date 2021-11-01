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

  testUIDispatchedModule2() {
    const { UIDispatchedModule2 } = NativeModules;
    const myConst = UIDispatchedModule2.myConst;
    UIDispatchedModule2.testAsyncMethod(myConst);
    UIDispatchedModule2.testSyncMethod(myConst);
  }

  testJSDispatchedModule2() {
    const { JSDispatchedModule2 } = NativeModules;
    const myConst = JSDispatchedModule2.myConst;
    JSDispatchedModule2.testAsyncMethod(myConst);
    JSDispatchedModule2.testSyncMethod(myConst);
  }

  testCustomDispatchedModule2() {
    const { CustomDispatchedModule2 } = NativeModules;
    const myConst = CustomDispatchedModule2.myConst;
    CustomDispatchedModule2.testAsyncMethod(myConst);
    CustomDispatchedModule2.testSyncMethod(myConst);
  }

  testUIDispatchedModule3() {
    const { UIDispatchedModule3 } = NativeModules;
    const myJSConst = UIDispatchedModule3.myJSConst;
    const myUIConst = UIDispatchedModule3.myUIConst;
    UIDispatchedModule3.testJSAsyncMethod(myJSConst);
    UIDispatchedModule3.testUIAsyncMethod(myUIConst);
    UIDispatchedModule3.testJSSyncMethod(myJSConst);
    UIDispatchedModule3.testUISyncMethod(myUIConst);
  }

  testCustomDispatchedModule3() {
    const { CustomDispatchedModule3 } = NativeModules;
    const myJSConst = CustomDispatchedModule3.myJSConst;
    const myCustomConst = CustomDispatchedModule3.myCustomConst;
    CustomDispatchedModule3.testJSAsyncMethod(myJSConst);
    CustomDispatchedModule3.testCustomAsyncMethod(myCustomConst);
    CustomDispatchedModule3.testJSSyncMethod(myJSConst);
    CustomDispatchedModule3.testCustomSyncMethod(myCustomConst);
  }
}

global.__fbBatchedBridge.registerLazyCallableModule('TestDriver', () => new TestDriver());

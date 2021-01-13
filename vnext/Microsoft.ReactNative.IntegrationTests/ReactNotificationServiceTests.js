import { NativeModules } from 'react-native';

// Accessing NotificationTestModule has a side effect of initializing global.__fbBatchedBridge
if (NativeModules.NotificationTestModule) {
  // Native modules are created on demand from JavaScript code.
  NativeModules.NotificationTestModule.start();
}

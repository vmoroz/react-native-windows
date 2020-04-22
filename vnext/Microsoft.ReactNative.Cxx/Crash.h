// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#pragma once

#ifndef VerifyElseCrash
#define VerifyElseCrash(condition) \
  do {                             \
    if (!(condition)) {            \
      assert(false && #condition); \
      *((int *)0) = 1;             \
      std::terminate();            \
    }                              \
  } while (false)
#endif

#ifndef VerifyElseCrashSz
#define VerifyElseCrashSz(condition, message) \
  do {                                        \
    if (!(condition)) {                       \
      assert(false && (message));             \
      *((int *)0) = 1;                        \
      std::terminate();                       \
    }                                         \
  } while (false)
#endif

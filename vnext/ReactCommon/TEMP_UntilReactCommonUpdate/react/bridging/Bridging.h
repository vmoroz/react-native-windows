/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */

#pragma once

// #define USE_FOLLY_DYNAMIC

#include <react/bridging/AString.h>
#include <react/bridging/Array.h>
#include <react/bridging/Bool.h>
#include <react/bridging/Class.h>
#ifdef USE_FOLLY_DYNAMIC
// #include <react/bridging/Dynamic.h>
#endif
#include <react/bridging/Error.h>
#include <react/bridging/Function.h>
#include <react/bridging/Number.h>
#include <react/bridging/Object.h>
#include <react/bridging/Promise.h>
#include <react/bridging/Value.h>
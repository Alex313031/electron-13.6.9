// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NODE_UTIL_H_
#define SHELL_COMMON_NODE_UTIL_H_

#include <vector>

#include "v8/include/v8.h"

namespace node {
class Environment;
}  // namespace node

namespace electron {

namespace util {

// Run a script with JS source bundled inside the binary as if it's wrapped
// in a function called with a null receiver and arguments specified in C++.
// The returned value is empty if an exception is encountered.
// JS code run with this method can assume that their top-level
// declarations won't affect the global scope.
v8::MaybeLocal<v8::Value> CompileAndCall(
    v8::Local<v8::Context> context,
    const char* id,
    std::vector<v8::Local<v8::String>>* parameters,
    std::vector<v8::Local<v8::Value>>* arguments,
    node::Environment* optional_env);

}  // namespace util

}  // namespace electron

#endif  // SHELL_COMMON_NODE_UTIL_H_

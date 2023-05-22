// Copyright (c) 2013 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NODE_BINDINGS_MAC_H_
#define SHELL_COMMON_NODE_BINDINGS_MAC_H_

#include "base/compiler_specific.h"
#include "shell/common/node_bindings.h"

namespace electron {

class NodeBindingsMac : public NodeBindings {
 public:
  explicit NodeBindingsMac(BrowserEnvironment browser_env);
  ~NodeBindingsMac() override;

  void RunMessageLoop() override;

 private:
  // Called when uv's watcher queue changes.
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  void PollEvents() override;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsMac);
};

}  // namespace electron

#endif  // SHELL_COMMON_NODE_BINDINGS_MAC_H_

// Copyright (c) 2014 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_COMMON_NODE_BINDINGS_LINUX_H_
#define SHELL_COMMON_NODE_BINDINGS_LINUX_H_

#include "base/compiler_specific.h"
#include "shell/common/node_bindings.h"

namespace electron {

class NodeBindingsLinux : public NodeBindings {
 public:
  explicit NodeBindingsLinux(BrowserEnvironment browser_env);
  ~NodeBindingsLinux() override;

  void RunMessageLoop() override;

 private:
  // Called when uv's watcher queue changes.
  static void OnWatcherQueueChanged(uv_loop_t* loop);

  void PollEvents() override;

  // Epoll to poll for uv's backend fd.
  int epoll_;

  DISALLOW_COPY_AND_ASSIGN(NodeBindingsLinux);
};

}  // namespace electron

#endif  // SHELL_COMMON_NODE_BINDINGS_LINUX_H_

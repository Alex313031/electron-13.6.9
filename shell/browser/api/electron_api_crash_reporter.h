// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_
#define SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_

#include <map>
#include <string>
#include "base/files/file_path.h"

namespace electron {

namespace api {

namespace crash_reporter {

bool IsCrashReporterEnabled();

#if defined(OS_LINUX)
const std::map<std::string, std::string>& GetGlobalCrashKeys();
std::string GetClientId();
#endif

// JS bindings API; exposed publicly because it's also called from node_main.cc
void Start(const std::string& submit_url,
           bool upload_to_server,
           bool ignore_system_crash_handler,
           bool rate_limit,
           bool compress,
           const std::map<std::string, std::string>& global_extra,
           const std::map<std::string, std::string>& extra,
           bool is_node_process);

}  // namespace crash_reporter

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_CRASH_REPORTER_H_

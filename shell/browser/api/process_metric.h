// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_PROCESS_METRIC_H_
#define SHELL_BROWSER_API_PROCESS_METRIC_H_

#include <memory>
#include <string>

#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/process/process_metrics.h"

namespace electron {

#if !defined(OS_LINUX)
struct ProcessMemoryInfo {
  size_t working_set_size = 0;
  size_t peak_working_set_size = 0;
#if defined(OS_WIN)
  size_t private_bytes = 0;
#endif
};
#endif

#if defined(OS_WIN)
enum class ProcessIntegrityLevel {
  kUnknown,
  kUntrusted,
  kLow,
  kMedium,
  kHigh,
};
#endif

struct ProcessMetric {
  int type;
  base::Process process;
  std::unique_ptr<base::ProcessMetrics> metrics;
  std::string service_name;
  std::string name;

  ProcessMetric(int type,
                base::ProcessHandle handle,
                std::unique_ptr<base::ProcessMetrics> metrics,
                const std::string& service_name = std::string(),
                const std::string& name = std::string());
  ~ProcessMetric();

#if !defined(OS_LINUX)
  ProcessMemoryInfo GetMemoryInfo() const;
#endif

#if defined(OS_WIN)
  ProcessIntegrityLevel GetIntegrityLevel() const;
  static bool IsSandboxed(ProcessIntegrityLevel integrity_level);
#elif defined(OS_MAC)
  bool IsSandboxed() const;
#endif
};

}  // namespace electron

#endif  // SHELL_BROWSER_API_PROCESS_METRIC_H_

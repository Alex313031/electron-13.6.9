// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_BROWSER_API_ELECTRON_API_NET_LOG_H_
#define SHELL_BROWSER_API_ELECTRON_API_NET_LOG_H_

#include <list>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/optional.h"
#include "base/values.h"
#include "gin/handle.h"
#include "gin/wrappable.h"
#include "services/network/public/mojom/net_log.mojom.h"
#include "shell/common/gin_helper/promise.h"

namespace gin {
class Arguments;
}

namespace electron {

class ElectronBrowserContext;

namespace api {

class NetLog : public gin::Wrappable<NetLog> {
 public:
  static gin::Handle<NetLog> Create(v8::Isolate* isolate,
                                    ElectronBrowserContext* browser_context);

  v8::Local<v8::Promise> StartLogging(base::FilePath log_path,
                                      gin::Arguments* args);
  v8::Local<v8::Promise> StopLogging(gin::Arguments* args);
  bool IsCurrentlyLogging() const;

  // gin::Wrappable
  static gin::WrapperInfo kWrapperInfo;
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;
  const char* GetTypeName() override;

 protected:
  explicit NetLog(v8::Isolate* isolate,
                  ElectronBrowserContext* browser_context);
  ~NetLog() override;

  void OnConnectionError();

  void StartNetLogAfterCreateFile(net::NetLogCaptureMode capture_mode,
                                  uint64_t max_file_size,
                                  base::Value custom_constants,
                                  base::File output_file);
  void NetLogStarted(int32_t error);

 private:
  ElectronBrowserContext* browser_context_;

  network::mojom::NetLogExporterPtr net_log_exporter_;

  base::Optional<gin_helper::Promise<void>> pending_start_promise_;

  scoped_refptr<base::TaskRunner> file_task_runner_;

  base::WeakPtrFactory<NetLog> weak_ptr_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(NetLog);
};

}  // namespace api

}  // namespace electron

#endif  // SHELL_BROWSER_API_ELECTRON_API_NET_LOG_H_

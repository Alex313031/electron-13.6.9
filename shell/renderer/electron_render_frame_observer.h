// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_
#define SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_

#include <string>

#include "content/public/renderer/render_frame_observer.h"
#include "ipc/ipc_platform_file.h"
#include "shell/renderer/renderer_client_base.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace base {
class ListValue;
}

namespace electron {

// Helper class to forward the messages to the client.
class ElectronRenderFrameObserver : public content::RenderFrameObserver {
 public:
  ElectronRenderFrameObserver(content::RenderFrame* frame,
                              RendererClientBase* renderer_client);

  // content::RenderFrameObserver:
  void DidClearWindowObject() override;
  void DidInstallConditionalFeatures(v8::Handle<v8::Context> context,
                                     int world_id) override;
  void DraggableRegionsChanged() override;
  void WillReleaseScriptContext(v8::Local<v8::Context> context,
                                int world_id) override;
  void OnDestruct() override;
  void DidMeaningfulLayout(blink::WebMeaningfulLayout layout_type) override;

 private:
  bool ShouldNotifyClient(int world_id);
  void CreateIsolatedWorldContext();
  bool IsMainWorld(int world_id);
  bool IsIsolatedWorld(int world_id);
  void OnTakeHeapSnapshot(IPC::PlatformFileForTransit file_handle,
                          const std::string& channel);

  content::RenderFrame* render_frame_;
  RendererClientBase* renderer_client_;

  DISALLOW_COPY_AND_ASSIGN(ElectronRenderFrameObserver);
};

}  // namespace electron

#endif  // SHELL_RENDERER_ELECTRON_RENDER_FRAME_OBSERVER_H_

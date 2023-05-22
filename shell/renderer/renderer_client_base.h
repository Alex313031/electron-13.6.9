// Copyright (c) 2017 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#ifndef SHELL_RENDERER_RENDERER_CLIENT_BASE_H_
#define SHELL_RENDERER_RENDERER_CLIENT_BASE_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "content/public/renderer/content_renderer_client.h"
#include "electron/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "shell/common/gin_helper/dictionary.h"
#include "third_party/blink/public/web/web_local_frame.h"
// In SHARED_INTERMEDIATE_DIR.
#include "widevine_cdm_version.h"  // NOLINT(build/include_directory)

#if defined(WIDEVINE_CDM_AVAILABLE)
#include "chrome/renderer/media/chrome_key_systems_provider.h"  // nogncheck
#endif

#if BUILDFLAG(ENABLE_PDF_VIEWER)
#include "chrome/renderer/pepper/chrome_pdf_print_client.h"  // nogncheck
#endif  // BUILDFLAG(ENABLE_PDF_VIEWER)

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
#include "services/service_manager/public/cpp/binder_registry.h"
#include "services/service_manager/public/cpp/local_interface_provider.h"

class SpellCheck;
#endif

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
namespace extensions {
class ExtensionsClient;
}
namespace content {
struct WebPluginInfo;
}
#endif

namespace electron {

class ElectronApiServiceImpl;

#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
class ElectronExtensionsRendererClient;
#endif

class RendererClientBase : public content::ContentRendererClient
#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
    ,
                           public service_manager::LocalInterfaceProvider
#endif
{
 public:
  RendererClientBase();
  ~RendererClientBase() override;

  static RendererClientBase* Get();

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  // service_manager::LocalInterfaceProvider implementation.
  void GetInterface(const std::string& name,
                    mojo::ScopedMessagePipeHandle interface_pipe) override;
#endif

  virtual void DidCreateScriptContext(v8::Handle<v8::Context> context,
                                      content::RenderFrame* render_frame);
  virtual void WillReleaseScriptContext(v8::Handle<v8::Context> context,
                                        content::RenderFrame* render_frame) = 0;
  virtual void DidClearWindowObject(content::RenderFrame* render_frame);
  virtual void SetupMainWorldOverrides(v8::Handle<v8::Context> context,
                                       content::RenderFrame* render_frame) = 0;

  std::unique_ptr<blink::WebPrescientNetworking> CreatePrescientNetworking(
      content::RenderFrame* render_frame) override;

  // Get the context that the Electron API is running in.
  v8::Local<v8::Context> GetContext(blink::WebLocalFrame* frame,
                                    v8::Isolate* isolate) const;
  // Executes a given v8 Script
  static v8::Local<v8::Value> RunScript(v8::Local<v8::Context> context,
                                        v8::Local<v8::String> source);

  // v8Util.getHiddenValue(window.frameElement, 'internal')
  bool IsWebViewFrame(v8::Handle<v8::Context> context,
                      content::RenderFrame* render_frame) const;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  SpellCheck* GetSpellCheck() { return spellcheck_.get(); }
#endif

 protected:
  void BindProcess(v8::Isolate* isolate,
                   gin_helper::Dictionary* process,
                   content::RenderFrame* render_frame);

  // content::ContentRendererClient:
  void RenderThreadStarted() override;
  void ExposeInterfacesToBrowser(mojo::BinderMap* binders) override;
  void RenderFrameCreated(content::RenderFrame*) override;
  bool OverrideCreatePlugin(content::RenderFrame* render_frame,
                            const blink::WebPluginParams& params,
                            blink::WebPlugin** plugin) override;
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<::media::KeySystemProperties>>* key_systems)
      override;
  bool IsKeySystemsUpdateNeeded() override;
  void DidSetUserAgent(const std::string& user_agent) override;
  bool IsPluginHandledExternally(content::RenderFrame* render_frame,
                                 const blink::WebElement& plugin_element,
                                 const GURL& original_url,
                                 const std::string& mime_type) override;
  bool IsOriginIsolatedPepperPlugin(const base::FilePath& plugin_path) override;

  void RunScriptsAtDocumentStart(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentEnd(content::RenderFrame* render_frame) override;
  void RunScriptsAtDocumentIdle(content::RenderFrame* render_frame) override;

  bool AllowScriptExtensionForServiceWorker(
      const url::Origin& script_origin) override;
  void DidInitializeServiceWorkerContextOnWorkerThread(
      blink::WebServiceWorkerContextProxy* context_proxy,
      const GURL& service_worker_scope,
      const GURL& script_url) override;
  void WillEvaluateServiceWorkerOnWorkerThread(
      blink::WebServiceWorkerContextProxy* context_proxy,
      v8::Local<v8::Context> v8_context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;
  void DidStartServiceWorkerContextOnWorkerThread(
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;
  void WillDestroyServiceWorkerContextOnWorkerThread(
      v8::Local<v8::Context> context,
      int64_t service_worker_version_id,
      const GURL& service_worker_scope,
      const GURL& script_url) override;

 protected:
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  // app_shell embedders may need custom extensions client interfaces.
  // This class takes ownership of the returned object.
  virtual extensions::ExtensionsClient* CreateExtensionsClient();
#endif

 private:
#if BUILDFLAG(ENABLE_ELECTRON_EXTENSIONS)
  std::unique_ptr<extensions::ExtensionsClient> extensions_client_;
  std::unique_ptr<ElectronExtensionsRendererClient> extensions_renderer_client_;
#endif

#if defined(WIDEVINE_CDM_AVAILABLE)
  ChromeKeySystemsProvider key_systems_provider_;
#endif
  std::string renderer_client_id_;
  // An increasing ID used for identifying an V8 context in this process.
  int64_t next_context_id_ = 0;

#if BUILDFLAG(ENABLE_BUILTIN_SPELLCHECKER)
  std::unique_ptr<SpellCheck> spellcheck_;
#endif
#if BUILDFLAG(ENABLE_PDF_VIEWER)
  std::unique_ptr<ChromePDFPrintClient> pdf_print_client_;
#endif
};

}  // namespace electron

#endif  // SHELL_RENDERER_RENDERER_CLIENT_BASE_H_

// Copyright (c) 2018 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/browser/net/system_network_context_manager.h"

#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/path_service.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/net/chrome_mojo_proxy_resolver_factory.h"
#include "chrome/common/chrome_switches.h"
#include "components/os_crypt/os_crypt.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/network_service_instance.h"
#include "content/public/common/content_features.h"
#include "content/public/common/network_service_util.h"
#include "electron/fuses.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "net/net_buildflags.h"
#include "services/cert_verifier/public/mojom/cert_verifier_service_factory.mojom.h"
#include "services/network/network_service.h"
#include "services/network/public/cpp/cross_thread_pending_shared_url_loader_factory.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "shell/browser/browser.h"
#include "shell/browser/electron_browser_client.h"
#include "shell/common/application_info.h"
#include "shell/common/electron_paths.h"
#include "shell/common/options_switches.h"
#include "url/gurl.h"

#if defined(OS_MAC)
#include "components/os_crypt/keychain_password_mac.h"
#endif

namespace {

// The global instance of the SystemNetworkContextmanager.
SystemNetworkContextManager* g_system_network_context_manager = nullptr;

network::mojom::HttpAuthStaticParamsPtr CreateHttpAuthStaticParams() {
  network::mojom::HttpAuthStaticParamsPtr auth_static_params =
      network::mojom::HttpAuthStaticParams::New();

  auth_static_params->supported_schemes = {"basic", "digest", "ntlm",
                                           "negotiate"};

  return auth_static_params;
}

network::mojom::HttpAuthDynamicParamsPtr CreateHttpAuthDynamicParams() {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  network::mojom::HttpAuthDynamicParamsPtr auth_dynamic_params =
      network::mojom::HttpAuthDynamicParams::New();

  auth_dynamic_params->server_allowlist = command_line->GetSwitchValueASCII(
      electron::switches::kAuthServerWhitelist);
  auth_dynamic_params->delegate_allowlist = command_line->GetSwitchValueASCII(
      electron::switches::kAuthNegotiateDelegateWhitelist);
  auth_dynamic_params->enable_negotiate_port =
      command_line->HasSwitch(electron::switches::kEnableAuthNegotiatePort);
  auth_dynamic_params->ntlm_v2_enabled =
      !command_line->HasSwitch(electron::switches::kDisableNTLMv2);

  return auth_dynamic_params;
}

}  // namespace

// SharedURLLoaderFactory backed by a SystemNetworkContextManager and its
// network context. Transparently handles crashes.
class SystemNetworkContextManager::URLLoaderFactoryForSystem
    : public network::SharedURLLoaderFactory {
 public:
  explicit URLLoaderFactoryForSystem(SystemNetworkContextManager* manager)
      : manager_(manager) {
    DETACH_FROM_SEQUENCE(sequence_checker_);
  }

  // mojom::URLLoaderFactory implementation:
  void CreateLoaderAndStart(
      mojo::PendingReceiver<network::mojom::URLLoader> request,
      int32_t request_id,
      uint32_t options,
      const network::ResourceRequest& url_request,
      mojo::PendingRemote<network::mojom::URLLoaderClient> client,
      const net::MutableNetworkTrafficAnnotationTag& traffic_annotation)
      override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    if (!manager_)
      return;
    manager_->GetURLLoaderFactory()->CreateLoaderAndStart(
        std::move(request), request_id, options, url_request, std::move(client),
        traffic_annotation);
  }

  void Clone(mojo::PendingReceiver<network::mojom::URLLoaderFactory> receiver)
      override {
    if (!manager_)
      return;
    manager_->GetURLLoaderFactory()->Clone(std::move(receiver));
  }

  // SharedURLLoaderFactory implementation:
  std::unique_ptr<network::PendingSharedURLLoaderFactory> Clone() override {
    DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return std::make_unique<network::CrossThreadPendingSharedURLLoaderFactory>(
        this);
  }

  void Shutdown() { manager_ = nullptr; }

 private:
  friend class base::RefCounted<URLLoaderFactoryForSystem>;
  ~URLLoaderFactoryForSystem() override = default;

  SEQUENCE_CHECKER(sequence_checker_);
  SystemNetworkContextManager* manager_;

  DISALLOW_COPY_AND_ASSIGN(URLLoaderFactoryForSystem);
};

network::mojom::NetworkContext* SystemNetworkContextManager::GetContext() {
  if (!network_context_ || !network_context_.is_connected()) {
    // This should call into OnNetworkServiceCreated(), which will re-create
    // the network service, if needed. There's a chance that it won't be
    // invoked, if the NetworkContext has encountered an error but the
    // NetworkService has not yet noticed its pipe was closed. In that case,
    // trying to create a new NetworkContext would fail, anyways, and hopefully
    // a new NetworkContext will be created on the next GetContext() call.
    content::GetNetworkService();
    DCHECK(network_context_);
  }
  return network_context_.get();
}

network::mojom::URLLoaderFactory*
SystemNetworkContextManager::GetURLLoaderFactory() {
  // Create the URLLoaderFactory as needed.
  if (url_loader_factory_ && url_loader_factory_.is_connected()) {
    return url_loader_factory_.get();
  }

  network::mojom::URLLoaderFactoryParamsPtr params =
      network::mojom::URLLoaderFactoryParams::New();
  params->process_id = network::mojom::kBrowserProcessId;
  params->is_corb_enabled = false;
  url_loader_factory_.reset();
  GetContext()->CreateURLLoaderFactory(
      url_loader_factory_.BindNewPipeAndPassReceiver(), std::move(params));
  return url_loader_factory_.get();
}

scoped_refptr<network::SharedURLLoaderFactory>
SystemNetworkContextManager::GetSharedURLLoaderFactory() {
  return shared_url_loader_factory_;
}

network::mojom::NetworkContextParamsPtr
SystemNetworkContextManager::CreateDefaultNetworkContextParams() {
  network::mojom::NetworkContextParamsPtr network_context_params =
      network::mojom::NetworkContextParams::New();

  ConfigureDefaultNetworkContextParams(network_context_params.get());

  cert_verifier::mojom::CertVerifierCreationParamsPtr
      cert_verifier_creation_params =
          cert_verifier::mojom::CertVerifierCreationParams::New();
  network_context_params->cert_verifier_params =
      content::GetCertVerifierParams(std::move(cert_verifier_creation_params));
  return network_context_params;
}

void SystemNetworkContextManager::ConfigureDefaultNetworkContextParams(
    network::mojom::NetworkContextParams* network_context_params) {
  network_context_params->enable_brotli = true;

  network_context_params->enable_referrers = true;

  network_context_params->proxy_resolver_factory =
      ChromeMojoProxyResolverFactory::CreateWithSelfOwnedReceiver();

#if !BUILDFLAG(DISABLE_FTP_SUPPORT)
  network_context_params->enable_ftp_url_support = true;
#endif
}

// static
SystemNetworkContextManager* SystemNetworkContextManager::CreateInstance(
    PrefService* pref_service) {
  DCHECK(!g_system_network_context_manager);
  g_system_network_context_manager =
      new SystemNetworkContextManager(pref_service);
  return g_system_network_context_manager;
}

// static
SystemNetworkContextManager* SystemNetworkContextManager::GetInstance() {
  return g_system_network_context_manager;
}

// static
void SystemNetworkContextManager::DeleteInstance() {
  DCHECK(g_system_network_context_manager);
  delete g_system_network_context_manager;
}

SystemNetworkContextManager::SystemNetworkContextManager(
    PrefService* pref_service)
    : proxy_config_monitor_(pref_service) {
  shared_url_loader_factory_ = new URLLoaderFactoryForSystem(this);
}

SystemNetworkContextManager::~SystemNetworkContextManager() {
  shared_url_loader_factory_->Shutdown();
}

void SystemNetworkContextManager::OnNetworkServiceCreated(
    network::mojom::NetworkService* network_service) {
  network_service->SetUpHttpAuth(CreateHttpAuthStaticParams());
  network_service->ConfigureHttpAuthPrefs(CreateHttpAuthDynamicParams());

  network_context_.reset();
  network_service->CreateNetworkContext(
      network_context_.BindNewPipeAndPassReceiver(),
      CreateNetworkContextParams());

  if (electron::fuses::IsCookieEncryptionEnabled()) {
    std::string app_name = electron::Browser::Get()->GetName();
#if defined(OS_MAC)
    *KeychainPassword::service_name = app_name + " Safe Storage";
    *KeychainPassword::account_name = app_name;
#endif
    // The OSCrypt keys are process bound, so if network service is out of
    // process, send it the required key.
    if (content::IsOutOfProcessNetworkService()) {
#if defined(OS_LINUX)
      // c.f.
      // https://source.chromium.org/chromium/chromium/src/+/master:chrome/browser/net/system_network_context_manager.cc;l=515;drc=9d82515060b9b75fa941986f5db7390299669ef1;bpv=1;bpt=1
      const base::CommandLine& command_line =
          *base::CommandLine::ForCurrentProcess();

      network::mojom::CryptConfigPtr config =
          network::mojom::CryptConfig::New();
      config->application_name = app_name;
      config->product_name = app_name;
      // c.f.
      // https://source.chromium.org/chromium/chromium/src/+/master:chrome/common/chrome_switches.cc;l=689;drc=9d82515060b9b75fa941986f5db7390299669ef1
      config->store =
          command_line.GetSwitchValueASCII(::switches::kPasswordStore);
      config->should_use_preference =
          command_line.HasSwitch(::switches::kEnableEncryptionSelection);
      base::PathService::Get(electron::DIR_USER_DATA, &config->user_data_path);
      network_service->SetCryptConfig(std::move(config));
#else
      network_service->SetEncryptionKey(OSCrypt::GetRawEncryptionKey());
#endif
    }
  }
}

network::mojom::NetworkContextParamsPtr
SystemNetworkContextManager::CreateNetworkContextParams() {
  // TODO(mmenke): Set up parameters here (in memory cookie store, etc).
  network::mojom::NetworkContextParamsPtr network_context_params =
      CreateDefaultNetworkContextParams();

  network_context_params->context_name = std::string("system");

  network_context_params->user_agent =
      electron::ElectronBrowserClient::Get()->GetUserAgent();

  network_context_params->http_cache_enabled = false;

  auto ssl_config = network::mojom::SSLConfig::New();
  ssl_config->version_min = network::mojom::SSLVersion::kTLS12;
  network_context_params->initial_ssl_config = std::move(ssl_config);

  proxy_config_monitor_.AddToNetworkContextParams(network_context_params.get());

  return network_context_params;
}

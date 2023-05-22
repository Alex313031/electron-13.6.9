// Copyright (c) 2015 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/utility/electron_content_utility_client.h"

#include <utility>

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "content/public/utility/utility_thread.h"
#include "mojo/public/cpp/bindings/service_factory.h"
#include "sandbox/policy/switches.h"
#include "services/proxy_resolver/proxy_resolver_factory_impl.h"
#include "services/proxy_resolver/public/mojom/proxy_resolver.mojom.h"
#include "services/service_manager/public/cpp/service.h"

#if defined(OS_WIN)
#include "chrome/services/util_win/public/mojom/util_read_icon.mojom.h"
#include "chrome/services/util_win/util_read_icon.h"
#endif  // defined(OS_WIN)

#if BUILDFLAG(ENABLE_PRINTING)
#include "components/services/print_compositor/print_compositor_impl.h"
#include "components/services/print_compositor/public/mojom/print_compositor.mojom.h"  // nogncheck
#endif  // BUILDFLAG(ENABLE_PRINTING)

#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
#include "chrome/services/printing/pdf_to_emf_converter_factory.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && defined(OS_WIN)
#include "chrome/utility/printing_handler.h"
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN))
#include "chrome/services/printing/printing_service.h"
#include "chrome/services/printing/public/mojom/printing_service.mojom.h"
#endif

namespace electron {

namespace {

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN))
auto RunPrintingService(
    mojo::PendingReceiver<printing::mojom::PrintingService> receiver) {
  return std::make_unique<printing::PrintingService>(std::move(receiver));
}
#endif

#if defined(OS_WIN)
auto RunWindowsIconReader(
    mojo::PendingReceiver<chrome::mojom::UtilReadIcon> receiver) {
  return std::make_unique<UtilReadIcon>(std::move(receiver));
}
#endif

#if BUILDFLAG(ENABLE_PRINTING)
auto RunPrintCompositor(
    mojo::PendingReceiver<printing::mojom::PrintCompositor> receiver) {
  return std::make_unique<printing::PrintCompositorImpl>(
      std::move(receiver), true /* initialize_environment */,
      content::UtilityThread::Get()->GetIOTaskRunner());
}
#endif

auto RunProxyResolver(
    mojo::PendingReceiver<proxy_resolver::mojom::ProxyResolverFactory>
        receiver) {
  return std::make_unique<proxy_resolver::ProxyResolverFactoryImpl>(
      std::move(receiver));
}

}  // namespace

ElectronContentUtilityClient::ElectronContentUtilityClient() {
#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && defined(OS_WIN)
  printing_handler_ = std::make_unique<printing::PrintingHandler>();
#endif
}

ElectronContentUtilityClient::~ElectronContentUtilityClient() = default;

// The guts of this came from the chromium implementation
// https://cs.chromium.org/chromium/src/chrome/utility/
// chrome_content_utility_client.cc?sq=package:chromium&dr=CSs&g=0&l=142
void ElectronContentUtilityClient::ExposeInterfacesToBrowser(
    mojo::BinderMap* binders) {
#if defined(OS_WIN)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  utility_process_running_elevated_ = command_line->HasSwitch(
      sandbox::policy::switches::kNoSandboxAndElevatedPrivileges);
#endif

  // If our process runs with elevated privileges, only add elevated Mojo
  // interfaces to the BinderMap.
  if (!utility_process_running_elevated_) {
#if BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN)
    binders->Add(
        base::BindRepeating(printing::PdfToEmfConverterFactory::Create),
        base::ThreadTaskRunnerHandle::Get());
#endif
  }
}

bool ElectronContentUtilityClient::OnMessageReceived(
    const IPC::Message& message) {
  if (utility_process_running_elevated_)
    return false;

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) && defined(OS_WIN)
  if (printing_handler_->OnMessageReceived(message))
    return true;
#endif

  return false;
}

void ElectronContentUtilityClient::RegisterMainThreadServices(
    mojo::ServiceFactory& services) {
#if defined(OS_WIN)
  services.Add(RunWindowsIconReader);
#endif

#if BUILDFLAG(ENABLE_PRINTING)
  services.Add(RunPrintCompositor);
#endif

#if BUILDFLAG(ENABLE_PRINT_PREVIEW) || \
    (BUILDFLAG(ENABLE_PRINTING) && defined(OS_WIN))
  services.Add(RunPrintingService);
#endif
}

void ElectronContentUtilityClient::RegisterIOThreadServices(
    mojo::ServiceFactory& services) {
  services.Add(RunProxyResolver);
}

}  // namespace electron

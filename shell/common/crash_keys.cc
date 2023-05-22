// Copyright (c) 2020 Slack Technologies, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "shell/common/crash_keys.h"

#include <deque>
#include <utility>
#include <vector>

#include "base/command_line.h"
#include "base/environment.h"
#include "base/no_destructor.h"
#include "base/strings/string_split.h"
#include "components/crash/core/common/crash_key.h"
#include "content/public/common/content_switches.h"
#include "electron/buildflags/buildflags.h"
#include "electron/fuses.h"
#include "shell/common/electron_constants.h"
#include "shell/common/options_switches.h"
#include "third_party/crashpad/crashpad/client/annotation.h"

#include "gin/wrappable.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/browser/api/electron_api_auto_updater.h"
#include "shell/browser/api/electron_api_browser_view.h"
#include "shell/browser/api/electron_api_cookies.h"
#include "shell/browser/api/electron_api_data_pipe_holder.h"
#include "shell/browser/api/electron_api_debugger.h"
#include "shell/browser/api/electron_api_desktop_capturer.h"
#include "shell/browser/api/electron_api_download_item.h"
#include "shell/browser/api/electron_api_global_shortcut.h"
#include "shell/browser/api/electron_api_in_app_purchase.h"
#include "shell/browser/api/electron_api_menu.h"
#include "shell/browser/api/electron_api_native_theme.h"
#include "shell/browser/api/electron_api_net_log.h"
#include "shell/browser/api/electron_api_notification.h"
#include "shell/browser/api/electron_api_power_monitor.h"
#include "shell/browser/api/electron_api_power_save_blocker.h"
#include "shell/browser/api/electron_api_protocol.h"
#include "shell/browser/api/electron_api_service_worker_context.h"
#include "shell/browser/api/electron_api_session.h"
#include "shell/browser/api/electron_api_system_preferences.h"
#include "shell/browser/api/electron_api_tray.h"
#include "shell/browser/api/electron_api_url_loader.h"
#include "shell/browser/api/electron_api_web_contents.h"
#include "shell/browser/api/electron_api_web_frame_main.h"
#include "shell/browser/api/electron_api_web_request.h"
#include "shell/browser/api/event.h"
#include "shell/common/api/electron_api_native_image.h"

namespace electron {

namespace crash_keys {

namespace {

#if defined(OS_LINUX)
// Breakpad has a flawed system of calculating the number of chunks
// we add 127 bytes to force an extra chunk
constexpr size_t kMaxCrashKeyValueSize = 20479;
#else
constexpr size_t kMaxCrashKeyValueSize = 20320;
#endif

static_assert(kMaxCrashKeyValueSize < crashpad::Annotation::kValueMaxSize,
              "max crash key value length above what crashpad supports");

using ExtraCrashKeys =
    std::deque<crash_reporter::CrashKeyString<kMaxCrashKeyValueSize>>;
ExtraCrashKeys& GetExtraCrashKeys() {
  static base::NoDestructor<ExtraCrashKeys> extra_keys;
  return *extra_keys;
}

std::deque<std::string>& GetExtraCrashKeyNames() {
  static base::NoDestructor<std::deque<std::string>> crash_key_names;
  return *crash_key_names;
}

}  // namespace

constexpr uint32_t kMaxCrashKeyNameLength = 40;
#if defined(OS_LINUX)
static_assert(kMaxCrashKeyNameLength <=
                  crash_reporter::internal::kCrashKeyStorageKeySize,
              "max crash key name length above what breakpad supports");
#else
static_assert(kMaxCrashKeyNameLength <= crashpad::Annotation::kNameMaxLength,
              "max crash key name length above what crashpad supports");
#endif

void SetCrashKey(const std::string& key, const std::string& value) {
  // Chrome DCHECK()s if we try to set an annotation with a name longer than
  // the max.
  // TODO(nornagon): warn the developer (via console.warn) when this happens.
  if (key.size() >= kMaxCrashKeyNameLength)
    return;
  auto& crash_key_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_key_names.begin(), crash_key_names.end(), key);
  if (iter == crash_key_names.end()) {
    crash_key_names.emplace_back(key);
    GetExtraCrashKeys().emplace_back(crash_key_names.back().c_str());
    iter = crash_key_names.end() - 1;
  }
  GetExtraCrashKeys()[iter - crash_key_names.begin()].Set(value);
}

void ClearCrashKey(const std::string& key) {
  const auto& crash_key_names = GetExtraCrashKeyNames();

  auto iter = std::find(crash_key_names.begin(), crash_key_names.end(), key);
  if (iter != crash_key_names.end()) {
    GetExtraCrashKeys()[iter - crash_key_names.begin()].Clear();
  }
}

void GetCrashKeys(std::map<std::string, std::string>* keys) {
  const auto& crash_key_names = GetExtraCrashKeyNames();
  const auto& crash_keys = GetExtraCrashKeys();
  int i = 0;
  for (const auto& key : crash_key_names) {
    const auto& value = crash_keys[i++];
    if (value.is_set()) {
      keys->emplace(key, value.value());
    }
  }
}

namespace {
bool IsRunningAsNode() {
#if BUILDFLAG(ENABLE_RUN_AS_NODE)
  if (!electron::fuses::IsRunAsNodeEnabled())
    return false;

  return base::Environment::Create()->HasVar(electron::kRunAsNode);
#else
  return false;
#endif
}
}  // namespace

void SetCrashKeysFromCommandLine(const base::CommandLine& command_line) {
#if defined(OS_LINUX)
  if (command_line.HasSwitch(switches::kGlobalCrashKeys)) {
    std::vector<std::pair<std::string, std::string>> global_crash_keys;
    base::SplitStringIntoKeyValuePairs(
        command_line.GetSwitchValueASCII(switches::kGlobalCrashKeys), '=', ',',
        &global_crash_keys);
    for (const auto& pair : global_crash_keys) {
      SetCrashKey(pair.first, pair.second);
    }
  }
#endif

  // NB. this is redundant with the 'ptype' key that //components/crash
  // reports; it's present for backwards compatibility.
  static crash_reporter::CrashKeyString<16> process_type_key("process_type");
  if (IsRunningAsNode()) {
    process_type_key.Set("node");
  } else {
    std::string process_type =
        command_line.GetSwitchValueASCII(::switches::kProcessType);
    if (process_type.empty()) {
      process_type_key.Set("browser");
    } else {
      process_type_key.Set(process_type);
    }
  }
}

void SetPlatformCrashKey() {
  // TODO(nornagon): this is redundant with the 'plat' key that
  // //components/crash already includes. Remove it.
  static crash_reporter::CrashKeyString<8> platform_key("platform");
#if defined(OS_WIN)
  platform_key.Set("win32");
#elif defined(OS_MAC)
  platform_key.Set("darwin");
#elif defined(OS_LINUX)
  platform_key.Set("linux");
#else
  platform_key.Set("unknown");
#endif
}

std::string GetCrashValueForGinWrappable(gin::WrapperInfo* info) {
  std::string crash_location;

  // Adds a breadcrumb for crashes within gin::WrappableBase::SecondWeakCallback
  // (see patch: add_gin_wrappable_crash_key.patch)
  // Compares the pointers for the kWrapperInfo within SecondWeakCallback
  // with the wrapper info from classes that use gin::Wrappable and
  // could potentially retain a reference after deletion.
  if (info == &electron::api::WebContents::kWrapperInfo)
    crash_location = "WebContents";
  else if (info == &electron::api::BrowserView::kWrapperInfo)
    crash_location = "BrowserView";
  else if (info == &electron::api::Notification::kWrapperInfo)
    crash_location = "Notification";
  else if (info == &electron::api::Cookies::kWrapperInfo)
    crash_location = "Cookies";
#if BUILDFLAG(ENABLE_DESKTOP_CAPTURER)
  else if (info == &electron::api::DesktopCapturer::kWrapperInfo)
    crash_location = "DesktopCapturer";
#endif
  else if (info == &electron::api::NetLog::kWrapperInfo)
    crash_location = "NetLog";
  else if (info == &electron::api::NativeImage::kWrapperInfo)
    crash_location = "NativeImage";
  else if (info == &electron::api::Menu::kWrapperInfo)
    crash_location = "Menu";
  else if (info == &electron::api::PowerMonitor::kWrapperInfo)
    crash_location = "PowerMonitor";
  else if (info == &electron::api::Protocol::kWrapperInfo)
    crash_location = "Protocol";
  else if (info == &electron::api::ServiceWorkerContext::kWrapperInfo)
    crash_location = "ServiceWorkerContext";
  else if (info == &electron::api::WebFrameMain::kWrapperInfo)
    crash_location = "WebFrameMain";
  else if (info == &electron::api::WebRequest::kWrapperInfo)
    crash_location = "WebRequest";
  else if (info == &electron::api::SystemPreferences::kWrapperInfo)
    crash_location = "SystemPreferences";
  else if (info == &electron::api::Session::kWrapperInfo)
    crash_location = "Session";
  else if (info == &electron::api::DownloadItem::kWrapperInfo)
    crash_location = "DownloadItem";
  else if (info == &electron::api::NativeTheme::kWrapperInfo)
    crash_location = "NativeTheme";
  else if (info == &electron::api::Debugger::kWrapperInfo)
    crash_location = "Debugger";
  else if (info == &electron::api::GlobalShortcut::kWrapperInfo)
    crash_location = "GlobalShortcut";
  else if (info == &electron::api::InAppPurchase::kWrapperInfo)
    crash_location = "InAppPurchase";
  else if (info == &electron::api::Tray::kWrapperInfo)
    crash_location = "Tray";
  else if (info == &electron::api::DataPipeHolder::kWrapperInfo)
    crash_location = "DataPipeHolder";
  else if (info == &electron::api::AutoUpdater::kWrapperInfo)
    crash_location = "AutoUpdater";
  else if (info == &electron::api::SimpleURLLoaderWrapper::kWrapperInfo)
    crash_location = "SimpleURLLoaderWrapper";
  else if (info == &gin_helper::Event::kWrapperInfo)
    crash_location = "Event";
  else if (info == &electron::api::PowerSaveBlocker::kWrapperInfo)
    crash_location = "PowerSaveBlocker";
  else if (info == &electron::api::App::kWrapperInfo)
    crash_location = "App";
  else
    crash_location =
        "Deleted kWrapperInfo does not match listed component. Please review "
        "listed crash keys.";

  return crash_location;
}

}  // namespace crash_keys

}  // namespace electron

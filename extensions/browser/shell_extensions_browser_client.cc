// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/crwebview/extensions/browser/shell_extensions_browser_client.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/user_agent.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/core_extensions_browser_api_provider.h"
#include "extensions/browser/event_router.h"
#include "extensions/browser/extensions_browser_interface_binders.h"
#include "extensions/browser/null_app_sorting.h"
#include "extensions/browser/updater/null_extension_cache.h"
#include "extensions/browser/url_request_util.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/shell/browser/api/runtime/shell_runtime_api_delegate.h"
//#include "extensions/shell/browser/delegates/shell_kiosk_delegate.h"
#include "android_webview/crwebview/extensions/browser/shell_extension_host_delegate.h"
#include "android_webview/crwebview/extensions/browser/shell_extension_system_factory.h"
#include "android_webview/crwebview/extensions/browser/shell_extension_web_contents_observer.h"
#include "android_webview/crwebview/extensions/browser/shell_extensions_api_client.h"
#include "android_webview/crwebview/extensions/browser/shell_navigation_ui_data.h"
#include "services/network/public/mojom/url_loader.mojom.h"
#include "chrome/browser/extensions/chrome_extensions_browser_api_provider.h"

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "chromeos/login/login_state/login_state.h"
#endif

using content::BrowserContext;
using content::BrowserThread;

namespace extensions {

namespace {
  class MyProcessManagerDelegate: public extensions::ProcessManagerDelegate {
  public:
    MyProcessManagerDelegate() {}
    ~MyProcessManagerDelegate() override {}

    // ProcessManagerDelegate:
    bool AreBackgroundPagesAllowedForContext(
        content::BrowserContext* context) const override {
      return true;
    }
    bool IsExtensionBackgroundPageAllowed(
        content::BrowserContext* context,
        const Extension& extension) const override {
      return true;
    }
    bool DeferCreatingStartupBackgroundHosts(
        content::BrowserContext* context) const override {
      return false;
    }
  };
}

static MyProcessManagerDelegate process_manager_delegate_;

ShellExtensionsBrowserClient::ShellExtensionsBrowserClient()
    : api_client_(new ShellExtensionsAPIClient),
      extension_cache_(new NullExtensionCache()) {
  // app_shell does not have a concept of channel yet, so leave UNKNOWN to
  // enable all channel-dependent extension APIs.
  SetCurrentChannel(version_info::Channel::STABLE);

  AddAPIProvider(std::make_unique<ChromeExtensionsBrowserAPIProvider>());
  AddAPIProvider(std::make_unique<CoreExtensionsBrowserAPIProvider>());
}

ShellExtensionsBrowserClient::~ShellExtensionsBrowserClient() {
}

bool ShellExtensionsBrowserClient::IsShuttingDown() {
  return false;
}

bool ShellExtensionsBrowserClient::AreExtensionsDisabled(
    const base::CommandLine& command_line,
    BrowserContext* context) {
  return false;
}

bool ShellExtensionsBrowserClient::IsValidContext(BrowserContext* context) {
  DCHECK(browser_context_);
  return context == browser_context_;
}

bool ShellExtensionsBrowserClient::IsSameContext(BrowserContext* first,
                                                 BrowserContext* second) {
  return first == second;
}

bool ShellExtensionsBrowserClient::HasOffTheRecordContext(
    BrowserContext* context) {
  return false;
}

BrowserContext* ShellExtensionsBrowserClient::GetOffTheRecordContext(
    BrowserContext* context) {
  // app_shell only supports a single context.
  return NULL;
}

BrowserContext* ShellExtensionsBrowserClient::GetOriginalContext(
    BrowserContext* context) {
  return context;
}

#if BUILDFLAG(IS_CHROMEOS_ASH)
std::string ShellExtensionsBrowserClient::GetUserIdHashFromContext(
    content::BrowserContext* context) {
  if (!chromeos::LoginState::IsInitialized())
    return "";
  return chromeos::LoginState::Get()->primary_user_hash();
}
#endif

#if BUILDFLAG(IS_CHROMEOS_LACROS)
bool ShellExtensionsBrowserClient::IsFromMainProfile(
    content::BrowserContext* context) {
  // AppShell only supports single context.
  return true;
}
#endif

bool ShellExtensionsBrowserClient::IsGuestSession(
    BrowserContext* context) const {
  return true;
}

bool ShellExtensionsBrowserClient::IsExtensionIncognitoEnabled(
    const std::string& extension_id,
    content::BrowserContext* context) const {
  return false;
}

bool ShellExtensionsBrowserClient::CanExtensionCrossIncognito(
    const Extension* extension,
    content::BrowserContext* context) const {
  return false;
}

base::FilePath ShellExtensionsBrowserClient::GetBundleResourcePath(
    const network::ResourceRequest& request,
    const base::FilePath& extension_resources_path,
    int* resource_id) const {
  *resource_id = 0;
  return base::FilePath();
}

void ShellExtensionsBrowserClient::LoadResourceFromResourceBundle(
    const network::ResourceRequest& request,
    mojo::PendingReceiver<network::mojom::URLLoader> loader,
    const base::FilePath& resource_relative_path,
    int resource_id,
    scoped_refptr<net::HttpResponseHeaders> headers,
    mojo::PendingRemote<network::mojom::URLLoaderClient> client) {
}

bool ShellExtensionsBrowserClient::AllowCrossRendererResourceLoad(
    const network::ResourceRequest& request,
    network::mojom::RequestDestination destination,
    ui::PageTransition page_transition,
    int child_id,
    bool is_incognito,
    const Extension* extension,
    const ExtensionSet& extensions,
    const ProcessMap& process_map) {
  bool allowed = false;
  if (url_request_util::AllowCrossRendererResourceLoad(
          request, destination, page_transition, child_id, is_incognito,
          extension, extensions, process_map, &allowed)) {
    return allowed;
  }

  // Couldn't determine if resource is allowed. Block the load.
  return true;
}

PrefService* ShellExtensionsBrowserClient::GetPrefServiceForContext(
    BrowserContext* context) {
  DCHECK(pref_service_);
  return pref_service_;
}

void ShellExtensionsBrowserClient::GetEarlyExtensionPrefsObservers(
    content::BrowserContext* context,
    std::vector<EarlyExtensionPrefsObserver*>* observers) const {}

bool ShellExtensionsBrowserClient::IsComponentExtensionResource(const base::FilePath& extension_path, const base::FilePath& resource_path, int* resource_id) const {
  return true;
}

const ui::TemplateReplacements* ShellExtensionsBrowserClient::GetTemplateReplacementsForExtension(const std::string& extension_id) const {
  LOG(INFO) << "ShellExtensionsBrowserClient GetTemplateReplacementsForExtension " << extension_id;
  return nullptr;
}

ProcessManagerDelegate*
ShellExtensionsBrowserClient::GetProcessManagerDelegate() const {
  return &process_manager_delegate_;
}

std::unique_ptr<ExtensionHostDelegate>
ShellExtensionsBrowserClient::CreateExtensionHostDelegate() {
  return base::WrapUnique(new ShellExtensionHostDelegate);
}

bool ShellExtensionsBrowserClient::DidVersionUpdate(BrowserContext* context) {
  // TODO(jamescook): We might want to tell extensions when app_shell updates.
  return false;
}

void ShellExtensionsBrowserClient::PermitExternalProtocolHandler() {
}

bool ShellExtensionsBrowserClient::IsInDemoMode() {
  return false;
}

bool ShellExtensionsBrowserClient::IsScreensaverInDemoMode(
    const std::string& app_id) {
  return false;
}

bool ShellExtensionsBrowserClient::IsRunningInForcedAppMode() {
  return false;
}

bool ShellExtensionsBrowserClient::IsAppModeForcedForApp(
    const ExtensionId& extension_id) {
  return false;
}

bool ShellExtensionsBrowserClient::IsLoggedInAsPublicAccount() {
  return false;
}

ExtensionSystemProvider*
ShellExtensionsBrowserClient::GetExtensionSystemFactory() {
  return ShellExtensionSystemFactory::GetInstance();
}

void ShellExtensionsBrowserClient::RegisterBrowserInterfaceBindersForFrame(
    mojo::BinderMapWithContext<content::RenderFrameHost*>* binder_map,
    content::RenderFrameHost* render_frame_host,
    const Extension* extension) const {
  PopulateExtensionFrameBinders(binder_map, render_frame_host, extension);
}

std::unique_ptr<RuntimeAPIDelegate>
ShellExtensionsBrowserClient::CreateRuntimeAPIDelegate(
    content::BrowserContext* context) const {
  return std::make_unique<ShellRuntimeAPIDelegate>(context);
}

const ComponentExtensionResourceManager*
ShellExtensionsBrowserClient::GetComponentExtensionResourceManager() {
  return this;
}

void ShellExtensionsBrowserClient::BroadcastEventToRenderers(
    events::HistogramValue histogram_value,
    const std::string& event_name,
    base::Value::List args,
    bool dispatch_to_off_the_record_profiles) {
  if (!BrowserThread::CurrentlyOn(BrowserThread::UI)) {
    content::GetUIThreadTaskRunner({})->PostTask(
        FROM_HERE,
        base::BindOnce(&ShellExtensionsBrowserClient::BroadcastEventToRenderers,
                       base::Unretained(this), histogram_value, event_name,
                       std::move(args), dispatch_to_off_the_record_profiles));
    return;
  }

  auto event =
      std::make_unique<Event>(histogram_value, event_name, std::move(args));
  EventRouter::Get(browser_context_)->BroadcastEvent(std::move(event));
}

ExtensionCache* ShellExtensionsBrowserClient::GetExtensionCache() {
  return extension_cache_.get();
}

bool ShellExtensionsBrowserClient::IsBackgroundUpdateAllowed() {
  return true;
}

bool ShellExtensionsBrowserClient::IsMinBrowserVersionSupported(
    const std::string& min_version) {
  return true;
}

void ShellExtensionsBrowserClient::SetAPIClientForTest(
    ExtensionsAPIClient* api_client) {
  api_client_.reset(api_client);
}

ExtensionWebContentsObserver*
ShellExtensionsBrowserClient::GetExtensionWebContentsObserver(
    content::WebContents* web_contents) {
  ExtensionWebContentsObserver* observer = nullptr;
  observer = ShellExtensionWebContentsObserver::FromWebContents(web_contents);
  return observer;
}

KioskDelegate* ShellExtensionsBrowserClient::GetKioskDelegate() {
  //if (!kiosk_delegate_)
  //  kiosk_delegate_ = std::make_unique<ShellKioskDelegate>();
  //return kiosk_delegate_.get();
  return nullptr;
}

bool ShellExtensionsBrowserClient::IsLockScreenContext(
    content::BrowserContext* context) {
  return false;
}

std::string ShellExtensionsBrowserClient::GetApplicationLocale() {
  // TODO(michaelpg): Use system locale.
  return "en-US";
}

std::string ShellExtensionsBrowserClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct(
      version_info::GetProductNameAndVersionForUserAgent());
}

void ShellExtensionsBrowserClient::InitWithBrowserContext(
    content::BrowserContext* context,
    PrefService* pref_service) {
  DCHECK(!browser_context_);
  DCHECK(!pref_service_);
  browser_context_ = context;
  pref_service_ = pref_service;
}

}  // namespace extensions